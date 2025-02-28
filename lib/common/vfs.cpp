/*
 * vfs.hpp
 * Created on: 14/02/2025
 * Author: Mateusz Piesta (mateusz.piesta@gmail.com)
 * Company: mprogramming
 */

#include "api/vfs/vfs.hpp"

#include <api/vfs/fstypes.hpp>
#include "api/vfs/disk.hpp"

#include "locker.hpp"
#include "file_descriptor_container.hpp"
#include "fstypes/filesystem_lwext4.hpp"
#include "logger/log.hpp"

#include <utility>
#include <unordered_map>
#include <sys/fcntl.h>
#include <sys/statvfs.h>

namespace vfs {

    struct MountPoint {
        std::unique_ptr<Filesystem> fs;
        BlockDevice&                disk;
        std::string                 root;
        Flags                       flags;
        fstype::Type                type;
    };

    std::unique_ptr<FilesystemFactory> get_fs_factory(const fstype::Type& type)
    {
        if (type == fstype::linux) { return std::make_unique<filesystem_factory_lwext4>(); }
        if (type == fstype::fat) { return {}; /* TODO  */ }
        return {};
    }

    auto get_mount_point_stats(const MountPoint& mnt) noexcept -> PartitionStats
    {
        struct statvfs stat_vfs {};
        std::ignore = mnt.fs->stat_vfs(mnt.root.c_str(), stat_vfs);

        PartitionStats stat {};
        stat.mount_point = mnt.root;
        stat.flags       = mnt.flags;
        stat.disk_name   = mnt.disk.get_name();
        stat.type        = mnt.type.name;
        stat.free_space  = stat_vfs.f_bfree * stat_vfs.f_bsize;
        stat.used_space  = (stat_vfs.f_blocks * stat_vfs.f_frsize) - stat.free_space;

        return stat;
    }

    struct VirtualFS::Pimpl {
        using LockedMountPoint = locker<MountPoint, Mutex>;

        explicit Pimpl(DiskManager& mngr)
            : m_disk_mgr(mngr)
        {
        }

        auto find_mount_point(const std::string& path) noexcept -> std::optional<std::reference_wrapper<LockedMountPoint>>
        {
            for (auto& [key, locker] : m_mounts) {
                if (path.find(key) != std::string::npos) { return locker; }
            }
            return {};
        }
        auto absolute_path(const std::filesystem::path& path) const noexcept -> result<std::filesystem::path>
        {
            if (not path.has_root_path()) {
                log_warning("Only absolute paths are supported");
                return error(EINVAL);
            }

            return path.lexically_normal();
        }

        std::optional<fstype::Type> get_fs_type_from_mbr_code(const std::uint8_t mbr_code)
        {
            const auto result = std::find_if(m_fs_factories.begin(), m_fs_factories.end(),
                [mbr_code](const auto& e) { return std::find_if(e.first.codes.begin(), e.first.codes.end(), [mbr_code](const auto& v) { return mbr_code == v; }) != e.first.codes.end(); });
            return result != m_fs_factories.end() ? result->first : std::optional<fstype::Type> {};
        }

        std::pair<std::optional<std::string>, std::optional<std::uint8_t>> split_disk_name(std::string_view disk)
        {
            const auto p = disk.find('p');
            if (p == std::string::npos) { return {std::string {disk}, std::nullopt}; }
            return {std::string {disk.substr(0, p)}, std::stoi(std::string {disk.substr(p + 1)})};
        }

        result<std::pair<fstype::Type, BlockDevice*>> fs_discovery(std::string_view disk_name)
        {
            const auto [diskn, partn] = split_disk_name(disk_name);
            if (not diskn) { return error(EINVAL); }

            const auto disk = m_disk_mgr.get(*diskn);
            if (disk == nullptr) { return error(EINVAL); }

            if (partn) {
                auto partition = disk->borrow_partition(*partn);
                if (partition == nullptr) { return error(EINVAL); }
                const auto type = get_fs_type_from_mbr_code(partition->get_info().type);
                if (not type) {
                    log_warning("Unsupported filesystem type: 0x%x", partition->get_info().type);
                    return error(EINVAL);
                }
                return std::pair {*type, partition};
            }

            /// TODO: filesystem discovery based on raw disk contents
            return error(EINVAL);
        }

