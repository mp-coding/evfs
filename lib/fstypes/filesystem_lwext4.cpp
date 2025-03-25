#include <memory>
#include "filesystem_lwext4.hpp"
#include "api/vfs/blockdev.hpp"
#include "api/vfs/fstypes.hpp"
#include "logger/log.hpp"

#include <ext4.h>
#include <ext4_inode.h>
#include <ext4_super.h>
#include <ext4_mkfs.h>

#include <sys/statvfs.h>
#include <sys/stat.h>
#include <climits>
#include <cerrno>
#include <cstring>

namespace vfs {
    namespace {

        constexpr file_handle_lwext4&      from(FileHandle& handle) { return static_cast<file_handle_lwext4&>(handle); }
        constexpr directory_handle_lwext4& from(DirectoryHandle& handle) { return static_cast<directory_handle_lwext4&>(handle); }
        constexpr std::string              to_native_path(const std::string& root, const std::string& path)
        {
            if (path == root) { return path + "/"; }
            return path;
        }
        constexpr std::string to_native_path(const std::string& root) { return root + "/"; }

        template <typename T, typename... Args> auto invoke_fs(FileHandle& handle, T efs_fun, Args&&... args) { return from_errno(efs_fun(&from(handle).get_raw(), std::forward<Args>(args)...)); }

        template <typename T, typename... Args> auto invoke_fs(T efs_fun, const std::filesystem::path& path, Args&&... args) { return from_errno(efs_fun(path.c_str(), std::forward<Args>(args)...)); }

        template <typename T, typename... Args> auto invoke_fs(DirectoryHandle& handle, T efs_fun, Args&&... args) { return from_errno(efs_fun(&from(handle).get_raw(), std::forward<Args>(args)...)); }

        constexpr auto ino_to_st_mode(const int dtype)
        {
            switch (dtype) {
            case EXT4_DE_REG_FILE:
                return S_IFREG;
            case EXT4_DE_DIR:
                return S_IFDIR;
            case EXT4_DE_CHRDEV:
                return S_IFCHR;
            case EXT4_DE_BLKDEV:
                return S_IFBLK;
            case EXT4_DE_FIFO:
                return S_IFIFO;
            case EXT4_DE_SOCK:
                return S_IFSOCK;
            default:
                return 0;
            }
        }

        std::time_t get_posix_time()
        {
            const auto time = std::time(nullptr);
            return time == std::time_t {-1} ? 0 : time;
        }
    } // namespace

    filesystem_lwext4::filesystem_lwext4(BlockDevice& bdev, Flags flags)
        : m_blockdev(bdev)
        , m_flags(flags)
        , m_handle(m_blockdev)
    {
    }
    auto filesystem_lwext4::mount(std::string root, const Flags flags) noexcept -> std::error_code
    {
        m_root = root;
        root   = to_native_path(root);

        auto err = ext4_device_register(&m_handle.get_blockdev(), m_blockdev.get_name().c_str());
        if (err) {
            log_error("Unable to register device with err: %i", err);
            return from_errno(err);
        }

        err = ext4_mount(m_blockdev.get_name().c_str(), root.c_str(), flags.test(MountFlags::read_only));
        if (err) {
            log_error("Unable to mount ext4 errno %i", err);
            ext4_device_unregister(m_blockdev.get_name().c_str());
            return from_errno(err);
        }

        err = ext4_recover(root.c_str());
        if (err) {
            log_error("Ext4 recover failed errno %i", err);
            ext4_umount(root.c_str());
            ext4_device_unregister(m_blockdev.get_name().c_str());
            return from_errno(err);
        }

        err = ext4_journal_start(root.c_str());
        if (err) {
            log_error("Unable to start journaling errno %i", err);
            ext4_umount(root.c_str());
            ext4_device_unregister(m_blockdev.get_name().c_str());
            return from_errno(err);
        }

        return {};
    }

    auto filesystem_lwext4::unmount() noexcept -> std::error_code
    {
        const auto native_root = to_native_path(m_root);

        auto err = ext4_journal_stop(native_root.c_str());
        if (err) {
            log_warning("Unable to stop ext4 journal %i", err);
            err = 0;
        }
        err = ext4_umount(native_root.c_str());
        if (err) {
            log_error("Unable to unmount device");
            return from_errno(err);
        }

        return from_errno(ext4_device_unregister(m_blockdev.get_name().c_str()));
    }

