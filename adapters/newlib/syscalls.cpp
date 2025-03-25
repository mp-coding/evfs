/**
 * syscalls
 * Created on: 25/03/2025
 * Author mateuszpiesta
 * Company: mprogramming
 */

#include "api/vfs/syscalls.hpp"

#include <cerrno>

namespace {
#ifndef _REENT

    struct _reent {
        int _errno;
    };
    _reent  reent;
    _reent* _REENT = &reent;
#endif

} // namespace

using namespace vfs;

extern "C" {
int     _open_r(_reent* r, const char* file, int flags, int mode) { return syscalls::open(r->_errno, file, flags, mode); }
int     _close_r(_reent* r, int fd) { return syscalls::close(r->_errno, fd); }
ssize_t _write_r(_reent* r, int fd, const void* buf, size_t cnt) { return syscalls::write(r->_errno, fd, buf, cnt); }
ssize_t _read_r(_reent* r, int fd, void* buf, size_t cnt) { return syscalls::read(r->_errno, fd, buf, cnt); }
off_t   _lseek_r(_reent* r, int fd, off_t pos, int dir) { return syscalls::lseek(r->_errno, fd, pos, dir); }
int     _fstat_r(_reent* r, int fd, struct stat* st) { return syscalls::fstat(r->_errno, fd, st); }
int     _stat_r(_reent* r, const char* file, struct stat* pstat) { return syscalls::stat(r->_errno, file, pstat); }
int     _isatty_r(_reent* r, int fd) { return syscalls::isatty(r->_errno, fd); }
int     _fcntl_r(_reent* r, int, int, int)
{
    r->_errno = ENOTSUP;
    return -1;
}

int _statvfs_r(_reent* r, const char* path, struct statvfs* buf)
{
    if (!buf) {
        r->_errno = EINVAL;
        return -1;
    }
    return syscalls::statvfs(r->_errno, path, buf);
}

int _fsync_r(_reent* r, int fd) { return syscalls::fsync(r->_errno, fd); }
int _link_r(_reent* r, const char* existing, const char* newLink) { return syscalls::link(r->_errno, existing, newLink); }
int _unlink_r(_reent* r, const char* name) { return syscalls::unlink(r->_errno, name); }
int _rmdir_r(_reent* r, const char* name) { return syscalls::rmdir(r->_errno, name); }
int _chdir_r(_reent* r, const char* path) { return syscalls::chdir(r->_errno, path); }
int _rename_r(_reent* r, const char* oldName, const char* newName) { return syscalls::rename(r->_errno, oldName, newName); }
int _mkdir_r(_reent* r, const char* path, int mode) { return syscalls::mkdir(r->_errno, path, mode); }

int     _open(const char* file, int flags, int mode) { return _open_r(_REENT, file, flags, mode); }
int     _close(int fd) { return _close_r(_REENT, fd); }
ssize_t _write(int fd, const void* buf, size_t cnt) { return _write_r(_REENT, fd, buf, cnt); }
ssize_t _read(int fd, void* buf, size_t cnt) { return _read_r(_REENT, fd, buf, cnt); }
off_t   _lseek(int fd, off_t pos, int dir) { return _lseek_r(_REENT, fd, pos, dir); }
int     _fstat(int fd, struct stat* st) { return _fstat_r(_REENT, fd, st); }
int     _stat(const char* file, struct stat* pstat) { return _stat_r(_REENT, file, pstat); }
int     _isatty(int fd) { return _isatty_r(_REENT, fd); }
int     _fcntlr(int fd, int op, int arg) { return _fcntl_r(_REENT, fd, op, arg); }
int     _statvfs(const char* path, struct statvfs* buf) { return _statvfs_r(_REENT, path, buf); }
int     _fsync(int fd) { return _fsync_r(_REENT, fd); }
int     _link(const char* existing, const char* newLink) { return _link_r(_REENT, existing, newLink); }
int     _unlink(const char* name) { return _unlink_r(_REENT, name); }
int     _rename(const char* old_name, const char* new_name) { return _rename_r(_REENT, old_name, new_name); }
int     _mkdir(const char* path, mode_t mode) { return _mkdir_r(_REENT, path, mode); }
int     rmdir(const char* name) { return _rmdir_r(_REENT, name); }
int     chdir(const char* path) { return _chdir_r(_REENT, path); }

DIR*           opendir(const char* dirname) { return syscalls::opendir(_REENT->_errno, dirname); }
int            closedir(DIR* dirp) { return syscalls::closedir(_REENT->_errno, dirp); }
int            readdir_r(DIR* dirp, struct dirent* entry, struct dirent** result) { return syscalls::readdir_r(_REENT->_errno, dirp, entry, result); }
struct dirent* readdir(DIR* dirp) { return syscalls::readdir(_REENT->_errno, dirp); }
void           rewinddir(DIR* dirp) { return syscalls::rewinddir(_REENT->_errno, dirp); }
void           seekdir(DIR* dirp, const long int loc) { return syscalls::seekdir(_REENT->_errno, dirp, loc); }
long int       telldir(DIR* dirp) { return syscalls::telldir(_REENT->_errno, dirp); }

int mount(const char* source, const char* target, const char* type, unsigned long mountflags, const void* data) { return syscalls::mount(_REENT->_errno, source, target, type, mountflags, data); }
int umount(const char* target) { return syscalls::umount(_REENT->_errno, target); }
}