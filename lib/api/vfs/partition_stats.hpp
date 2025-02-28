/*
 * partition_stats.hpp
 * Created on: 13/02/2025
 * Author: Mateusz Piesta (mateusz.piesta@gmail.com)
 * Company: mprogramming
 */

#pragma once

#include "defs.hpp"
#include <string>

namespace vfs {
    struct PartitionStats {
        std::string disk_name;   //!< Disk or partition name, e.g. 'sd0p0'
        std::string mount_point; //!< Mount point path, e.g. '/root'
        std::string type;        //!< Mount partition type, i.e. 'linux','vfat', etc.
        Flags       flags;       //!< Mount point flags
        std::size_t used_space;  //!< Used space in bytes
        std::size_t free_space;  //!< Free space in bytes
    };
} // namespace vfs
