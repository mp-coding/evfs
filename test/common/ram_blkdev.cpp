#include "ram_blkdev.hpp"

#include <memory>
#include <cassert>

namespace vfs::tests {
    RAMBlockDevice::RAMBlockDevice(const std::size_t total_size)
        : total_size(total_size)
        , memory(std::make_unique<std::byte[]>(total_size))
    {
    }
    std::error_code RAMBlockDevice::probe()
    {
        initialized = true;
        return {};
    }
    std::error_code RAMBlockDevice::flush() { return {}; }
    std::error_code RAMBlockDevice::write(const std::byte& buf, const sector_t lba, const std::size_t count)
    {
        const auto dst_addr = &memory[lba * sector_size];
        const auto src_addr = &buf;
        const auto to_write = count * sector_size;

        assert((dst_addr + to_write) <= &memory[total_size]);

        memcpy(dst_addr, src_addr, to_write);
        return {};
    }
    std::error_code RAMBlockDevice::read(std::byte& buf, const sector_t lba, const std::size_t count)
    {
        const auto dst_addr = &buf;
        const auto src_addr = &memory[lba * sector_size];
        const auto to_read  = count * sector_size;

        assert((src_addr + to_read) <= &memory[total_size]);

        memcpy(dst_addr, src_addr, to_read);
        return {};
    }
    result<std::size_t>           RAMBlockDevice::get_sector_size() const { return sector_size; }
    result<BlockDevice::sector_t> RAMBlockDevice::get_sector_count() const { return total_size / sector_size; }
    std::string                   RAMBlockDevice::get_name() const { return "ram0"; }
} // namespace vfs::tests
