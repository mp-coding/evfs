#pragma once

#include <sys/types.h>
#include <sys/syslimits.h>

#define _DIRENT_HAVE_D_TYPE

#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12

__BEGIN_DECLS

struct dirent {
    ino_t         d_ino;
    unsigned char d_type;
    size_t        d_reclen;
    char          d_name[NAME_MAX + 1];
};

struct __dirstream;
typedef struct __dirstream DIR;

__END_DECLS