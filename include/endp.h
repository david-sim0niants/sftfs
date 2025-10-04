#pragma once

#include <stddef.h>
#include <stdint.h>

#include <sys/stat.h>

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

int sftfs_endp_getattr(sftfs_endp endp, const char *path, sftfs_endp_file file, struct stat *stat);

int sftfs_endp_readlink(sftfs_endp endp, const char *path, char *buf, size_t bufsiz);

int sftfs_endp_opendir(sftfs_endp endp, const char *path, sftfs_endp_dir *dir);

int sftfs_endp_readdir(sftfs_endp endp, sftfs_endp_dir dir, int flags,
        sftfs_endp_readdir_callee callee, void *user_data);

int sftfs_endp_closedir(sftfs_endp endp, sftfs_endp_dir dir);

int sftfs_endp_open(sftfs_endp endp, sftfs_endp_file *file, const char *path, int access_flags);

int sftfs_endp_read(sftfs_endp endp, sftfs_endp_file file, char *buf, size_t size, off_t off);

int sftfs_endp_close(sftfs_endp endp, sftfs_endp_file file);

int sftfs_endp_access(sftfs_endp endp, const char *path, int mode);
