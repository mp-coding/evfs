#include "common/ram_blkdev.hpp"
#include "common/FilesystemUnderTest.hpp"
#include "common/partition_layout.hpp"
#include "vfs/disk.hpp"

#include <vfs/disk_mngr.hpp>
#include <vfs/vfs.hpp>

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <catch2/catch_all.hpp>

#include <fcntl.h>

using namespace vfs::tests;

TEST_CASE("register/unregister filesystem")
{
    auto dmgr = vfs::DiskManager {};
    auto vfs  = vfs::VirtualFS {dmgr, std::make_unique<Stream>()};

    REQUIRE(vfs.register_filesystem(vfs::fstype::ext4).value() == 0);
    REQUIRE(vfs.register_filesystem(vfs::fstype::Type {"custom"}, {}).value() == 0);

    /// Try to register filesystem that was already registered
    REQUIRE(vfs.register_filesystem(vfs::fstype::ext4).value() == EEXIST);
    REQUIRE(vfs.register_filesystem(vfs::fstype::Type {"custom"}, {}).value() == EEXIST);

    /// Try to unregister non registered filesystem
    REQUIRE(vfs.unregister_filesystem(vfs::fstype::vfat).value() == ENOENT);
    REQUIRE(vfs.unregister_filesystem(vfs::fstype::ext4).value() == 0);
}

TEST_CASE("mount-all/umount-all")
{
    SECTION("no registered block device")
    {
        auto dmgr = vfs::DiskManager {};
        auto vfs  = vfs::VirtualFS {dmgr, std::make_unique<Stream>()};

        REQUIRE(vfs.mount_all().value() == ENOTBLK);
    }

    SECTION("disk that doesn't contain valid partitions")
    {
        auto dmgr       = vfs::DiskManager {};
        auto ram_blkdev = std::make_unique<RAMBlockDevice>(1024);
        REQUIRE(dmgr.register_device(*ram_blkdev));
        auto vfs = vfs::VirtualFS {dmgr, std::make_unique<Stream>()};

        REQUIRE(vfs.mount_all().value() == 0);
    }

    SECTION("success")
    {
        auto       fsut      = ext4UnderTest::Builder {}.create();
        const auto part_name = fsut->get_disk().borrow_partition(0)->get_name();

        REQUIRE(fsut->get().mount_all().value() == 0);
        REQUIRE(fsut->get().umount_all().value() == 0);
        REQUIRE(fsut->get().get_roots().empty());
    }
}

TEST_CASE("mount/umount")
{
    SECTION("non-existent or wrong disk name")
    {
        auto dmgr = vfs::DiskManager {};
        auto vfs  = vfs::VirtualFS {dmgr, std::make_unique<Stream>()};

        REQUIRE(vfs.mount("/wrongdisk", "/", {}).value() == EINVAL);
        REQUIRE(vfs.mount("", "/", {}).value() == EINVAL);

        REQUIRE(vfs.umount("/wrongroot").value() == EINVAL);
        REQUIRE(vfs.umount("").value() == EINVAL);
    }

    SECTION("mount")
    {
        auto       fsut      = ext4UnderTest::Builder {}.create();
        const auto part_name = fsut->get_disk().borrow_partition(0)->get_name();

        /// TODO: add support for explicit fs types
        SECTION("incorrect partition type") { REQUIRE(fsut->get().mount(part_name, "/", "vfat", {}).value() == 0); }
        SECTION("empty root")
        {
            /// Partition label will be used to construct mount point. If not available, mount point will be generated
            REQUIRE(fsut->get().mount(part_name, {}, {}).value() == 0);

            /// Try to mount already mounted partition
            REQUIRE(fsut->get().mount(part_name, {}, {}).value() == EEXIST);

            /// Check mounted partition stats
            const auto parts = fsut->get().stat_parts();
            REQUIRE(parts.size() == 1);
            REQUIRE(parts[0].disk_name == part_name);
            REQUIRE(parts[0].mount_point == test_volume0_name);
        }
        SECTION("filled root")
        {
            /// Partition label will be used to construct mount point. If not available, mount point will be generated
            REQUIRE(fsut->get().mount(part_name, "/root", {}).value() == 0);

            /// Try to mount already mounted partition
            REQUIRE(fsut->get().mount(part_name, "/root", {}).value() == EEXIST);

            /// Check mounted partition stats
            const auto parts = fsut->get().stat_parts();
            REQUIRE(parts.size() == 1);
            REQUIRE(parts[0].disk_name == part_name);
            REQUIRE(parts[0].mount_point == "/root");
        }
    }
    SECTION("umount")
    {
        SECTION("by specifying root path")
        {
            auto fsut = ext4UnderTest::Builder {}.set_automount().create();
            REQUIRE(fsut->get().umount(test_volume0_name.string()).value() == 0);
            REQUIRE(fsut->get().get_roots().empty());
        }
    }

    SECTION("mount/unmount with unlink - any files marked to unlink should be removed upon closing filesystem")
    {
        auto fsut = ext4UnderTest::Builder {}.set_automount().create();

        REQUIRE(fsut->get().open(test_volume0_name / "test.txt", O_WRONLY | O_CREAT, 0));
        REQUIRE(not fsut->get().unlink(test_volume0_name / "test.txt").value());

        /// Reload vfs to simulate filesystem close
        fsut->reload();

        struct stat st {};
        REQUIRE(fsut->get().stat(test_volume0_name / "test.txt", st).value() == ENOENT);
    }
}

TEST_CASE("various")
{
    auto fsut = ext4UnderTest::Builder {}.set_automount().create();

    SECTION("stat root")
    {
        struct stat st {};
        REQUIRE(not fsut->get().stat(test_volume0_name, st));
    }
}