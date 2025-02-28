#pragma once

#include <vfs/tools/fdisk.hpp>
#include <vfs/tools/mkfs.hpp>
#include <vfs/tools/mbr_partition.hpp>

#include <filesystem>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

namespace vfs::tests {
    const std::filesystem::path test_volume0_name = "/volume0";
    const std::filesystem::path test_volume1_name = "/volume1";

    namespace layout {
        constexpr auto start_offset     = 1;
        constexpr auto partition_0_size = 1024 * 1024 * 32 / 512;
        constexpr auto partition_0_conf = tools::fdisk::partition_conf {0, start_offset, partition_0_size, tools::partition_code::linux, false};
        const auto     partition_0_ext  = tools::mkfs::ext_params {
                 .journal = true,
                 .label   = test_volume0_name.c_str() + 1,
                 .uuid    = {0x22, 0x11, 0x33, 0x22, 0x77, 0x33, 0x66, 0x44, 0x12, 0x11, 0x32, 0x22, 0x77, 0x33, 0x66, 0x44},
        };
        constexpr auto partition_1_size = 1024 * 1024 * 32 / 512;
        constexpr auto partition_1_conf = tools::fdisk::partition_conf {1, start_offset + partition_0_size, partition_1_size, tools::partition_code::linux, false};
        const auto     partition_1_ext  = tools::mkfs::ext_params {
                 .journal = true,
                 .label   = test_volume1_name.c_str() + 1,
                 .uuid    = {0x20, 0x11, 0x30, 0x22, 0x77, 0x30, 0x60, 0x44, 0x12, 0x11, 0x32, 0x22, 0x77, 0x33, 0x66, 0x44},
        };
    } // namespace layout
} // namespace vfs::tests

#pragma GCC diagnostic pop
