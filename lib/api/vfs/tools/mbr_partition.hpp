/*
 * mbr_partition.hpp
 * Created on: 13/02/2025
 * Author: Mateusz Piesta (mateusz.piesta@gmail.com)
 * Company: mprogramming
 */

#pragma once

#include "vfs/blockdev.hpp"

#include <cstdint>

namespace vfs::tools {

    namespace partition_code {
#undef linux
        constexpr std::uint8_t linux     = 0x83; /// Linux(ext)
        constexpr std::uint8_t vfat32chs = 0x0B; /// FAT32(CHS)
        constexpr std::uint8_t vfat32    = 0x0C; /// FAT32(LBA)
        constexpr std::uint8_t vfat16    = 0x06; /// FAT16B
        constexpr std::uint8_t vfat12    = 0x01; /// FAT12(LBA)
    } // namespace partition_code

    struct MBRPartition {
        MBRPartition() = default;

        MBRPartition(const std::uint8_t nr, const BlockDevice::sector_t start_sector, const std::size_t num_sectors, const std::uint8_t type, const bool bootable)
            : physical_number {nr}
            , start_sector {start_sector}
            , num_sectors {num_sectors}
            , bootable {bootable}
            , type {type}
        {
        }

        std::uint8_t          physical_number {}; /// Partition physical number in part table
        BlockDevice::sector_t start_sector {};    /// First sector
        std::size_t           num_sectors {};     /// Number of sectors
        bool                  bootable {};        /// Partition is bootable
        std::uint8_t          type {};            /// Partition type
    };
} // namespace vfs::tools
