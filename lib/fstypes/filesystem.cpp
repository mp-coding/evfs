#include "api/vfs/filesystem.hpp"
#include <cerrno>
#include <utility>

namespace vfs {

    auto Filesystem::stat_vfs(const std::filesystem::path&, struct statvfs&) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::lseek(FileHandle&, off_t, int) noexcept -> result<off_t> { return error(ENOTSUP); }
    auto Filesystem::fstat(FileHandle&, struct stat&) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::stat(const std::filesystem::path&, struct stat&) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::link(const std::filesystem::path&, const std::filesystem::path&) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::symlink(const std::filesystem::path&, const std::filesystem::path&) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::unlink(const std::filesystem::path&) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::rmdir(const std::filesystem::path&) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::rename(const std::filesystem::path&, const std::filesystem::path&) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::mkdir(const std::filesystem::path&, int) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::diropen(const std::filesystem::path&) noexcept -> result<std::unique_ptr<DirectoryHandle>> { return error(ENOTSUP); }
    auto Filesystem::dirreset(DirectoryHandle&) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::dirnext(DirectoryHandle&, std::filesystem::path&, struct stat&) -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::dirclose(DirectoryHandle&) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::ftruncate(FileHandle&, off_t) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::fsync(FileHandle&) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::ioctl(const std::filesystem::path&, int, void*) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::utimens(const std::filesystem::path&, std::array<timespec, 2>&) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::flock(FileHandle&, int) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::isatty(FileHandle&) noexcept -> result<bool> { return error(ENOTSUP); }
    auto Filesystem::chmod(const std::filesystem::path&, mode_t) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::fchmod(FileHandle&, mode_t) noexcept -> std::error_code { return from_errno(ENOTSUP); }
    auto Filesystem::get_label() noexcept -> result<std::string> { return error(ENOTSUP); }

    FileHandle::FileHandle(std::string root, std::filesystem::path abspath)
        : abspath(std::move(abspath))
        , root(std::move(root))
    {
    }
    DirectoryHandle::DirectoryHandle(std::string root)
        : root(std::move(root))
    {
    }
    std::filesystem::path DirectoryHandle::get_root() const noexcept { return root; }
    std::filesystem::path FileHandle::get_path() const noexcept { return abspath; }
    std::filesystem::path FileHandle::get_root() const noexcept { return root; }
} // namespace vfs
