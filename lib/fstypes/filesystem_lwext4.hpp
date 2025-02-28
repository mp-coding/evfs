#pragma once

#include "api/vfs/filesystem.hpp"
#include "handle/lwext4_handle.hpp"

#include <ext4.h>

namespace vfs {
    class partition;

    class filesystem_lwext4 final : public Filesystem {
    public:
        filesystem_lwext4(BlockDevice& bdev, Flags flags);

        auto mount(std::string root, Flags flags) noexcept -> std::error_code override;
        auto unmount() noexcept -> std::error_code override;
        auto stat_vfs(const std::filesystem::path& path, struct statvfs& stat) noexcept -> std::error_code override;

        /** Standard file access API */
        auto open(const std::filesystem::path& abspath, Flags flags, int mode) noexcept -> result<std::unique_ptr<FileHandle>> override;
        auto close(FileHandle& handle) noexcept -> std::error_code override;
        auto write(FileHandle& handle, const char* ptr, size_t len) noexcept -> result<std::size_t> override;
        auto read(FileHandle& handle, char* ptr, size_t len) noexcept -> result<std::size_t> override;
        auto lseek(FileHandle& handle, off_t pos, int dir) noexcept -> result<off_t> override;
        auto fstat(FileHandle& handle, struct stat& st) noexcept -> std::error_code override;
        auto stat(const std::filesystem::path& file, struct stat& st) noexcept -> std::error_code override;
        auto link(const std::filesystem::path& existing, const std::filesystem::path& newlink) noexcept -> std::error_code override;
        auto symlink(const std::filesystem::path& existing, const std::filesystem::path& newlink) noexcept -> std::error_code override;
        auto unlink(const std::filesystem::path& name) noexcept -> std::error_code override;
        auto rmdir(const std::filesystem::path& name) noexcept -> std::error_code override;
        auto rename(const std::filesystem::path& oldname, const std::filesystem::path& newname) noexcept -> std::error_code override;
        auto mkdir(const std::filesystem::path& path, int mode) noexcept -> std::error_code override;

        /** Directory support API */
        auto diropen(const std::filesystem::path& path) noexcept -> result<std::unique_ptr<DirectoryHandle>> override;
        auto dirreset(DirectoryHandle& handle) noexcept -> std::error_code override;
        auto dirnext(DirectoryHandle& handle, std::filesystem::path& filename, struct stat& filestat) -> std::error_code override;
        auto dirclose(DirectoryHandle& handle) noexcept -> std::error_code override;

        /** Other fops API */
        auto ftruncate(FileHandle& handle, off_t len) noexcept -> std::error_code override;
        auto fsync(FileHandle& handle) noexcept -> std::error_code override;

        auto chmod(const std::filesystem::path& path, mode_t mode) noexcept -> std::error_code override;
        auto fchmod(FileHandle& handle, mode_t mode) noexcept -> std::error_code override;

        auto get_label() noexcept -> result<std::string> override;

    private:
        auto _stat(const std::filesystem::path& path, struct stat* st) noexcept -> std::error_code;

        BlockDevice&  m_blockdev;
        Flags         m_flags;
        lwext4_handle m_handle;
        std::string   m_root;
    };

    class filesystem_factory_lwext4 final : public FilesystemFactory {
    public:
        std::unique_ptr<Filesystem> create_filesystem(BlockDevice& bdev, Flags flags) override;
    };

    class file_handle_lwext4 final : public FileHandle, public RawHandle<ext4_file> {
    public:
        using FileHandle::FileHandle;
    };

    class directory_handle_lwext4 final : public DirectoryHandle, public RawHandle<ext4_dir> {
    public:
        using DirectoryHandle::DirectoryHandle;
    };

} // namespace vfs
