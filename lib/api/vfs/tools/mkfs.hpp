/*
 * mkfs.hpp
 * Created on: 13/02/2025
 * Author: Mateusz Piesta (mateusz.piesta@gmail.com)
 * Company: mprogramming
 */

#pragma once

#include <string>
#include <array>
#include <system_error>

namespace vfs {
    class Partition;
    class Disk;
} // namespace vfs

namespace vfs::tools::mkfs {

    enum class ext_type { ext2, ext3, ext4 };

    /// If parameter is not set, it will be calculated automatically or set to default value
    struct ext_params {
        std::uint64_t        length;
        std::uint32_t        block_size;
        std::uint32_t        blocks_per_group;
        std::uint32_t        inodes_per_group;
        std::uint32_t        inode_size;
        std::uint32_t        inodes_count;
        std::uint32_t        journal_blocks;
        std::uint32_t        descriptor_size;
        bool                 journal;
        std::string          label;
        std::array<char, 16> uuid;
    };

    /**
     * Create EXT3/4 partition
     * @param part partition/disk handle
     * @param params EXT parameters
     * @param type @ext_type
     * @return 0 in case of success, otherwise an error
     */
    std::error_code mkext(Partition& part, const ext_params& params, ext_type type);

    /**
     * Create FAT12/16/32 partition
     * @warning Only one partition spanning the whole available disk can be created
     * @param d disk handle
     * @return 0 in case of success, otherwise an error
     */
    std::error_code mkfat(Disk& d);

} // namespace vfs::tools::mkfs