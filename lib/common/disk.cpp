#include "vfs/disk.hpp"
#include "vfs/tools/fdisk.hpp"

namespace vfs {

    Disk::Disk(BlockDevice& device)
        : device {device}
    {
    }

    result<std::unique_ptr<Disk>> Disk::create(BlockDevice& device)
    {
        std::vector<tools::MBRPartition> parts;
        /// Blockdev might not have MBR entry at all which may be fine. There's a chance it contains raw filesystem without partition table.
        if (const auto ret = tools::fdisk::get_partition_entries(device, parts); ret and ret.value() != ENXIO) { return error(ret); }

        auto disk = std::unique_ptr<Disk>(new Disk(device));
        for (const auto& p : parts) { disk->partitions.emplace_back(*disk, p); }
        return disk;
    }

    Partition* Disk::borrow_partition(const std::uint8_t nr)
    {
        if (nr >= partitions.size()) { return nullptr; }
        return &partitions[nr];
    }
    std::error_code Disk::probe()
    {
        auto_lock _lock(mutex);
        return device.probe();
    }
    std::error_code Disk::flush()
    {
        auto_lock _lock(mutex);
        return device.flush();
    }
    std::error_code Disk::write(const std::byte& buf, const sector_t lba, const std::size_t count)
    {
        auto_lock _lock(mutex);
        return device.write(buf, lba, count);
    }
    std::error_code Disk::read(std::byte& buf, const sector_t lba, const std::size_t count)
    {
        auto_lock _lock(mutex);
        return device.read(buf, lba, count);
    }
    result<std::size_t> Disk::get_sector_size() const
    {
        auto_lock _lock(mutex);
        return device.get_sector_size();
    }
    result<BlockDevice::sector_t> Disk::get_sector_count() const
    {
        auto_lock _lock(mutex);
        return device.get_sector_count();
    }
    std::string Disk::get_name() const
    {
        auto_lock _lock(mutex);
        return device.get_name();
    }
} // namespace vfs