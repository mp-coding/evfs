/*
 * fdisk.hpp
 * Created on: 13/02/2025
 * Author: Mateusz Piesta (mateusz.piesta@gmail.com)
 * Company: mprogramming
 */

#pragma once

#include "vfs/blockdev.hpp"
#include "mbr_partition.hpp"

#include <vector>
#include <array>

namespace vfs::tools::fdisk {

    struct partition_conf {
        std::uint8_t          physical_number {}; //! Partition physical number in part table
        BlockDevice::sector_t start_sector {};    //! First sector
        std::size_t           num_sectors {};     //! Number of sectors
        std::uint8_t          type {};            //! Partition code
        bool                  bootable {};        //! Partition is bootable
    };

    /**
     * Read and validate MBR signature
     * @param blkdev blockdevice to work on
     * @return 0 in case of success, otherwise an error
     */
    std::error_code validate_mbr(BlockDevice& blkdev);

    /**
     * Wipe out MBR entry
     * @param blkdev blockdevice to work on
     * @return 0 in case of success, otherwise an error
     */

    std::error_code erase_mbr(BlockDevice& blkdev);

    /**
     * Create and fill MBR entry with zeros
     * @param blkdev blockdevice to work on
     * @return 0 in case of success, otherwise an error
     */
    std::error_code create_mbr(BlockDevice& blkdev);

    /**
     * Read partition entries from MBR
     * @param blkdev blockdevice to work on
     * @param entries vector to be filled with entries
     * @return 0 in case of success, otherwise an error
     */
    std::error_code get_partition_entries(BlockDevice& blkdev, std::vector<MBRPartition>& entries);

    /**
     * Write single partition entry into MBR
     * @param blkdev blockdevice to work on
     * @param part partition configuration
     * @return 0 in case of success, otherwise an error
     */
    std::error_code write_partition_entry(BlockDevice& blkdev, const partition_conf& part);
    std::error_code write_partition_entry(BlockDevice& blkdev, const std::array<partition_conf, 4>& entries);
} // namespace vfs::tools::fdisk
