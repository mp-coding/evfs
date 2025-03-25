#include "api/vfs/syscalls.hpp"

#include <vfs/vfs.hpp>

#include <filesystem>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <sys/dirent.h>

namespace vfs {
    class VirtualFS;
    /// Returns unique instance of vfs class that is available throughout the lifetime of the application
    /// It is responsibility of a user to provide the implementation(should be part of integration layer)
    VirtualFS& borrow_vfs();
} // namespace vfs

struct __dirstream {
    std::unique_ptr<vfs::DirectoryHandle> dirh;
    size_t                                position {};
    dirent                                dir_data {};
};

namespace {

    template <template <typename...> class Template, typename T> struct is_instance_of : std::false_type { };
    template <template <typename...> class Template, typename... Args> struct is_instance_of<Template, Template<Args...>> : std::true_type { };
    template <typename T> struct is_instance_of<vfs::result, std::expected<T, std::error_code>> : std::true_type { };
    template <template <typename...> class Template, typename T> inline constexpr bool is_instance_of_v = is_instance_of<Template, std::remove_cv_t<T>>::value;

    template <class Class, typename Method, typename... Args> auto invoke_fs(int& error, Method Class::* method, Args&&... args) -> decltype(auto)
    {
        auto&      vfs = vfs::borrow_vfs();
        const auto ret = (vfs.*method)(std::forward<Args>(args)...);
        if constexpr (is_instance_of_v<vfs::result, decltype(ret)>) {
            if (not ret) {
                error = ret.error().value();
                return int {-1};
            }
            error = 0;
            return static_cast<int>(ret.value());
        } else {
            error = ret.value();
        }

        return error ? -1 : static_cast<int>(ret.value());
    }

    int stmode_to_type(const mode_t mode)
    {
        if (S_ISREG(mode)) { return DT_REG; }
        if (S_ISDIR(mode)) { return DT_DIR; }
        if (S_ISCHR(mode)) { return DT_CHR; }
        if (S_ISBLK(mode)) { return DT_BLK; }
        if (S_ISFIFO(mode)) { return DT_FIFO; }
        if (S_ISSOCK(mode)) { return DT_SOCK; }
        return 0;
    }
} // namespace

namespace vfs::syscalls {
    int open(int& _errno_, const char* file, int flags, int mode) { return invoke_fs(_errno_, &VirtualFS::open, file, flags, mode); }

    int close(int& _errno_, int fd) { return invoke_fs(_errno_, &VirtualFS::close, fd); }

    ssize_t write(int& _errno_, int fd, const void* buf, size_t cnt) { return invoke_fs(_errno_, &VirtualFS::write, fd, static_cast<const char*>(buf), cnt); }

    ssize_t read(int& _errno_, int fd, void* buf, size_t cnt) { return invoke_fs(_errno_, &VirtualFS::read, fd, static_cast<char*>(buf), cnt); }

    off_t lseek(int& _errno_, int fd, off_t pos, int dir) { return invoke_fs(_errno_, &VirtualFS::lseek, fd, pos, dir); }

    int fstat(int& _errno_, int fd, struct stat* pstat)
    {
        if (pstat) { return invoke_fs(_errno_, &VirtualFS::fstat, fd, *pstat); }
        _errno_ = EINVAL;
        return -1;
    }

    int link(int& _errno_, const char* existing, const char* newLink) { return invoke_fs(_errno_, &VirtualFS::link, existing, newLink); }

    int unlink(int& _errno_, const char* name) { return invoke_fs(_errno_, &VirtualFS::unlink, name); }

    int rmdir(int& _errno_, const char* name) { return invoke_fs(_errno_, &VirtualFS::rmdir, name); }

    int fcntl(int& _errno_, [[maybe_unused]] int fd, [[maybe_unused]] int cmd, [[maybe_unused]] int arg)
    {
        _errno_ = ENOTSUP;
        return -1;
    }

    int stat(int& _errno_, const char* file, struct stat* pstat)
    {
        if (pstat) { return invoke_fs(_errno_, &VirtualFS::stat, file, *pstat); }
        _errno_ = EINVAL;
        return -1;
    }

    int chdir(int& _errno_, const char* path) { return invoke_fs(_errno_, &VirtualFS::chdir, path); }

    char* getcwd(int& _errno_, char* buf, size_t size)
    {
        if (!buf) {
            _errno_ = EINVAL;
            return nullptr;
        }
        auto&      vfs = borrow_vfs();
        const auto ret = vfs.getcwd();
        snprintf(buf, size, "%s", vfs.getcwd().c_str());
        return buf;
    }

    int rename(int& _errno_, const char* oldName, const char* newName) { return invoke_fs(_errno_, &VirtualFS::rename, oldName, newName); }

    int mkdir(int& _errno_, const char* path, mode_t mode) { return invoke_fs(_errno_, &VirtualFS::mkdir, path, mode); }

    DIR* opendir(int& _errno_, const char* dirname)
    {
        __dirstream* ret {};
        if (!dirname) {
            _errno_ = EIO;
            return ret;
        }
        auto& vfs = borrow_vfs();

        ret = new (std::nothrow) __dirstream;
        if (!ret) {
            _errno_ = ENOMEM;
            return ret;
        }
        ret->position = 0;
        auto handle   = vfs.diropen(dirname);
        if (not handle) {
            _errno_ = handle.error().value();
            delete ret;
            return nullptr;
        }
        ret->dirh = std::move(*handle);
        return ret;
    }

