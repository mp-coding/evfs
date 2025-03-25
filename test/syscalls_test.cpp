#include "initializers.hpp"

#include <vfs/syscalls.hpp>
#include "common/FilesystemUnderTest.hpp"
#include "common/partition_layout.hpp"

#include <catch2/catch_all.hpp>

#include <fcntl.h>

using namespace vfs::tests;

namespace vfs {
    std::unique_ptr<FilesystemUnderTest> fut;
    VirtualFS&                           borrow_vfs() { return fut->get(); }
} // namespace vfs

namespace {
    void spawn_test_file(const std::filesystem::path& name, const std::string& content)
    {
        using namespace vfs;
        auto fd = syscalls::open(errno, name.c_str(), O_RDWR | O_CREAT, 0);
        syscalls::write(errno, fd, content.data(), content.size());
        syscalls::close(errno, fd);
    }

} // namespace

TEST_CASE("syscalls")
{
    using namespace vfs;
    errno = 0;
    fut.reset();
    fut = ext4UnderTest::Builder {}.set_automount().create();

    SECTION("File descriptor related")
    {
        auto fd = syscalls::open(errno, (test_volume0_name / "test.txt").c_str(), O_RDWR | O_CREAT, 0);
        REQUIRE(fd > 0);
        REQUIRE(errno == 0);

        REQUIRE(syscalls::write(errno, fd, "test", 4) == 4);
        REQUIRE(errno == 0);

        REQUIRE(syscalls::fsync(errno, fd) == 0);
        REQUIRE(errno == 0);

        REQUIRE(syscalls::lseek(errno, fd, 0, SEEK_SET) == 0);
        REQUIRE(errno == 0);

        REQUIRE(syscalls::lseek(errno, fd, 50, SEEK_SET) == -1);
        REQUIRE(errno == EINVAL);
        errno = 0;

        std::string read_buffer(4, 0);
        REQUIRE(syscalls::read(errno, fd, read_buffer.data(), 4) == 4);
        REQUIRE(read_buffer == "test");
        REQUIRE(errno == 0);

        struct stat st {};
        REQUIRE(syscalls::fstat(errno, fd, &st) == 0);
        REQUIRE(st.st_size == 4);
        REQUIRE(st.st_mode & S_IFREG);
        REQUIRE(errno == 0);

        REQUIRE(syscalls::fstat(errno, 1, &st) == 0);
        REQUIRE(errno == 0);

        REQUIRE(syscalls::close(errno, fd) == 0);
        REQUIRE(errno == 0);
        st = {};
        REQUIRE(syscalls::stat(errno, (test_volume0_name / "test.txt").c_str(), &st) == 0);
        REQUIRE(st.st_size == 4);
        REQUIRE(st.st_mode & S_IFREG);
        REQUIRE(errno == 0);

        /// Try to open root directory
        st = {};
        REQUIRE(syscalls::stat(errno, test_volume0_name.c_str(), &st) == 0);
        REQUIRE(st.st_mode & S_IFDIR);
    }

    SECTION("Directory related")
    {
        SECTION("readdir")
        {
            REQUIRE(syscalls::mkdir(errno, (test_volume0_name / "test").c_str(), 0777) == 0);
            REQUIRE(errno == 0);
            REQUIRE(syscalls::mkdir(errno, (test_volume0_name / "test2").c_str(), 0777) == 0);
            REQUIRE(errno == 0);

            SECTION("open root")
            {
                auto dirh = syscalls::opendir(errno, test_volume0_name.c_str());
                REQUIRE(dirh != nullptr);
                REQUIRE(errno == 0);

                auto entry = syscalls::readdir(errno, dirh);
                REQUIRE(entry != nullptr);
                REQUIRE(std::string {entry->d_name} == ".");
                REQUIRE(errno == 0);

                entry = syscalls::readdir(errno, dirh);
                REQUIRE(entry != nullptr);
                REQUIRE(std::string {entry->d_name} == "..");
                REQUIRE(errno == 0);

                entry = syscalls::readdir(errno, dirh);
                REQUIRE(entry != nullptr);
                REQUIRE(std::string {entry->d_name} == "lost+found");
                REQUIRE(errno == 0);

                entry = syscalls::readdir(errno, dirh);
                REQUIRE(entry != nullptr);
                REQUIRE(std::string {entry->d_name} == "test");
                REQUIRE(errno == 0);

                entry = syscalls::readdir(errno, dirh);
                REQUIRE(entry != nullptr);
                REQUIRE(std::string {entry->d_name} == "test2");
                REQUIRE(errno == 0);

                REQUIRE(syscalls::readdir(errno, dirh) == nullptr);
                REQUIRE(errno == 0);

                REQUIRE(syscalls::closedir(errno, dirh) == 0);
                REQUIRE(errno == 0);
            }

            SECTION("sub-dir")
            {
                spawn_test_file(test_volume0_name / "test/test1.txt", "test");
                spawn_test_file(test_volume0_name / "test/test2.txt", "test");

                auto dirh = syscalls::opendir(errno, (test_volume0_name / "test").c_str());
                REQUIRE(dirh != nullptr);
                REQUIRE(errno == 0);

                auto entry = syscalls::readdir(errno, dirh);
                REQUIRE(entry != nullptr);
                REQUIRE(std::string {entry->d_name} == ".");
                REQUIRE(errno == 0);

                entry = syscalls::readdir(errno, dirh);
                REQUIRE(entry != nullptr);
                REQUIRE(std::string {entry->d_name} == "..");
                REQUIRE(errno == 0);

                entry = syscalls::readdir(errno, dirh);
                REQUIRE(entry != nullptr);
                REQUIRE(std::string {entry->d_name} == "test1.txt");
                REQUIRE(errno == 0);

                entry = syscalls::readdir(errno, dirh);
                REQUIRE(entry != nullptr);
                REQUIRE(std::string {entry->d_name} == "test2.txt");
                REQUIRE(errno == 0);

                REQUIRE(syscalls::readdir(errno, dirh) == nullptr);
                REQUIRE(errno == 0);

                REQUIRE(syscalls::closedir(errno, dirh) == 0);
                REQUIRE(errno == 0);
            }
        }
    }
}

/// add mount/umount syscalls