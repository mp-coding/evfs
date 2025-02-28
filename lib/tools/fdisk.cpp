#include "api/vfs/tools/fdisk.hpp"

#include <array>
#include <algorithm>
#include <tuple>

namespace defs {
    constexpr std::byte operator""_byte(unsigned long long i) { return static_cast<std::byte>(i); }

    namespace offset {
        constexpr auto mbr_signature = 0x1FE;

        constexpr auto ptbl_start    = 0x1BE;
        constexpr auto ptbl_active   = 0x000;
        constexpr auto ptbl_type     = 0x004;
        constexpr auto ptbl_sect_cnt = 0x00c;
        constexpr auto ptbl_lba      = 0x008;

        constexpr auto ptbl_start_head = 0x01;
        constexpr auto ptbl_start_sec  = 0x02;
        constexpr auto ptbl_start_cyl  = 0x03;

        constexpr auto ptbl_end_head = 0x05;
        constexpr auto ptbl_end_sec  = 0x06;
        constexpr auto ptbl_end_cyl  = 0x07;
    } // namespace offset

    constexpr auto mbr_signature_lo = 0x55_byte;
    constexpr auto mbr_signature_hi = 0xAA_byte;
    constexpr auto mbr_size         = 512;

    constexpr auto ptbl_size = 16;

    constexpr auto ext_part       = 0x05;
    constexpr auto ext_linux_part = 0x85;
    constexpr auto ext_win98_part = 0x0f;
    constexpr auto num_parts      = 4;

} // namespace defs

namespace {
    auto to_word(const std::vector<std::byte>& vec, const std::size_t offs)
    {
        const auto buf = &vec[offs];
        return (static_cast<uint32_t>(buf[0]) << 0U) | (static_cast<uint32_t>(buf[1]) << 8U) | (static_cast<uint32_t>(buf[2]) << 16U)
            | (static_cast<uint32_t>(buf[3]) << 24U);
    }

    void to_word(std::uint8_t* ptr, std::uint32_t val)
    {
        *ptr++ = static_cast<uint8_t>(val);
        val >>= 8;
        *ptr++ = static_cast<uint8_t>(val);
        val >>= 8;
        *ptr++ = static_cast<uint8_t>(val);
        val >>= 8;
        *ptr = static_cast<uint8_t>(val);
    }

} // namespace

namespace vfs::tools::fdisk {
    bool is_extended(const uint8_t type) { return type == defs::ext_linux_part || type == defs::ext_part || type == defs::ext_win98_part; }

    auto read_partitions(const std::vector<std::byte>& buffer, std::array<MBRPartition, defs::num_parts>& parts) -> void
    {
        std::size_t offs = defs::offset::ptbl_start;
        for (auto& part : parts) {
            part.bootable     = static_cast<std::uint8_t>(buffer[defs::offset::ptbl_active + offs]) & 0x80;
            part.type         = static_cast<std::uint8_t>(buffer[defs::offset::ptbl_type + offs]);
            part.num_sectors  = to_word(buffer, defs::offset::ptbl_sect_cnt + offs);
            part.start_sector = to_word(buffer, defs::offset::ptbl_lba + offs);
            offs += defs::ptbl_size;
        }
    }

    auto validate_partition(const MBRPartition& part, const std::size_t sector_size, const std::size_t sector_count) -> bool
    {
        const auto this_size = static_cast<uint64_t>(sector_count) * static_cast<uint64_t>(sector_size);
        const auto poffset   = static_cast<uint64_t>(part.start_sector) * static_cast<uint64_t>(sector_size);
        const auto psize     = static_cast<uint64_t>(part.num_sectors) * static_cast<uint64_t>(sector_size);
        const auto pnext     = static_cast<uint64_t>(part.start_sector) * static_cast<uint64_t>(sector_size) + poffset;
        return not((poffset + psize > this_size) ||             // oversized
            (pnext < uint64_t(part.start_sector) * sector_size) // going backward
        );
    }

    std::error_code fetch_partitions(const BlockDevice::sector_t sector_size, const BlockDevice::sector_t sector_count, std::vector<std::byte>&& mbr,
        std::vector<MBRPartition>& entries)
    {
        // Check initial signature
        if ((mbr[defs::offset::mbr_signature] != defs::mbr_signature_lo) && (mbr[defs::offset::mbr_signature + 1] != defs::mbr_signature_hi)) {
            return from_errno(ENXIO);
        }

        std::array<MBRPartition, defs::num_parts> partitions;
        read_partitions(mbr, partitions);

        // Add not extended partitions
        int part_no {0};
        for (auto& part : partitions) {
            if (is_extended(part.type)) continue;

            if (not validate_partition(part, sector_size, sector_count)) continue;

            if (part.num_sectors) {
                part.physical_number = part_no;
                entries.emplace_back(part);
            }
            ++part_no;
        }
        return {};
    }

