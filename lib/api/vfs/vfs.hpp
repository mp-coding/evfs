/*
 * vfs.hpp
 * Created on: 14/02/2025
 * Author: Mateusz Piesta (mateusz.piesta@gmail.com)
 * Company: mprogramming
 */

#pragma once

#include "defs.hpp"
#include "partition_stats.hpp"
#include "disk_mngr.hpp"
#include "filesystem.hpp"
#include "fstypes.hpp"

#include <string>
#include <array>
#include <filesystem>
#include <vector>

struct statvfs;
struct stat;

namespace vfs {

    class DirectoryHandle;
    class StdStream;

    class VirtualFS {
    public:
        VirtualFS(DiskManager& dmngr, std::unique_ptr<StdStream>&& stream);
        ~VirtualFS();
        VirtualFS(const VirtualFS&)      = delete;
        auto operator=(const VirtualFS&) = delete;

        /**
         * Register a filesystem.
         * @param type
         * @return 0 in case of success otherwise, an error code
         */
        [[nodiscard]] auto register_filesystem(const fstype::Type& type) const -> std::error_code;

        /**
         * Register a filesystem.
         * @param type
         * @param factory
         * @return 0 in case of success otherwise, an error code
         */
        [[nodiscard]] auto register_filesystem(const fstype::Type& type, std::unique_ptr<FilesystemFactory> factory) const -> std::error_code;

        /**
         * Unregister a filesystem
         * @param type filesystem type encoded as string, i.e. partition_name::linux, partition_name::vfat and so on
         * @return 0 in case of success otherwise, an error code
         */
        [[nodiscard]] auto unregister_filesystem(const fstype::Type& type) const -> std::error_code;

        /**
         * Mount all available partitions within registered blockdevices automatically. Root directories will be filled automatically based on partition's label
         * or predefined prefix(if label is not available). It tries to mount all available partitions from all registered blok devices even if, during the
         * process, some can't be mounted. In that case, it tries to mount a next partition from the list.
         * @return 0 in case of success otherwise, an error code
         */
        std::error_code mount_all();

        /**
         * Mount a specific partition
         * @param disk_name disk, i.e. 'disk0p0'
         * @param root where to mount partition. It's possible to pass empty string as root. In this case, VFS will create unique root directory based either on
         * partition's label or predefined prefix, e.g. '/volumeX' where X is a unique number.
         * @param fstype filesystem type(e.g., 'ext4', 'ext3','vfat'). Pass empty string to detect type automatically.
         * @param flags optional mount flags
         * @return 0 in case of success otherwise, an error code
         */
        std::error_code mount(std::string_view disk_name, std::string root, std::string fstype, Flags flags = 0);

        /**
         * Un-mount all partitions
         * @return 0 in case of success otherwise, an error code
         */
        std::error_code umount_all();

        /**
         * Un-mount specified partition by its mount point directory
         * @param mount_point directory where a partition is mounted
         * @return 0 in case of success otherwise, an error code
         */
        std::error_code umount(std::string_view mount_point);

        /**
         * Fetch list of root paths associated with mount points
         * @return list of roots
         */
        [[nodiscard]] auto get_roots() const -> std::vector<std::string>;

        /**
         * Get information about currently mounted partitions
         * @return list of partition stats
         */
        auto stat_parts() noexcept -> std::vector<PartitionStats>;

        /**
         * Get information about corresponding partition for given path
         * @return partition stats
         */
        auto stat_parts_of(const std::filesystem::path& path) noexcept -> result<PartitionStats>;

        /** Standard file access API */
        auto open(const std::filesystem::path& path, int flags, int mode) noexcept -> result<int>;
        auto close(int fd) noexcept -> std::error_code;
        auto write(int fd, const char* ptr, size_t len) noexcept -> result<std::size_t>;
        auto read(int fd, char* ptr, size_t len) noexcept -> result<std::size_t>;
        auto lseek(int fd, off_t pos, int dir) noexcept -> result<off_t>;
        auto fstat(int fd, struct stat& st) noexcept -> std::error_code;
        auto stat(const std::filesystem::path& path, struct stat& st) noexcept -> std::error_code;
        auto link(const std::filesystem::path& existing, const std::filesystem::path& newlink) noexcept -> std::error_code;
        auto symlink(const std::filesystem::path& existing, const std::filesystem::path& newlink) noexcept -> std::error_code;
        auto unlink(const std::filesystem::path& name) noexcept -> std::error_code;
        auto rename(const std::filesystem::path& oldname, const std::filesystem::path& newname) noexcept -> std::error_code;
        auto mkdir(const std::filesystem::path& path, int mode) noexcept -> std::error_code;
        auto rmdir(const std::filesystem::path& path) noexcept -> std::error_code;

        /** Directory support API */
        auto diropen(const std::filesystem::path& path) noexcept -> result<std::unique_ptr<DirectoryHandle>>;
        auto dirreset(DirectoryHandle& handle) noexcept -> std::error_code;
        auto dirnext(DirectoryHandle& handle, std::filesystem::path& filename, struct stat& filestat) noexcept -> std::error_code;
        auto dirclose(DirectoryHandle& handle) noexcept -> std::error_code;

        /** Other fops API */
        auto ftruncate(int fd, off_t len) noexcept -> std::error_code;
        auto fsync(int fd) noexcept -> std::error_code;
        auto ioctl(const std::filesystem::path& path, int cmd, void* arg) noexcept -> std::error_code;
        auto utimens(const std::filesystem::path& path, std::array<timespec, 2>& tv) noexcept -> std::error_code;
        auto flock(int fd, int cmd) noexcept -> std::error_code;
        auto isatty(int fd) noexcept -> result<bool>;

        auto chmod(const std::filesystem::path& path, mode_t mode) noexcept -> std::error_code;
        auto fchmod(int fd, mode_t mode) noexcept -> std::error_code;

        auto getcwd() noexcept -> std::filesystem::path;
        auto chdir(const std::filesystem::path& name) noexcept -> std::error_code;

        auto stat_vfs(const std::filesystem::path& path, struct statvfs& stat) noexcept -> std::error_code;

    private:
        struct Pimpl;
        std::unique_ptr<Pimpl> pimpl;
    };
} // namespace vfs