        template <typename ErrT> ErrT terror(const int err)
        {
            if constexpr (std::is_same_v<ErrT, std::error_code>) {
                return from_errno(err);
            } else {
                return error(err);
            }
        }
        template <typename Class, typename Method, typename... Args> auto invoke_fops(Method Class::* method, const int fd, Args&&... args) -> decltype(auto)
        {
            using Ret = std::invoke_result_t<decltype(method), Class, FileHandle&, Args...>;

            m_mutex.lock();
            if (not m_fd_container.exist(fd)) {
                m_mutex.unlock();
                return terror<Ret>(EBADF);
            }
            auto& fil = m_fd_container[fd];
            m_mutex.unlock();

            const auto locked = m_mounts.at(fil.get_root()).lock();

            return (locked.get().fs.get()->*method)(fil, std::forward<Args>(args)...);
        }

        template <typename Class, typename Method, typename... Args> auto invoke_fops(Method Class::* method, const std::filesystem::path& path, Args&&... args) -> decltype(auto)
        {
            if (path.empty()) { return from_errno(ENOENT); }

            const auto abspath = absolute_path(path);
            if (not abspath) { return from_errno(abspath.error().value()); }
            const auto mount = find_mount_point(*abspath);
            if (not mount) { return from_errno(ENOENT); }
            const auto& locked = mount->get().lock();

            return (locked.get().fs.get()->*method)(*abspath, std::forward<Args>(args)...);
        }

        template <typename Class, typename Method, typename... Args> auto invoke_dirops(Method Class::* method, DirectoryHandle& handle, Args&&... args) -> std::error_code
        {
            const auto mp = m_mounts.at(handle.get_root()).lock();
            return (mp.get().fs.get()->*method)(handle, std::forward<Args>(args)...);
        }

        template <class Base, class T, typename... Args>
        auto invoke_fops_same_mp(T Base::* method, const std::filesystem::path& path, const std::filesystem::path& path2, Args&&... args) -> std::error_code
        {
            if (path.empty() || path2.empty()) { return from_errno(ENOENT); }

            const auto abspath = absolute_path(path);
            if (not abspath) { return abspath.error(); }
            const auto abspath2 = absolute_path(path2);
            if (not abspath2) { return abspath2.error(); }

            if (abspath->root_path() != abspath2->root_path()) {
                // Mount points are not the same
                return from_errno(EXDEV);
            }

            const auto mount = find_mount_point(*abspath);
            if (not mount) { return from_errno(ENOENT); }
            const auto& locked = mount->get().lock();

            if (locked.get().flags.test(MountFlags::read_only)) { return from_errno(EACCES); }

            return (locked.get().fs.get()->*method)(*abspath, *abspath2, std::forward<Args>(args)...);
        }
        auto cleanup_opened_files() -> void
        {
            for (const auto& [path, entry] : m_fd_container) {
                for (const auto& fd : entry) { invoke_fops(&Filesystem::close, fd); }
                if (entry.is_marked_for_unlink()) { invoke_fops(&Filesystem::unlink, path); }
            }
        }

        auto generate_unique_root_dir() -> std::string { return root_base + std::to_string(m_volume_index++); }

        DiskManager&                                                         m_disk_mgr;
        std::unordered_map<fstype::Type, std::unique_ptr<FilesystemFactory>> m_fs_factories;
        std::unordered_map<std::string, LockedMountPoint>                    m_mounts;
        mutable Mutex                                                        m_mutex;
        file_descriptor_container                                            m_fd_container;
        std::uint32_t                                                        m_volume_index {};
    };

    VirtualFS::VirtualFS(DiskManager& dmngr)
        : pimpl(std::make_unique<Pimpl>(dmngr))
    {
    }
    VirtualFS::~VirtualFS()
    {
        pimpl->cleanup_opened_files();
        unmount();
    }