    std::error_code get_partition_entries(BlockDevice& blkdev, std::vector<MBRPartition>& entries)
    {
        const auto sector_size = blkdev.get_sector_size();
        if (not sector_size) { return sector_size.error(); }

        const auto sector_count = blkdev.get_sector_count();
        if (not sector_count) { return sector_count.error(); }

        auto mbr = std::vector<std::byte>(*sector_size);
        if (const auto ret = blkdev.read(*mbr.data(), 0, 1)) { return ret; }

        return fetch_partitions(*sector_size, *sector_count, std::move(mbr), entries);
    }

    std::error_code write_partition_entry(BlockDevice& blkdev, const partition_conf& part)
    {
        const auto sector_count = blkdev.get_sector_count();
        if (not sector_count) { return sector_count.error(); }

        if (part.num_sectors == 0) { return from_errno(EINVAL); }
        if (*sector_count - part.start_sector < part.num_sectors) { return from_errno(EINVAL); }

        auto mbr = std::vector<std::byte>(defs::mbr_size);

        if (const auto ret = blkdev.read(*mbr.data(), 0, 1)) { return ret; }

        auto* buffer = reinterpret_cast<std::uint8_t*>(mbr.data()) + defs::offset::ptbl_start + (part.physical_number * defs::ptbl_size);

        // status
        buffer[defs::offset::ptbl_active] = part.bootable ? 0x80 : 0x00;
        // partition type
        buffer[defs::offset::ptbl_type] = part.type;
        // sector count
        to_word(&buffer[defs::offset::ptbl_sect_cnt], part.num_sectors);
        // start sector(LBA)
        to_word(&buffer[defs::offset::ptbl_lba], part.start_sector);

        const auto spt                        = 63;                                                        // assume 63 sectors per track
        const auto hpc                        = 16;                                                        // assume 16 heads per cylinder
        auto       cy                         = static_cast<std::uint32_t>(part.start_sector / spt / hpc); // start cylinder
        auto       hd                         = static_cast<std::uint8_t>(part.start_sector / spt % hpc);  // start head
        auto       sc                         = static_cast<std::uint8_t>(part.start_sector % spt + 1);    // start sector
        buffer[defs::offset::ptbl_start_head] = hd;
        buffer[defs::offset::ptbl_start_sec]  = static_cast<std::uint8_t>((cy >> 2 & 0xC0) | sc);
        buffer[defs::offset::ptbl_start_cyl]  = static_cast<std::uint8_t>(cy);

        cy                                  = static_cast<std::uint32_t>((part.start_sector + part.num_sectors - 1) / spt / hpc); // end cylinder
        hd                                  = static_cast<std::uint8_t>((part.start_sector + part.num_sectors - 1) / spt % hpc);  // end head
        sc                                  = static_cast<std::uint8_t>((part.start_sector + part.num_sectors - 1) % spt + 1);    // end sector
        buffer[defs::offset::ptbl_end_head] = hd;
        buffer[defs::offset::ptbl_end_sec]  = static_cast<std::uint8_t>((cy >> 2 & 0xC0) | sc);
        buffer[defs::offset::ptbl_end_cyl]  = static_cast<std::uint8_t>(cy);

        return blkdev.write(*mbr.data(), 0, 1);
    }

    std::error_code write_partition_entry(BlockDevice& blkdev, const std::array<partition_conf, 4>& entries)
    {
        std::error_code result {};
        std::ignore = std::all_of(entries.begin(), entries.end(), [&blkdev, &result](const auto& entry) {
            const auto ret = write_partition_entry(blkdev, entry);
            if (not ret) { return true; }
            result = ret;
            return false;
        });
        return result;
    }

    std::error_code validate_mbr(BlockDevice& blkdev)
    {
        auto mbr = std::vector<std::byte>(defs::mbr_size);
        if (const auto ret = blkdev.read(*mbr.data(), 0, 1)) { return ret; }

        if ((mbr[defs::offset::mbr_signature] != defs::mbr_signature_lo) && (mbr[defs::offset::mbr_signature + 1] != defs::mbr_signature_hi)) {
            return from_errno(ENXIO);
        }
        return {};
    }
    std::error_code erase_mbr(BlockDevice& blkdev)
    {
        auto mbr = std::vector<std::byte>(defs::mbr_size);
        return blkdev.write(*mbr.data(), 0, 1);
    }
    std::error_code create_mbr(BlockDevice& blkdev)
    {
        auto mbr                             = std::vector<std::byte>(defs::mbr_size);
        mbr[defs::offset::mbr_signature]     = defs::mbr_signature_lo;
        mbr[defs::offset::mbr_signature + 1] = defs::mbr_signature_hi;
        return blkdev.write(*mbr.data(), 0, 1);
    }
} // namespace vfs::tools::fdisk