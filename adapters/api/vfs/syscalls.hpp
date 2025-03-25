#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dirent.h>

struct statvfs;

namespace vfs::syscalls {
    int            open(int& _errno_, const char* file, int flags, int mode);
    int            close(int& _errno_, int fd);
    ssize_t        write(int& _errno_, int fd, const void* buf, size_t cnt);
    ssize_t        read(int& _errno_, int fd, void* buf, size_t cnt);
    off_t          lseek(int& _errno_, int fd, off_t pos, int dir);
    int            fstat(int& _errno_, int fd, struct stat* st);
    int            link(int& _errno_, const char* existing, const char* newLink);
    int            unlink(int& _errno_, const char* name);
    int            rmdir(int& _errno_, const char* name);
    int            fcntl(int& _errno_, int fd, int cmd, int arg);
    int            stat(int& _errno_, const char* file, struct stat* pstat);
    int            chdir(int& _errno_, const char* path);
    char*          getcwd(int& _errno_, char* buf, size_t size);
    int            rename(int& _errno_, const char* oldName, const char* newName);
    int            mkdir(int& _errno_, const char* path, mode_t mode);
    int            closedir(int& _errno_, DIR* dirp);
    DIR*           opendir(int& _errno_, const char* dirname);
    struct dirent* readdir(int& _errno_, DIR* dirp);
    int            readdir_r(int& _errno_, DIR* dirp, struct dirent* entry, struct dirent** result);
    void           rewinddir(int& _errno_, DIR* dirp);
    void           seekdir(int& _errno_, DIR* dirp, long int loc);
    long int       telldir(int& _errno_, DIR* dirp);
    int            chmod(int& _errno_, const char* path, mode_t mode);
    int            fchmod(int& _errno_, int fd, mode_t mode);
    int            fsync(int& _errno_, int fd);
    int            mount(int& _errno_, const char* dev, const char* dir, const char* fstype, unsigned long int rwflag, const void* data);
    int            umount(int& _errno_, const char* dev);
    int            statvfs(int& _errno_, const char* path, struct statvfs* buf);
    ssize_t        readlink(int& _errno_, const char* path, char* buf, size_t buflen);
    int            symlink(int& _errno_, const char* name1, const char* name2);
    long           fpathconf(int& _errno, int fd, int name);
    long           pathconf(int& _errno, const char* path, int name);
    int            isatty(int& _errno_, int fd);

} // namespace vfs::syscalls
