#pragma once

#include <ext4_blockdev.h>
#include "api/vfs/blockdev.hpp"

#include <memory>

namespace vfs {

    class lwext4_handle {
    public:
        explicit lwext4_handle(BlockDevice& handle);
        ext4_blockdev&            get_blockdev();
        [[nodiscard]] std::string get_name() const;

    private:
        static int write(ext4_blockdev* bdev, const void* buf, std::uint64_t blk_id, std::uint32_t blk_cnt);
        static int read(ext4_blockdev* bdev, void* buf, std::uint64_t blk_id, std::uint32_t blk_cnt);
        static int open(ext4_blockdev* bdev);
        static int close(ext4_blockdev* bdev);

        BlockDevice&               device;
        ext4_blockdev              bdev {};
        std::unique_ptr<uint8_t[]> buf;
        ext4_blockdev_iface        ifc {};
    };
} // namespace vfs