    auto VirtualFS::register_filesystem(const fstype::Type& type) const -> std::error_code
    {
        auto_lock lock {pimpl->m_mutex};
        return pimpl->m_fs_factories.emplace(type, get_fs_factory(type)).second ? std::error_code {} : from_errno(EEXIST);
    }
    auto VirtualFS::register_filesystem(const fstype::Type& type, std::unique_ptr<FilesystemFactory> factory) const -> std::error_code
    {
        auto_lock lock {pimpl->m_mutex};
        return pimpl->m_fs_factories.emplace(type, std::move(factory)).second ? std::error_code {} : from_errno(EEXIST);
    }
    auto VirtualFS::unregister_filesystem(const fstype::Type& type) const -> std::error_code
    {
        auto_lock lock {pimpl->m_mutex};
        return pimpl->m_fs_factories.erase(type) != 0 ? std::error_code {} : from_errno(ENOENT);
    }

    std::error_code VirtualFS::mount()
    {
        auto_lock lock {pimpl->m_mutex};

        if (pimpl->m_disk_mgr.size() == 0) { return from_errno(ENOTBLK); }

        std::error_code ret;

        for (auto& [name, handle] : pimpl->m_disk_mgr) {
            log_info("Scanning disk '%s'...", name.c_str());

            std::for_each(handle->begin(), handle->end(), [this, &ret](const auto& p) {
                const auto result = mount(p.get_name(), {});
                /// Capture first occurence of mount error, but keep mounting the rest of partitions
                ret = not ret ? result : ret;
            });
        }
        return ret;
    }
    std::error_code VirtualFS::mount(std::string_view disk_name, std::string root, Flags flags)
    {
        auto_lock lock {pimpl->m_mutex};

        const auto result = pimpl->fs_discovery(disk_name);
        if (not result) { return result.error(); }
        const auto [type, disk] = *result;

        auto fs = pimpl->m_fs_factories.find(type)->second->create_filesystem(*disk, {});
        if (root.empty()) {
            if (const auto label = fs->get_label(); not label) {
                root = pimpl->generate_unique_root_dir();
                log_info("Disk '%s' label not available nor root directory specified, will be mounted as '%s'", disk->get_name().c_str(), root.c_str());
            } else {
                root = "/" + *label;
            }
        }

        if (pimpl->m_mounts.contains(root)) {
            log_error("Disk '%s' already mounted as '%s'", disk->get_name().c_str(), root.c_str());
            return from_errno(EEXIST);
        }

        if (const auto ret = fs->mount(root, flags)) {
            log_error("Failed to mount '%s' to '%s' with errno: %d", disk->get_name().c_str(), root.c_str(), ret);
            return ret;
        }

        log_info("Disk '%s' of type '%s' mounted successfully to '%s'", disk->get_name().c_str(), type.name.c_str(), root.c_str());

        if (const auto [_, inserted] = pimpl->m_mounts.emplace(root, Pimpl::LockedMountPoint {{std::move(fs), *disk, root, flags, type}}); not inserted) {
            log_error("Disk '%s' already mounted as '%s'", disk->get_name().c_str(), root.c_str());
            return from_errno(EEXIST);
        }

        return {};
    }

    std::error_code VirtualFS::unmount()
    {
        auto_lock       lock {pimpl->m_mutex};
        std::error_code ret;

        std::ignore = std::all_of(pimpl->m_mounts.begin(), pimpl->m_mounts.end(), [&ret](auto& m) {
            auto& locked = m.second.lock().get();

            if (const auto result = locked.fs->unmount()) {
                ret = result;
                return false;
            }
            return true;
        });
        pimpl->m_mounts.clear();
        pimpl->m_volume_index = 0;
        return ret;
    }

