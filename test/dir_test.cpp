#include "common/initializers.hpp"
#include "common/partition_layout.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <catch2/catch_all.hpp>

using namespace vfs::tests;

TEMPLATE_PRODUCT_TEST_CASE("Directory related API", "", (initializer), (ext4_initializer))
{
    auto fsut = TestType {}();

    SECTION("mkdir")
    {
        REQUIRE(not fsut->get().mkdir(test_volume0_name / "test", 0777));

        REQUIRE(fsut->get().mkdir("/wrong_mount_point/test", 0777).value() == ENOENT);
    }

    SECTION("rmdir")
    {
        REQUIRE(not fsut->get().mkdir(test_volume0_name / "test", 0777));

        REQUIRE(not fsut->get().rmdir(test_volume0_name / "test"));

        REQUIRE(fsut->get().rmdir("/wrong_mount_point/test").value() == ENOENT);
    }

    SECTION("opendir")
    {
        REQUIRE(not fsut->get().mkdir(test_volume0_name / "test", 0777));

        REQUIRE(fsut->get().diropen(test_volume0_name / "test"));
        REQUIRE(fsut->get().diropen("/wrong_mount_point/test").error().value() == ENOENT);
        REQUIRE(fsut->get().diropen(test_volume0_name / "non_existent_dir").error().value() == ENOENT);
    }

    SECTION("dirclose")
    {
        REQUIRE(not fsut->get().mkdir(test_volume0_name / "test", 0777));

        auto dirh = fsut->get().diropen(test_volume0_name / "test");
        REQUIRE(dirh);
        REQUIRE(not fsut->get().dirclose(*dirh.value()));
    }

    SECTION("rename")
    {
        REQUIRE(not fsut->get().mkdir(test_volume0_name / "test", 0777));

        struct stat st {};
        REQUIRE(not fsut->get().stat(test_volume0_name / "test", st));

        REQUIRE(not fsut->get().rename(test_volume0_name / "test", test_volume0_name / "new_test"));
        REQUIRE(fsut->get().stat(test_volume0_name / "test", st).value() == ENOENT);
        REQUIRE(not fsut->get().stat(test_volume0_name / "new_test", st));
    }
}

