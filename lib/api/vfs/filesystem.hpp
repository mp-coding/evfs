/*
 * filesystem.hpp
 * Created on: 10/02/2025
 * Author: Mateusz Piesta (mateusz.piesta@gmail.com)
 * Company: mprogramming
 */

#pragma once

#include <string>
#include <filesystem>
#include <system_error>
#include "defs.hpp"

struct statvfs;
struct stat;

namespace vfs {

    class FileHandle;
    class DirectoryHandle;
    class BlockDevice;

    class Filesystem {
    public:
        virtual ~Filesystem()             = default;
        auto operator=(const Filesystem&) = delete;

        virtual auto mount(std::string root, Flags flags) noexcept -> std::error_code = 0;
        virtual auto unmount() noexcept -> std::error_code                            = 0;
        virtual auto stat_vfs(const std::filesystem::path& path, struct statvfs& stat) noexcept -> std::error_code;

        /** Standard file access API */
        virtual auto open(const std::filesystem::path& abspath, Flags flags, int mode) noexcept -> result<std::unique_ptr<FileHandle>> = 0;
        virtual auto close(FileHandle& handle) noexcept -> std::error_code                                                             = 0;
        virtual auto write(FileHandle& handle, const char* ptr, size_t len) noexcept -> result<std::size_t>                            = 0;
        virtual auto read(FileHandle& handle, char* ptr, size_t len) noexcept -> result<std::size_t>                                   = 0;
        virtual auto lseek(FileHandle& handle, off_t pos, int dir) noexcept -> result<off_t>;
        virtual auto fstat(FileHandle& handle, struct stat& st) noexcept -> std::error_code;
        virtual auto stat(const std::filesystem::path& file, struct stat& st) noexcept -> std::error_code;
        virtual auto link(const std::filesystem::path& existing, const std::filesystem::path& newlink) noexcept -> std::error_code;
        virtual auto symlink(const std::filesystem::path& existing, const std::filesystem::path& newlink) noexcept -> std::error_code;
        virtual auto unlink(const std::filesystem::path& name) noexcept -> std::error_code;
        virtual auto rmdir(const std::filesystem::path& name) noexcept -> std::error_code;
        virtual auto rename(const std::filesystem::path& oldname, const std::filesystem::path& newname) noexcept -> std::error_code;
        virtual auto mkdir(const std::filesystem::path& path, int mode) noexcept -> std::error_code;

        /** Directory support API */
        virtual auto diropen(const std::filesystem::path& path) noexcept -> result<std::unique_ptr<DirectoryHandle>>;
        virtual auto dirreset(DirectoryHandle& handle) noexcept -> std::error_code;
        virtual auto dirnext(DirectoryHandle& handle, std::filesystem::path& filename, struct stat& filestat) -> std::error_code;
        virtual auto dirclose(DirectoryHandle& handle) noexcept -> std::error_code;

        /** Other fops API */
        virtual auto ftruncate(FileHandle& handle, off_t len) noexcept -> std::error_code;
        virtual auto fsync(FileHandle& handle) noexcept -> std::error_code;
        virtual auto ioctl(const std::filesystem::path& path, int cmd, void* arg) noexcept -> std::error_code;
        virtual auto utimens(const std::filesystem::path& path, std::array<timespec, 2>& tv) noexcept -> std::error_code;
        virtual auto flock(FileHandle& handle, int cmd) noexcept -> std::error_code;
        virtual auto isatty(FileHandle& handle) noexcept -> std::error_code;

        virtual auto chmod(const std::filesystem::path& path, mode_t mode) noexcept -> std::error_code;
        virtual auto fchmod(FileHandle& handle, mode_t mode) noexcept -> std::error_code;

        /// Try to fetch partition label
        virtual auto get_label() noexcept -> result<std::string>;
    };

    class FilesystemFactory {
    public:
        virtual ~FilesystemFactory()                                                     = default;
        virtual std::unique_ptr<Filesystem> create_filesystem(BlockDevice&, Flags flags) = 0;
    };

    template <typename Type> class RawHandle {
    public:
        Type& get_raw() noexcept { return handle; }

    private:
        Type handle;
    };

    class DirectoryHandle {
    public:
        virtual ~DirectoryHandle() = default;
        explicit DirectoryHandle(std::string root);

        DirectoryHandle(const DirectoryHandle&) = delete;
        auto operator=(const DirectoryHandle&)  = delete;

        [[nodiscard]] std::filesystem::path get_root() const noexcept;

    private:
        std::string root;
    };

    class FileHandle {
    public:
        virtual ~FileHandle() = default;
        FileHandle(std::string root, std::filesystem::path abspath);

        FileHandle(const FileHandle&)     = delete;
        auto operator=(const FileHandle&) = delete;

        [[nodiscard]] std::filesystem::path get_path() const noexcept;
        [[nodiscard]] std::filesystem::path get_root() const noexcept;

    private:
        std::filesystem::path abspath;
        std::string           root;
    };

} // namespace vfs