    std::error_code VirtualFS::unmount(std::string_view mount_point)
    {
        auto_lock lock {pimpl->m_mutex};

        if (const auto result = pimpl->m_mounts.find(std::string {mount_point}); result != pimpl->m_mounts.end()) {
            const auto& locked = result->second.lock();
            if (const auto ret = locked.get().fs->unmount()) { return ret; }

            pimpl->m_mounts.erase(result);
            return {};
        }
        return from_errno(EINVAL);
    }
    auto VirtualFS::get_roots() const -> std::vector<std::string>
    {
        auto_lock lock {pimpl->m_mutex};

        std::vector<std::string> mount_names;
        for (const auto& [root, _] : pimpl->m_mounts) { mount_names.emplace_back(root); }
        return mount_names;
    }
    auto VirtualFS::stat_parts() noexcept -> std::vector<PartitionStats>
    {
        std::vector<PartitionStats> stats;
        for (auto& [_, mp] : pimpl->m_mounts) { stats.emplace_back(get_mount_point_stats(mp.lock().get())); }
        return stats;
    }
    auto VirtualFS::stat_parts_of(const std::filesystem::path& path) noexcept -> result<PartitionStats>
    {
        auto mount = pimpl->find_mount_point(path);
        if (!mount) { return error(ENOENT); }
        const auto& locked = mount->get().lock();
        return get_mount_point_stats(locked.get());
    }

    auto VirtualFS::getcwd() noexcept -> std::filesystem::path { /* TODO */ return {}; }
    auto VirtualFS::chdir(const std::filesystem::path&) noexcept -> std::error_code { return from_errno(ENOTSUP); }

    auto VirtualFS::open(const std::filesystem::path& path, const int flags, const int mode) noexcept -> result<int>
    {
        auto_lock lock {pimpl->m_mutex};

        const auto abspath = pimpl->absolute_path(path);
        if (not abspath) { return error(abspath.error()); }

        const auto mount = pimpl->find_mount_point(*abspath);
        if (not mount) {
            log_error("Unable to find mount point for path: '%s'", abspath->c_str());
            return error(ENOENT);
        }

        const auto& locked = mount->get().lock();

        if ((flags & O_ACCMODE) != O_RDONLY && (locked.get().flags.test(MountFlags::read_only))) {
            log_error("Trying to open file with 'WR' flag on read-only filesystem");
            return error(EACCES);
        }

        auto handle = locked.get().fs->open(*abspath, flags, mode);
        if (not handle) { return error(handle.error()); }
        return pimpl->m_fd_container.insert(path, std::move(handle.value()));
    }

    auto VirtualFS::close(const int fd) noexcept -> std::error_code
    {
        auto_lock lock {pimpl->m_mutex};

        if (not pimpl->m_fd_container.exist(fd)) { return from_errno(EBADF); }

        const auto ret = pimpl->invoke_fops(&Filesystem::close, fd);
        if (const auto path = pimpl->m_fd_container.remove(fd)) { return pimpl->invoke_fops(&Filesystem::unlink, path.value()); }

        return ret;
    }

    auto VirtualFS::write(const int fd, const char* ptr, size_t len) noexcept -> result<std::size_t> { return pimpl->invoke_fops(&Filesystem::write, fd, ptr, len); }

    auto VirtualFS::read(const int fd, char* ptr, size_t len) noexcept -> result<std::size_t> { return pimpl->invoke_fops(&Filesystem::read, fd, ptr, len); }

    auto VirtualFS::lseek(const int fd, off_t pos, int dir) noexcept -> result<off_t> { return pimpl->invoke_fops(&Filesystem::lseek, fd, pos, dir); }

    auto VirtualFS::fstat(const int fd, struct stat& st) noexcept -> std::error_code { return pimpl->invoke_fops(&Filesystem::fstat, fd, st); }

    auto VirtualFS::ftruncate(const int fd, off_t len) noexcept -> std::error_code { return pimpl->invoke_fops(&Filesystem::ftruncate, fd, len); }

    auto VirtualFS::fsync(const int fd) noexcept -> std::error_code { return pimpl->invoke_fops(&Filesystem::fsync, fd); }

    auto VirtualFS::fchmod(const int fd, mode_t mode) noexcept -> std::error_code { return pimpl->invoke_fops(&Filesystem::fchmod, fd, mode); }