    // Stat the filesystem
    auto filesystem_lwext4::stat_vfs([[maybe_unused]] const std::filesystem::path& path, struct statvfs& stat) noexcept -> std::error_code
    {
        const auto native_root = to_native_path(m_root);

        ext4_mount_stats estats {};
        auto             err = ext4_mount_point_stats(native_root.c_str(), &estats);
        if (err) {
            log_error("Mount point stats error %i", err);
            return from_errno(err);
        }
        std::memset(&stat, 0, sizeof stat);
        stat.f_bsize   = estats.block_size;
        stat.f_frsize  = estats.block_size;
        stat.f_blocks  = estats.blocks_count;
        stat.f_bfree   = estats.free_blocks_count;
        stat.f_bavail  = stat.f_bfree;
        stat.f_files   = estats.inodes_count;
        stat.f_ffree   = estats.free_inodes_count;
        stat.f_favail  = stat.f_ffree;
        stat.f_flag    = m_flags.to_ullong();
        stat.f_namemax = NAME_MAX;
        return {};
    }

    auto filesystem_lwext4::open(const std::filesystem::path& abspath, const Flags flags, [[maybe_unused]] const int mode) noexcept -> result<std::unique_ptr<FileHandle>>
    {
        auto       handle = std::make_unique<file_handle_lwext4>(m_root, abspath);
        const auto err    = ext4_fopen2(&handle->get_raw(), abspath.c_str(), static_cast<int>(flags.to_ullong()));
        if (err == EOK) {
            ext4_atime_set(abspath.c_str(), get_posix_time());
            return handle;
        }
        return error(err);
    }

    auto filesystem_lwext4::close(FileHandle& handle) noexcept -> std::error_code
    {
        const auto err = invoke_fs(handle, ::ext4_fclose);
        if (not err) { return from_errno(ext4_mtime_set(handle.get_path().c_str(), get_posix_time())); }
        return err;
    }

    auto filesystem_lwext4::write(FileHandle& handle, const char* ptr, size_t len) noexcept -> result<std::size_t>
    {
        std::size_t n_written {};
        if (const auto err = invoke_fs(handle, ::ext4_fwrite, ptr, len, &n_written)) { return error(err); }
        return n_written;
    }

    auto filesystem_lwext4::read(FileHandle& handle, char* ptr, size_t len) noexcept -> result<std::size_t>
    {
        size_t n_read {};
        if (const auto err = invoke_fs(handle, ::ext4_fread, ptr, len, &n_read)) { return error(err); }
        return n_read;
    }

    auto filesystem_lwext4::lseek(FileHandle& handle, off_t pos, int dir) noexcept -> result<off_t>
    {
        auto& nhandle = from(handle);
        if (const auto ret = ext4_fseek(&nhandle.get_raw(), pos, dir); ret != 0) { return error(ret); }
        return ext4_ftell(&nhandle.get_raw());
    }

    auto filesystem_lwext4::_stat(const std::filesystem::path& path, struct stat* st) noexcept -> std::error_code
    {
        uint32_t     inonum;
        ext4_inode   ino;
        ext4_sblock* sb;
        auto         err = ext4_raw_inode_fill(to_native_path(m_root, path).c_str(), &inonum, &ino);
        if (err) { return from_errno(err); }
        err = ext4_get_sblock(to_native_path(m_root).c_str(), &sb);
        if (err) { return from_errno(err); }
        std::memset(st, 0, sizeof(*st));
        st->st_ino       = inonum;
        const auto btype = ext4_inode_type(sb, &ino);
        st->st_mode      = ext4_inode_get_mode(sb, &ino) | ino_to_st_mode(btype);
        // Update file type
        st->st_nlink   = ext4_inode_get_links_cnt(&ino);
        st->st_uid     = ext4_inode_get_uid(&ino);
        st->st_gid     = ext4_inode_get_gid(&ino);
        st->st_blocks  = ext4_inode_get_blocks_count(sb, &ino);
        st->st_size    = ext4_inode_get_size(sb, &ino);
        st->st_blksize = ext4_sb_get_block_size(sb);
        st->st_dev     = ext4_inode_get_dev(&ino);
        st->st_atime   = ext4_inode_get_access_time(&ino);
        st->st_ctime   = ext4_inode_get_change_inode_time(&ino);
        st->st_mtime   = ext4_inode_get_modif_time(&ino);
        return {};
    }

    auto filesystem_lwext4::fstat(FileHandle& handle, struct stat& st) noexcept -> std::error_code
    {
        auto& nhandle = from(handle);
        return _stat(nhandle.get_path().c_str(), &st);
    }

    auto filesystem_lwext4::stat(const std::filesystem::path& path, struct stat& st) noexcept -> std::error_code { return _stat(path, &st); }

    auto filesystem_lwext4::link(const std::filesystem::path& existing, const std::filesystem::path& newlink) noexcept -> std::error_code
    {
        return invoke_fs(::ext4_flink, existing.c_str(), newlink.c_str());
    }

    auto filesystem_lwext4::symlink(const std::filesystem::path& existing, const std::filesystem::path& newlink) noexcept -> std::error_code
    {
        return invoke_fs(::ext4_fsymlink, existing.c_str(), newlink.c_str());
    }