TEST_CASE("Directory related API: ext4 specific")
{
    using namespace vfs::tests;

    auto fsut = ext4UnderTest::Builder {}.set_automount().create();

    SECTION("dirnext")
    {
        SECTION("root")
        {
            auto root = fsut->get().diropen(test_volume0_name);
            REQUIRE(root);

            /// Fill root with some test directories
            REQUIRE(not fsut->get().mkdir(test_volume0_name / "test", 0777));
            REQUIRE(not fsut->get().mkdir(test_volume0_name / "test2", 0777));

            std::filesystem::path entry;
            struct stat           st {};

            REQUIRE(not fsut->get().dirnext(*root.value(), entry, st));
            REQUIRE(entry == ".");
            REQUIRE(st.st_mode & S_IFDIR);

            REQUIRE(not fsut->get().dirnext(*root.value(), entry, st));
            REQUIRE(entry == "..");
            REQUIRE(st.st_mode & S_IFDIR);

            REQUIRE(not fsut->get().dirnext(*root.value(), entry, st));
            REQUIRE(entry == "lost+found");
            REQUIRE(st.st_mode & S_IFDIR);

            REQUIRE(not fsut->get().dirnext(*root.value(), entry, st));
            REQUIRE(entry == "test");
            REQUIRE(st.st_mode & S_IFDIR);

            REQUIRE(not fsut->get().dirnext(*root.value(), entry, st));
            REQUIRE(entry == "test2");
            REQUIRE(st.st_mode & S_IFDIR);

            REQUIRE(fsut->get().dirnext(*root.value(), entry, st).value() == ENOENT);
        }

        SECTION("open sub-dir")
        {
            /// Prepare test directory and generate several test files
            REQUIRE(not fsut->get().mkdir(test_volume0_name / "test", 0777));

            auto dirh = fsut->get().diropen(test_volume0_name / "test");
            REQUIRE(dirh);

            auto fd = fsut->get().open(test_volume0_name / "test/entry1.txt", O_CREAT | O_WRONLY, 0);
            REQUIRE(fd);
            REQUIRE(fsut->get().write(*fd, "test", 4));
            REQUIRE(not fsut->get().close(*fd));
            fd = fsut->get().open(test_volume0_name / "test/entry2.txt", O_CREAT | O_WRONLY, 0);
            REQUIRE(fd);
            REQUIRE(fsut->get().write(*fd, "test", 4));
            REQUIRE(not fsut->get().close(*fd));

            std::filesystem::path entry;
            struct stat           st {};

            REQUIRE(not fsut->get().dirnext(*dirh.value(), entry, st));
            REQUIRE(entry == ".");
            REQUIRE(st.st_mode & S_IFDIR);

            REQUIRE(not fsut->get().dirnext(*dirh.value(), entry, st));
            REQUIRE(entry == "..");
            REQUIRE(st.st_mode & S_IFDIR);

            REQUIRE(not fsut->get().dirnext(*dirh.value(), entry, st));
            REQUIRE(entry == "entry1.txt");
            REQUIRE(st.st_mode & S_IFREG);

            REQUIRE(not fsut->get().dirnext(*dirh.value(), entry, st));
            REQUIRE(entry == "entry2.txt");
            REQUIRE(st.st_mode & S_IFREG);

            REQUIRE(fsut->get().dirnext(*dirh.value(), entry, st).value() == ENOENT);
        }
    }
}
#if 0
TEMPLATE_PRODUCT_TEST_CASE("Directory related API: vfat specific", "", (initializer), (fat_initializer))
{
    auto ctx = TestType {}();

    SECTION("dirnext")
    {
        REQUIRE(ctx->m_vfs->mkdir(test_volume0_name / "test", 0777) == 0);
        REQUIRE(ctx->m_vfs->mkdir(test_volume0_name / "test2", 0777) == 0);

        SECTION("open root")
        {
            auto dirh = ctx->m_vfs->diropen(test_volume0_name);

            std::filesystem::path entry;
            struct stat           st {};

            REQUIRE(ctx->m_vfs->dirnext(*dirh.value(), entry, st) == 0);
            REQUIRE(entry == "TEST");
            REQUIRE(st.st_mode & S_IFDIR);

            REQUIRE(ctx->m_vfs->dirnext(*dirh.value(), entry, st) == 0);
            REQUIRE(entry == "TEST2");
            REQUIRE(st.st_mode & S_IFDIR);

            REQUIRE(ctx->m_vfs->dirnext(*dirh.value(), entry, st) == -ENOENT);
        }

        SECTION("open sub-dir")
        {
            /// Prepare test directory and generate several test files
            auto dirh = ctx->m_vfs->diropen(test_volume0_name / "test");
            auto fd   = ctx->m_vfs->open(test_volume0_name / "test/entry1.txt", O_CREAT | O_WRONLY, 0);
            ctx->m_vfs->write(fd, "test", 4);
            ctx->m_vfs->close(fd);
            fd = ctx->m_vfs->open(test_volume0_name / "test/entry2.txt", O_CREAT | O_WRONLY, 0);
            ctx->m_vfs->write(fd, "test", 4);
            ctx->m_vfs->close(fd);

            std::filesystem::path entry;
            struct stat           st {};

            REQUIRE(ctx->m_vfs->dirnext(*dirh.value(), entry, st) == 0);
            REQUIRE(entry == "ENTRY1.TXT");
            REQUIRE(st.st_mode & S_IFREG);

            REQUIRE(ctx->m_vfs->dirnext(*dirh.value(), entry, st) == 0);
            REQUIRE(entry == "ENTRY2.TXT");
            REQUIRE(st.st_mode & S_IFREG);

            REQUIRE(ctx->m_vfs->dirnext(*dirh.value(), entry, st) == -ENOENT);
        }
    }
}
#endif