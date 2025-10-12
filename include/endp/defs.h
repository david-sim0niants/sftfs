#pragma once

#include <stddef.h>
#include <stdint.h>

#include <sys/stat.h>
#include <sys/statvfs.h>

typedef void *sftfs_endp;

typedef struct { uintptr_t handle; } sftfs_endp_dir;

struct sftfs_endp_direntry {
    const char *name;
    struct stat stat;
};

typedef int (*sftfs_endp_readdir_callee)(struct sftfs_endp_direntry *direntry, void *user_data);

enum sftfs_endp_readdir_flags {
    SFTFS_ENDP_READDIR_DEFAULT = 0,
    SFTFS_ENDP_READDIR_PLUS = 1,
};

typedef struct { uintptr_t handle; } sftfs_endp_file;
