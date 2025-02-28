#include "common/FilesystemUnderTest.hpp"
#include "common/partition_layout.hpp"
#include "common/initializers.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <catch2/catch_all.hpp>

using namespace vfs::tests;
using namespace vfs;

TEMPLATE_PRODUCT_TEST_CASE("File descriptor related API", "", initializer, ext4_initializer)
{
    auto fs = TestType {}();

    SECTION("open/close")
    {
        /// open for write with 'O_CREAT' flag
        {
            auto fd = fs->get().open(test_volume0_name / "test.txt", O_WRONLY | O_CREAT, 0);
            REQUIRE(fd);
            REQUIRE(not fs->get().close(*fd));
        }
        /// pass wrong fd to close
        {
            auto fd = fs->get().open(test_volume0_name / "test.txt", O_WRONLY | O_CREAT, 0);
            REQUIRE(fd);
            REQUIRE(fs->get().close(*fd + 500) == from_errno(EBADF));
        }
        /// open for write without 'O_CREAT' flag
        {
            REQUIRE(fs->get().open(test_volume0_name / "test2.txt", O_WRONLY, 0) == error(ENOENT));
        }
        /// open non-existing file for read
        REQUIRE(fs->get().open(test_volume0_name / "non_existent.txt", O_RDONLY, 0) == error(ENOENT));

        /// open file for read using wrong mount point
        REQUIRE(fs->get().open("/wrong_mp/non_existent.txt", O_RDONLY, 0) == error(ENOENT));

        /// open file for read
        {
            /// Create empty file first
            auto fd = fs->get().open(test_volume0_name / "read.txt", O_WRONLY | O_CREAT, 0);
            fs->get().close(*fd);

            REQUIRE(fs->get().open(test_volume0_name / "read.txt", O_RDONLY, 0));
        }
        /// try to read write-only file
        {
            auto fd = fs->get().open(test_volume0_name / "wronly.txt", O_WRONLY | O_CREAT, 0);

            REQUIRE(fs->get().read(*fd, nullptr, 0) == error(EPERM));
        }
        /// try to write read-only file
        {
            auto fd = fs->get().open(test_volume0_name / "rdonly.txt", O_RDONLY | O_CREAT, 0);

            REQUIRE(fs->get().write(*fd, "", 0) == error(EPERM));
        }
        /// normalize path
        {
            auto fd = fs->get().open("/volume0/folder/../normalize.txt", O_WRONLY | O_CREAT, 0);
            REQUIRE(fd);
        }
        /// opening root not available
        {
            REQUIRE(not fs->get().open(test_volume0_name, O_RDONLY, 0));
        }
    }

    SECTION("file write")
    {
        auto test_string = std::string {"test string"};
        auto fd          = fs->get().open(test_volume0_name / "test.txt", O_WRONLY | O_CREAT, 0);
        REQUIRE(fd);

        /// success
        REQUIRE(fs->get().write(*fd, test_string.c_str(), test_string.size()).value() == test_string.size());
        /// wrong fd
        REQUIRE(fs->get().write(*fd + 500, test_string.c_str(), test_string.size()) == error(EBADF));
        /// 0 size
        REQUIRE(fs->get().write(*fd, test_string.c_str(), 0).value() == 0);

        fs->get().close(*fd);
    }

    SECTION("read")
    {
        auto test_string = std::string {"test string"};
        auto read_string = std::string(test_string.size(), 0);

        auto fd = fs->get().open(test_volume0_name / "test.txt", O_RDWR | O_CREAT, 0);
        REQUIRE(fd);

        /// Write test data
        REQUIRE(fs->get().write(*fd, test_string.c_str(), test_string.size()).value() == test_string.size());
        REQUIRE(fs->get().lseek(*fd, 0, SEEK_SET));

        /// success
        REQUIRE(fs->get().read(*fd, read_string.data(), test_string.size()).value() == test_string.size());
        REQUIRE(test_string == read_string);
        /// wrong fd
        REQUIRE(fs->get().read(*fd + 500, read_string.data(), test_string.size()) == error(EBADF));
        /// 0 size
        REQUIRE(fs->get().read(*fd, read_string.data(), 0).value() == 0);
    }

    SECTION("fstat")
    {
        auto test_string = std::string {"test string"};
        auto read_string = std::string(test_string.size(), 0);

        auto fd = fs->get().open(test_volume0_name / "test.txt", O_WRONLY | O_CREAT, 0);
        REQUIRE(fd);
        REQUIRE(fs->get().write(*fd, test_string.c_str(), test_string.size()).value() == test_string.size());
        REQUIRE(not fs->get().fsync(*fd));

        struct stat st {};
        REQUIRE(not fs->get().fstat(*fd, st));
        REQUIRE(st.st_size == static_cast<off_t>(test_string.size()));

        /// wrong file descriptor
        REQUIRE(fs->get().fstat(*fd + 500, st) == from_errno(EBADF));
    }

    SECTION("stat")
    {
        auto test_string = std::string {"test string"};
        auto read_string = std::string(test_string.size(), 0);

        auto fd = fs->get().open(test_volume0_name / "test.txt", O_WRONLY | O_CREAT, 0);
        REQUIRE(fd);
        REQUIRE(fs->get().write(*fd, test_string.c_str(), test_string.size()).value() == test_string.size());
        REQUIRE(not fs->get().close(*fd));

        struct stat st {};
        REQUIRE(not fs->get().stat(test_volume0_name / "test.txt", st));
        REQUIRE(st.st_size == static_cast<off_t>(test_string.size()));

        REQUIRE(fs->get().stat(test_volume0_name / "non_existent_file.txt", st) == from_errno(ENOENT));
    }

    SECTION("lseek")
    {
        auto test_string = std::string {"test string"};
        auto read_string = std::string(test_string.size(), 0);

        auto fd = fs->get().open(test_volume0_name / "test.txt", O_WRONLY | O_CREAT, 0);
        REQUIRE(fd);

        REQUIRE(fs->get().write(*fd, test_string.c_str(), test_string.size()).value() == test_string.size());

        /// Should be set to the end of file
        REQUIRE(fs->get().lseek(*fd, 0, SEEK_CUR).value() == static_cast<off_t>(test_string.size()));

        /// Move to the specific position starting from the current position
        REQUIRE(fs->get().lseek(*fd, -10, SEEK_CUR).value() == static_cast<off_t>(test_string.size()) - 10);

        /// Move right back to the beginning
        REQUIRE(fs->get().lseek(*fd, 0, SEEK_SET).value() == 0);

        /// Move to the specific position starting from the beginning of the file
        REQUIRE(fs->get().lseek(*fd, 4, SEEK_SET).value() == 4);

        /// Move to the specific position starting from the end of the file
        REQUIRE(fs->get().lseek(*fd, -3, SEEK_END).value() == static_cast<off_t>(test_string.size()) - 3);

        /// Moving backwards when using 'SEEK_SET' should return error
        REQUIRE(fs->get().lseek(*fd, -1, SEEK_SET) == error(EINVAL));
    }

    SECTION("rename")
    {
        auto fd = fs->get().open(test_volume0_name / "test.txt", O_CREAT | O_WRONLY, 0);
        REQUIRE(fd);

        REQUIRE(not fs->get().close(*fd));

        struct stat st {};
        REQUIRE(not fs->get().stat(test_volume0_name / "test.txt", st));

        REQUIRE(not fs->get().rename(test_volume0_name / "test.txt", test_volume0_name / "new_test.txt"));
        REQUIRE(fs->get().stat(test_volume0_name / "test.txt", st) == from_errno(ENOENT));
        REQUIRE(not fs->get().stat(test_volume0_name / "new_test.txt", st));
    }

    SECTION("unlink")
    {
        auto fd = fs->get().open(test_volume0_name / "test.txt", O_CREAT | O_WRONLY, 0);
        REQUIRE(fd);

        REQUIRE(not fs->get().close(*fd));

        REQUIRE(not fs->get().unlink(test_volume0_name / "test.txt"));

        struct stat st {};
        REQUIRE(fs->get().stat(test_volume0_name / "test.txt", st) == from_errno(ENOENT));
    }

    SECTION("file descriptor ref counting")
    {
        SECTION("multiple opened descriptors of the same file - w+")
        {
            auto fd  = fs->get().open(test_volume0_name / "test.txt", O_RDWR | O_CREAT, 0);
            auto fd1 = fs->get().open(test_volume0_name / "test.txt", O_RDWR | O_CREAT, 0);
            auto fd2 = fs->get().open(test_volume0_name / "test.txt", O_RDWR | O_CREAT, 0);
            REQUIRE(fd);
            REQUIRE(fd1);
            REQUIRE(fd2);

            REQUIRE(*fd != *fd1);
            REQUIRE(*fd1 != *fd2);

            REQUIRE(fs->get().write(*fd, "test1", 5).value() == 5);
            REQUIRE(fs->get().write(*fd1, "test2", 5).value() == 5);
            REQUIRE(fs->get().write(*fd2, "test3", 5).value() == 5);

            REQUIRE(not fs->get().close(*fd));
            REQUIRE(not fs->get().close(*fd1));
            REQUIRE(not fs->get().close(*fd2));

            char rd_buff[16] {};
            auto fd3 = fs->get().open(test_volume0_name / "test.txt", O_RDONLY, 0);
            REQUIRE(fd3);
            REQUIRE(fs->get().read(*fd3, rd_buff, sizeof(rd_buff)).value() == 5);
            REQUIRE(std::string {"test3"} == rd_buff);
            REQUIRE(not fs->get().close(*fd3));
        }

        SECTION("two readers of the same file")
        {
            auto fd = fs->get().open(test_volume0_name / "test.txt", O_WRONLY | O_CREAT, 0);
            REQUIRE(fd);
            REQUIRE(fs->get().write(*fd, "test", 4).value() == 4);
            REQUIRE(not fs->get().close(*fd));

            auto fd1 = fs->get().open(test_volume0_name / "test.txt", O_RDONLY, 0);
            auto fd2 = fs->get().open(test_volume0_name / "test.txt", O_RDONLY, 0);
            REQUIRE(fd1);
            REQUIRE(fd2);

            REQUIRE(*fd1 != *fd2);

            char rd_buff[16] {};
            REQUIRE(fs->get().read(*fd1, rd_buff, 4).value() == 4);
            REQUIRE(std::string {"test"} == rd_buff);

            REQUIRE(fs->get().read(*fd2, rd_buff, 4).value() == 4);
            REQUIRE(std::string {"test"} == rd_buff);

            REQUIRE(not fs->get().close(*fd1));
            REQUIRE(not fs->get().close(*fd2));
        }
    }
    SECTION("unlink/refcount")
    {
        auto fd = fs->get().open(test_volume0_name / "test.txt", O_WRONLY | O_CREAT, 0);
        REQUIRE(fd);

        REQUIRE(not fs->get().unlink(test_volume0_name / "test.txt"));

        struct stat st {};
        REQUIRE(not fs->get().stat(test_volume0_name / "test.txt", st));

        REQUIRE(not fs->get().close(*fd));
        REQUIRE(fs->get().stat(test_volume0_name / "test.txt", st) == from_errno(ENOENT));
    }
    SECTION("timestamps")
    {
        struct stat st {};
        const auto  current_time = std::time(nullptr);
        auto        fd           = fs->get().open(test_volume0_name / "test.txt", O_WRONLY | O_CREAT, 0);
        REQUIRE(fd);

        REQUIRE(not fs->get().close(*fd));
        REQUIRE(not fs->get().stat(test_volume0_name / "test.txt", st));
        REQUIRE_THAT(st.st_atime, Catch::Matchers::WithinAbs(current_time, 1.0));
    }
}
