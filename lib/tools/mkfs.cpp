#include "api/vfs/tools/mkfs.hpp"
#include "api/vfs/defs.hpp"
#include "api/vfs/partition.hpp"
#include "api/vfs/disk.hpp"

#include <ext4_mkfs.h>
#include "fstypes/handle/lwext4_handle.hpp"

namespace vfs::tools::mkfs {

    std::error_code mkext(Partition& part, const ext_params& params, const ext_type type)
    {
        auto           ctx = lwext4_handle {part};
        ext4_fs        fs {};
        ext4_mkfs_info info {};

        info.len              = params.length;
        info.block_size       = params.block_size;
        info.blocks_per_group = params.blocks_per_group;
        info.inodes_per_group = params.inodes_per_group;
        info.inode_size       = params.inode_size;
        info.inodes           = params.inodes_count;
        info.journal_blocks   = params.journal_blocks;
        info.dsc_size         = params.descriptor_size;
        std::copy(params.uuid.begin(), params.uuid.end(), info.uuid);
        info.journal = params.journal;
        snprintf(info.label, sizeof info.label, "%s", params.label.data());

        int fs_type {F_SET_EXT4};
        switch (type) {
        case ext_type::ext2:
            fs_type = F_SET_EXT2;
            break;
        case ext_type::ext3:
            fs_type = F_SET_EXT3;
            break;
        case ext_type::ext4:
            fs_type = F_SET_EXT4;
            break;
        }
        return from_errno(ext4_mkfs(&fs, &ctx.get_blockdev(), &info, fs_type));
    }

    std::error_code mkfat(Disk&) { return from_errno(ENOTSUP); }
} // namespace vfs::tools::mkfs