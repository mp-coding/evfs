/**
 * mount
 * Created on: 12/03/2025
 * Author mateuszpiesta
 * Company: mprogramming
 */

#pragma once

#include <sys/types.h>

__BEGIN_DECLS

/* Mount flags */
#define MS_RDONLY 0x0001      /* Mount read-only */
#define MS_NOSUID 0x0002      /* Ignore set-user-ID and set-group-ID bits */
#define MS_NODEV 0x0004       /* Disallow access to device files */
#define MS_NOEXEC 0x0008      /* Disallow program execution */
#define MS_SYNCHRONOUS 0x0010 /* Writes are synced immediately */
#define MS_REMOUNT 0x0020     /* Remount with different options */
#define MS_MANDLOCK 0x0040    /* Allow mandatory file locking */
#define MS_NOATIME 0x0080     /* Do not update access times */
#define MS_NODIRATIME 0x0100  /* Do not update directory access times */
#define MS_BIND 0x0200        /* Bind mount */
#define MS_MOVE 0x0400        /* Move mount */
#define MS_REC 0x0800         /* Recursive mount */
#define MS_PRIVATE 0x1000     /* Set mount as private */
#define MS_UNBINDABLE 0x2000  /* Mark mount as unbindable */

int mount(const char* source, const char* target, const char* filesystemtype, unsigned long mountflags, const void* data);
int umount(const char* target);
int umount2(const char* target, int flags);

__END_DECLS