    int closedir(int& _errno_, DIR* dirp)
    {
        if (!dirp) {
            _errno_ = EBADF;
            return -1;
        }
        auto& vfs = borrow_vfs();

        const auto ret = vfs.dirclose(*dirp->dirh);
        if (ret) { _errno_ = ret.value(); }
        delete dirp;
        return ret ? -1 : 0;
    }

    struct dirent* readdir(int& _errno_, DIR* dirp)
    {
        if (!dirp) {
            _errno_ = EBADF;
            return nullptr;
        }
        auto&                 vfs = borrow_vfs();
        std::filesystem::path fname;
        struct stat           stdata {};
        const auto            ret = vfs.dirnext(*dirp->dirh, fname, stdata);
        if (ret and ret.value() != ENOENT) {
            _errno_ = ret.value();
            return nullptr;
        }
        if (ret.value() == ENOENT) { return nullptr; }

        if (fname.string().size() >= sizeof(dirp->dir_data.d_name)) {
            _errno_ = EOVERFLOW;
            return nullptr;
        }
        dirp->position += 1;
        dirp->dir_data.d_ino    = stdata.st_ino;
        dirp->dir_data.d_type   = stmode_to_type(stdata.st_mode);
        dirp->dir_data.d_reclen = fname.string().size();
        snprintf(dirp->dir_data.d_name, sizeof(dirp->dir_data.d_name), "%s", fname.c_str());
        return &dirp->dir_data;
    }

    int readdir_r(int& _errno_, DIR* dirp, struct dirent* entry, struct dirent** result)
    {
        if (!dirp) {
            _errno_ = EBADF;
            return -1;
        }
        if (!*result) {
            _errno_ = EINVAL;
            return -1;
        }
        auto&                 vfs = borrow_vfs();
        std::filesystem::path fname;
        struct stat           stdata {};
        const auto            ret = vfs.dirnext(*dirp->dirh, fname, stdata);

        if (ret and ret.value() != ENOENT) {
            _errno_ = ret.value();
            return -1;
        }

        if (ret.value() == ENOENT) {
            *result = nullptr;
            return 0;
        }

        if (fname.string().size() >= sizeof(dirp->dir_data.d_name)) {
            _errno_ = EOVERFLOW;
            return -1;
        }
        dirp->position += 1;
        entry->d_ino    = stdata.st_ino;
        entry->d_type   = stmode_to_type(stdata.st_mode);
        entry->d_reclen = fname.string().size();
        snprintf(entry->d_name, sizeof(entry->d_name), "%s", fname.c_str());
        *result = entry;
        return 0;
    }

    void rewinddir(int& _errno_, DIR* dirp)
    {
        if (!dirp) {
            _errno_ = EBADF;
            return;
        }
        auto& vfs = borrow_vfs();
        if (const auto res = vfs.dirreset(*dirp->dirh)) {
            _errno_ = res.value();
            return;
        }
        dirp->position = 0;
    }

    void seekdir(int& _errno_, DIR* dirp, const long int loc)
    {
        if (!dirp) {
            _errno_ = EBADF;
            return;
        }
        if (loc < 0) {
            _errno_ = EINVAL;
            return;
        }
        auto& vfs = borrow_vfs();
        if (static_cast<long>(dirp->position) > loc) {
            vfs.dirreset(*dirp->dirh);
            dirp->position = 0;
        }
        std::filesystem::path name;
        struct stat           st {};
        while ((static_cast<long>(dirp->position) < loc) && vfs.dirnext(*dirp->dirh, name, st).value() == 0) { dirp->position += 1; }
    }

    long int telldir(int& _errno_, DIR* dirp)
    {
        if (!dirp) {
            _errno_ = EBADF;
            return -1;
        }
        return dirp->position;
    }

    int chmod(int& _errno_, const char* path, mode_t mode) { return invoke_fs(_errno_, &VirtualFS::chmod, path, mode); }

    int fchmod(int& _errno_, int fd, mode_t mode) { return invoke_fs(_errno_, &VirtualFS::fchmod, fd, mode); }

    int fsync(int& _errno_, int fd) { return invoke_fs(_errno_, &VirtualFS::fsync, fd); }

    int statvfs(int& _errno_, const char* path, struct statvfs* buf)
    {
        if (!buf) {
            _errno_ = EINVAL;
            return -1;
        }
        return invoke_fs(_errno_, &VirtualFS::stat_vfs, path, *buf);
    }

    ssize_t readlink(int& _errno_, const char*, char*, size_t)
    {
        // Symlinks are not supported. According to man(2) symlink EINVAL when is not symlink.
        _errno_ = EINVAL;
        return -1;
    }

    int symlink(int& _errno_, const char* name1, const char* name2) { return invoke_fs(_errno_, &VirtualFS::symlink, name1, name2); }

    long fpathconf(int& _errno, int, int)
    {
        _errno = ENOTSUP;
        return -1;
    }

    long pathconf(int& _errno, const char*, int)
    {
        _errno = ENOTSUP;
        return -1;
    }

    int isatty(int& _errno_, int fd) { return invoke_fs(_errno_, &VirtualFS::isatty, fd); }

    int mount(int& _errno_, const char* dev, const char* dir, const char* fstype, unsigned long mountflags, const void*) { return invoke_fs(_errno_, &VirtualFS::mount, dev, dir, fstype, mountflags); }

    int umount(int& _errno_, const char* dev) { return invoke_fs(_errno_, &VirtualFS::umount, dev); }

} // namespace vfs::syscalls