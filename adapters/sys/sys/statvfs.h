#pragma once

#if __has_include_next(<sys/statvfs.h>)
#pragma message("<sys/statvfs.h> available, using it instead of the custom one")
#include_next <sys/statvfs.h>
#else
#include <sys/_types.h>

#ifndef __fsblkcnt_t_defined
typedef __fsblkcnt_t fsblkcnt_t;
#define __fsblkcnt_t_defined
#endif
#ifndef __fsfilcnt_t_defined
typedef __fsfilcnt_t fsfilcnt_t;
#define __fsfilcnt_t_defined
#endif

struct statvfs {
    unsigned long int f_bsize;  /// File system block size
    unsigned long int f_frsize; /// Fundamental file system block size

    fsblkcnt_t f_blocks; /// Total number of blocks on file system in units of f_frsize
    fsblkcnt_t f_bfree;  /// Total number of free blocks
    fsblkcnt_t f_bavail; /// Number of free blocks available to non-privileged process
    fsfilcnt_t f_files;  /// Total number of file serial numbers
    fsfilcnt_t f_ffree;  /// Total number of free file serial numbers
    fsfilcnt_t f_favail; /// Number of file serial numbers available to non-privileged process

    unsigned long int f_fsid;    /// File system ID
    unsigned long int f_flag;    /// Bit mask of f_flag values
    unsigned long int f_namemax; /// Maximum filename length
};

enum {
    ST_RDONLY = 1, /* Mount read-only.  */
#define ST_RDONLY ST_RDONLY
    ST_NOSUID = 2  /* Ignore suid and sgid bits.  */
#define ST_NOSUID ST_NOSUID
};

__BEGIN_DECLS

/**
 * Return information about a mounted filesystem.
 * For more info, please check POSIX specification
 * @param __file he pathname of any file within the mounted filesystem
 * @param __buf a pointer to a @ref statvfs structure
 * @return On success, zero is returned.  On error, value < 0 is returned, and errno is set to indicate the error.
 */
int statvfs(const char* __restrict __file, struct statvfs* __restrict __buf) __THROW __nonnull((1, 2));
/**
 * Return the same information as @ref statvfs, but uses file descriptor instead of a file path
 * For more info, please check POSIX specification
 * @param __fildes file descriptor
 * @param __buf a pointer to a @ref statvfs structure
 * @return
 */
int fstatvfs(int __fildes, struct statvfs* __buf) __THROW __nonnull((2));

__END_DECLS

#endif