    auto VirtualFS::stat(const std::filesystem::path& path, struct stat& st) noexcept -> std::error_code { return pimpl->invoke_fops(&Filesystem::stat, path, st); }

    auto VirtualFS::symlink(const std::filesystem::path& existing, const std::filesystem::path& newlink) noexcept -> std::error_code
    {
        return pimpl->invoke_fops(&Filesystem::symlink, existing, newlink);
    }

    auto VirtualFS::link(const std::filesystem::path& existing, const std::filesystem::path& newlink) noexcept -> std::error_code { return pimpl->invoke_fops(&Filesystem::link, existing, newlink); }

    auto VirtualFS::unlink(const std::filesystem::path& name) noexcept -> std::error_code
    {
        auto_lock lock {pimpl->m_mutex};
        if (pimpl->m_fd_container.exist(name)) {
            pimpl->m_fd_container[name].mark_for_unlink();
            return {};
        }
        return pimpl->invoke_fops(&Filesystem::unlink, name);
    }

    auto VirtualFS::rename(const std::filesystem::path& oldname, const std::filesystem::path& newname) noexcept -> std::error_code
    {
        return pimpl->invoke_fops_same_mp(&Filesystem::rename, oldname, newname);
    }

    auto VirtualFS::diropen(const std::filesystem::path& path) noexcept -> result<std::unique_ptr<DirectoryHandle>>
    {
        const auto abspath = pimpl->absolute_path(path);
        if (not abspath) { return error(abspath.error()); }
        const auto mount = pimpl->find_mount_point(*abspath);
        if (!mount) {
            log_error("Unable to find mount point for path: '%s'", abspath->c_str());
            return error(ENOENT);
        }
        const auto& locked = mount->get().lock();
        return locked.get().fs->diropen(*abspath);
    }

    auto VirtualFS::dirreset(DirectoryHandle& handle) noexcept -> std::error_code { return pimpl->invoke_dirops(&Filesystem::dirreset, handle); }

    auto VirtualFS::dirnext(DirectoryHandle& handle, std::filesystem::path& filename, struct stat& filestat) noexcept -> std::error_code
    {
        return pimpl->invoke_dirops(&Filesystem::dirnext, handle, filename, filestat);
    }

    auto VirtualFS::dirclose(DirectoryHandle& handle) noexcept -> std::error_code { return pimpl->invoke_dirops(&Filesystem::dirclose, handle); }

    auto VirtualFS::mkdir(const std::filesystem::path& path, int mode) noexcept -> std::error_code { return pimpl->invoke_fops(&Filesystem::mkdir, path, mode); }

    auto VirtualFS::rmdir(const std::filesystem::path& path) noexcept -> std::error_code { return pimpl->invoke_fops(&Filesystem::rmdir, path); }

    auto VirtualFS::stat_vfs(const std::filesystem::path& path, struct statvfs& stat) noexcept -> std::error_code { return pimpl->invoke_fops(&Filesystem::stat_vfs, path, stat); }

    auto VirtualFS::chmod(const std::filesystem::path& path, mode_t mode) noexcept -> std::error_code { return pimpl->invoke_fops(&Filesystem::chmod, path, mode); }
    auto VirtualFS::ioctl(const std::filesystem::path& path, int cmd, void* arg) noexcept -> std::error_code { return pimpl->invoke_fops(&Filesystem::ioctl, path, cmd, arg); }
    auto VirtualFS::utimens(const std::filesystem::path& path, std::array<timespec, 2>& tv) noexcept -> std::error_code { return pimpl->invoke_fops(&Filesystem::utimens, path, tv); }
    auto VirtualFS::flock(const int fd, int cmd) noexcept -> std::error_code { return pimpl->invoke_fops(&Filesystem::flock, fd, cmd); }
    auto VirtualFS::isatty(const int fd) noexcept -> std::error_code { return pimpl->invoke_fops(&Filesystem::isatty, fd); }

} // namespace vfs
