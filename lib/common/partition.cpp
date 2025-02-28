#include "api/vfs/partition.hpp"
#include "api/vfs/disk.hpp"

namespace vfs {
    /// Build partition name in a form of <disk_name>p<partition_number>, i.e 'disk0p0'
    std::string create_partition_name(const std::string_view disk, const std::size_t index) { return std::string {disk} + "p" + std::to_string(index); }

    Partition::Partition(Disk& disk, const tools::MBRPartition& info)
        : disk(disk)
        , info(info)
    {
    }
    std::error_code Partition::probe() { return disk.probe(); }
    std::error_code Partition::flush() { return disk.flush(); }
    std::error_code Partition::write(const std::byte& buf, const sector_t lba, const std::size_t count)
    {
        return disk.write(buf, translate_sector(lba), count);
    }
    std::error_code     Partition::read(std::byte& buf, const sector_t lba, const std::size_t count) { return disk.read(buf, translate_sector(lba), count); }
    result<std::size_t> Partition::get_sector_size() const { return disk.get_sector_size(); }
    result<BlockDevice::sector_t> Partition::get_sector_count() const { return info.num_sectors; }
    std::string                   Partition::get_name() const { return create_partition_name(disk.get_name(), info.physical_number); }
    BlockDevice::sector_t         Partition::translate_sector(const sector_t sector) const { return sector + info.start_sector; }
} // namespace vfs
