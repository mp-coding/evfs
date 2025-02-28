/*
 * FilesystemUnderTest.hpp
 * Created on: 13/02/2025
 * Author: Mateusz Piesta (mateusz.piesta@gmail.com)
 * Company: mprogramming
 */

#include "FilesystemUnderTest.hpp"
#include "partition_layout.hpp"

#include <vfs/disk.hpp>

namespace vfs::tests {

    VirtualFS& FilesystemUnderTest::get() const { return *vfs; }
    VirtualFS& FilesystemUnderTest::operator->() { return *vfs; }
    Disk&      FilesystemUnderTest::get_disk() const { return *disk; }

    ext4UnderTest::Builder& ext4UnderTest::Builder::with_multipartition()
    {
        multipartition = true;
        return *this;
    }
    std::unique_ptr<FilesystemUnderTest> ext4UnderTest::Builder::create()
    {
        auto instance = std::unique_ptr<ext4UnderTest>(new ext4UnderTest());

        instance->disk_mngr    = std::make_unique<DiskManager>();
        instance->block_device = std::make_unique<RAMBlockDevice>(blockdev_size);
        tools::fdisk::erase_mbr(*instance->block_device);
        tools::fdisk::create_mbr(*instance->block_device);

        write_partition_entry(*instance->block_device, layout::partition_0_conf);
        if (multipartition) { write_partition_entry(*instance->block_device, layout::partition_1_conf); }

        auto ret = instance->disk_mngr->register_device(*instance->block_device);
        if (not ret) { throw std::runtime_error {"Failed to register block device within disk manager"}; }
        instance->disk = *ret;

        auto part = instance->disk->borrow_partition(0);
        mkext(*part, layout::partition_0_ext, tools::mkfs::ext_type::ext4);

        if (multipartition) {
            auto spart = instance->disk->borrow_partition(1);
            mkext(*spart, layout::partition_1_ext, tools::mkfs::ext_type::ext4);
        }

        instance->vfs = std::make_unique<VirtualFS>(*instance->disk_mngr);
        std::ignore   = instance->vfs->register_filesystem(fstype::linux);
        if (automount) { instance->vfs->mount(); }

        return instance;
    }
    void ext4UnderTest::reload()
    {
        vfs         = std::make_unique<VirtualFS>(*disk_mngr);
        std::ignore = vfs->register_filesystem(fstype::linux);
        std::ignore = vfs->mount();
    }

} // namespace vfs::tests