    auto filesystem_lwext4::unlink(const std::filesystem::path& name) noexcept -> std::error_code
    {
        if (ext4_inode_exist(name.c_str(), EXT4_DE_DIR) == 0) {
            log_warning("rmdir syscall instead of unlink is recommended for remove directory");
            return from_errno(ext4_dir_rm(name.c_str()));
        }
        return from_errno(ext4_fremove(name.c_str()));
    }

    auto filesystem_lwext4::rmdir(const std::filesystem::path& name) noexcept -> std::error_code { return from_errno(ext4_dir_rm(name.c_str())); }

    auto filesystem_lwext4::rename(const std::filesystem::path& oldname, const std::filesystem::path& newname) noexcept -> std::error_code
    {
        return invoke_fs(::ext4_frename, oldname.c_str(), newname.c_str());
    }

    auto filesystem_lwext4::mkdir(const std::filesystem::path& path, [[maybe_unused]] int mode) noexcept -> std::error_code { return from_errno(ext4_dir_mk(path.c_str())); }

    auto filesystem_lwext4::diropen(const std::filesystem::path& path) noexcept -> result<std::unique_ptr<DirectoryHandle>>
    {
        auto dirp = std::make_unique<directory_handle_lwext4>(m_root);

        const auto ret = ext4_dir_open(&dirp->get_raw(), to_native_path(m_root, path).c_str());
        if (ret == 0) { return dirp; }
        return error(ret);
    }

    auto filesystem_lwext4::dirreset(DirectoryHandle& handle) noexcept -> std::error_code
    {
        ext4_dir_entry_rewind(&from(handle).get_raw());
        return {};
    }

    auto filesystem_lwext4::dirnext(DirectoryHandle& handle, std::filesystem::path& filename, struct stat& st) -> std::error_code
    {
        if (const auto dentry = ext4_dir_entry_next(&from(handle).get_raw())) {
            std::memset(&st, 0, sizeof(st));
            st.st_ino  = dentry->inode;
            st.st_mode = ino_to_st_mode(dentry->inode_type);
            filename   = std::string(reinterpret_cast<const char*>(dentry->name), dentry->name_length);
            return {};
        }
        return from_errno(ENOENT);
    }

    auto filesystem_lwext4::dirclose(DirectoryHandle& handle) noexcept -> std::error_code { return invoke_fs(handle, ::ext4_dir_close); }

    auto filesystem_lwext4::chmod(const std::filesystem::path& path, const mode_t mode) noexcept -> std::error_code
    {
        const auto err = ext4_mode_set(path.c_str(), mode);
        if (err == EOK) { ext4_mtime_set(path.c_str(), get_posix_time()); }
        return from_errno(err);
    }

    auto filesystem_lwext4::fchmod(FileHandle& handle, const mode_t mode) noexcept -> std::error_code
    {
        const auto err = ext4_mode_set(from(handle).get_path().c_str(), mode);
        if (err == EOK) { ext4_mtime_set(from(handle).get_path().c_str(), get_posix_time()); }
        return from_errno(err);
    }

    result<bool> filesystem_lwext4::isatty(FileHandle&) noexcept { return false; }

    auto filesystem_lwext4::ftruncate(FileHandle& handle, off_t len) noexcept -> std::error_code
    {
        return invoke_fs(
            handle,
            [](ext4_file* file, uint64_t length) {
                int err = ext4_ftruncate(file, length);
                if (err == ENOTSUP) {
                    // NOTE: Ext4 ftruncate supports only shrinking
                    const auto zbuf_len = 8192;
                    auto       buf      = std::make_unique<char[]>(zbuf_len);
                    err                 = 0;
                    for (size_t n = 0; n < length / zbuf_len; ++n) {
                        err = ext4_fwrite(file, buf.get(), zbuf_len, nullptr);
                        if (err) {
                            err = -err;
                            break;
                        }
                    }
                    if (!err) {
                        const ssize_t remain = length % zbuf_len;
                        if (remain > 0) { err = -ext4_fwrite(file, buf.get(), remain, nullptr); }
                    }
                }
                return err;
            },
            len);
    }

    auto filesystem_lwext4::fsync([[maybe_unused]] FileHandle& handle) noexcept -> std::error_code { return from_errno(ext4_cache_flush(to_native_path(m_root).c_str())); }

    auto filesystem_lwext4::get_label() noexcept -> result<std::string>
    {
        ext4_mkfs_info info {};

        if (const auto r = ext4_mkfs_read_info(&m_handle.get_blockdev(), &info); r != EOK) {
            log_error("Unable to read ext partition info with errno: %d\n", r);
            return error(r);
        }
        return info.label;
    }

    std::unique_ptr<Filesystem> filesystem_factory_lwext4::create_filesystem(BlockDevice& bdev, Flags flags) { return std::make_unique<filesystem_lwext4>(bdev, flags); }

} // namespace vfs
