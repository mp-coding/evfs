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
    auto vfs  = vfs::VirtualFS {dmgr};

    REQUIRE(vfs.register_filesystem(vfs::fstype::linux).value() == 0);
    REQUIRE(vfs.register_filesystem(vfs::fstype::Type {"custom"}, {}).value() == 0);

    /// Try to register filesystem that was already registered
    REQUIRE(vfs.register_filesystem(vfs::fstype::linux).value() == EEXIST);
    REQUIRE(vfs.register_filesystem(vfs::fstype::Type {"custom"}, {}).value() == EEXIST);

    /// Try to unregister non registered filesystem
    REQUIRE(vfs.unregister_filesystem(vfs::fstype::fat).value() == ENOENT);
    REQUIRE(vfs.unregister_filesystem(vfs::fstype::linux).value() == 0);
}

TEST_CASE("mount/unmount")
{
    SECTION("auto-mount without registered block device")
    {
        auto dmgr = vfs::DiskManager {};
        auto vfs  = vfs::VirtualFS {dmgr};

        REQUIRE(vfs.mount().value() == ENOTBLK);
    }

    SECTION("auto-mount disk that does not contain valid partitions")
    {
        auto dmgr       = vfs::DiskManager {};
        auto ram_blkdev = std::make_unique<RAMBlockDevice>(1024);
        REQUIRE(dmgr.register_device(*ram_blkdev));
        auto _vfs = vfs::VirtualFS {dmgr};

        REQUIRE(_vfs.mount().value() == 0);
    }

    SECTION("mount specific partition")
    {
        SECTION("no valid partition")
        {
            auto dmgr       = vfs::DiskManager {};
            auto ram_blkdev = std::make_unique<RAMBlockDevice>(1024);
            REQUIRE(dmgr.register_device(*ram_blkdev));
            auto vfs = vfs::VirtualFS {dmgr};

            REQUIRE(vfs.mount(ram_blkdev->get_name() + "p0", "/root", vfs::Flags {}).value() == EINVAL);
        }

        SECTION("success")
        {
            auto       fsut      = ext4UnderTest::Builder {}.create();
            const auto part_name = fsut->get_disk().borrow_partition(0)->get_name();

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

        SECTION("auto-mount")
        {
            SECTION("single partition")
            {
                auto       fsut      = ext4UnderTest::Builder {}.create();
                const auto part_name = fsut->get_disk().borrow_partition(0)->get_name();

                REQUIRE(fsut->get().mount().value() == 0);

                /// Check mounted partition stats
                const auto parts = fsut->get().stat_parts();
                REQUIRE(parts.size() == 1);
                REQUIRE(parts[0].disk_name == part_name);
                REQUIRE(parts[0].mount_point == test_volume0_name);
            }

            SECTION("multiple partitions")
            {
                auto       fsut       = ext4UnderTest::Builder {}.with_multipartition().create();
                const auto part0_name = fsut->get_disk().borrow_partition(0)->get_name();
                const auto part1_name = fsut->get_disk().borrow_partition(1)->get_name();

                REQUIRE(fsut->get().mount().value() == 0);

                /// Check mounted partition stats
                const auto parts = fsut->get().stat_parts();
                REQUIRE(parts.size() == 2);
                REQUIRE(parts[0].disk_name == part1_name);
                REQUIRE(parts[0].mount_point == test_volume1_name);
                REQUIRE(parts[1].disk_name == part0_name);
                REQUIRE(parts[1].mount_point == test_volume0_name);
            }
        }
    }

    SECTION("unmount")
    {
        SECTION("no mounted partitions")
        {
            auto fsut = ext4UnderTest::Builder {}.create();
            REQUIRE(fsut->get().unmount().value() == 0);
        }

        SECTION("by specifying root path")
        {
            auto fsut = ext4UnderTest::Builder {}.set_automount().create();
            REQUIRE(fsut->get().unmount("/non-existent_root").value() == EINVAL);
            REQUIRE(fsut->get().unmount(test_volume0_name.string()).value() == 0);
            REQUIRE(fsut->get().get_roots().empty());
        }

        SECTION("auto-unmount")
        {
            auto fsut = ext4UnderTest::Builder {}.set_automount().create();
            REQUIRE(fsut->get().unmount().value() == 0);
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

#if 0
TEST_CASE("mount tiny FAT filesystem(~128kB)")
{
    auto ctx = prepare_vfat_context(true, 1024 * 128);

    static uint32_t cnt = 0;
    while (cnt++ < 1024 * 128) {
        const auto fd = ctx->m_vfs->open("/volume0/w_" + std::to_string(cnt) + ".txt", O_WRONLY | O_CREAT, 0);
        REQUIRE(fd >= 0);
        REQUIRE(ctx->m_vfs->write(fd, "test", 4) == 4);
        REQUIRE(ctx->m_vfs->close(fd) == 0);
        REQUIRE(ctx->m_vfs->unlink("/volume0/w_" + std::to_string(cnt) + ".txt") == 0);
    }
}
#endif