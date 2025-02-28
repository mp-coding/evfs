#pragma once

#include "api/vfs/blockdev.hpp"
#include <ff.h>

namespace vfs {
    class fatfs_handle {
    public:
        explicit fatfs_handle(BlockDevice& handle_);

        FATFS& get_fatfs();

    private:
        struct raw_handle {
            BlockDevice* handle {};
            bool         is_initialized {false};
        };

        friend raw_handle& get_raw(BYTE pdrive);

        static raw_handle raw;

        FATFS fatfs;
    };
} // namespace vfs
