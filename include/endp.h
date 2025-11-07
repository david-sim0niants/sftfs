#pragma once

#include "endp/endp.h"

static inline
int sftfs_endp_getattr(sftfs_endp endp, const char *path, sftfs_endp_file file, struct stat *stat)
{
    return SFTFS_ENDP_getattr(endp, path, file, stat);
}

static inline
int sftfs_endp_readlink(sftfs_endp endp, const char *path, char *buf, size_t bufsiz)
{
    return SFTFS_ENDP_readlink(endp, path, buf, bufsiz);
}

static inline
int sftfs_endp_mkdir(sftfs_endp endp, const char *path, mode_t mode)
{
    return SFTFS_ENDP_mkdir(endp, path, mode);
}

static inline
int sftfs_endp_unlink(sftfs_endp endp, const char *path)
{
    return SFTFS_ENDP_unlink(endp, path);
}

static inline
int sftfs_endp_rmdir(sftfs_endp endp, const char *path)
{
    return SFTFS_ENDP_rmdir(endp, path);
}

static inline
int sftfs_endp_symlink(sftfs_endp endp, const char *target, const char *linkpath)
{
    return SFTFS_ENDP_symlink(endp, target, linkpath);
}

static inline
int sftfs_endp_rename(sftfs_endp endp, const char *oldpath, const char *newpath, unsigned int flags)
{
    return SFTFS_ENDP_rename(endp, oldpath, newpath, flags);
}

static inline
int sftfs_endp_chmod(sftfs_endp endp, const char *path, mode_t mode)
{
    return SFTFS_ENDP_chmod(endp, path, mode);
}

static inline
int sftfs_endp_chown(sftfs_endp endp, const char *path, uid_t uid, gid_t gid)
{
    return SFTFS_ENDP_chown(endp, path, uid, gid);
}

static inline
int sftfs_endp_opendir(sftfs_endp endp, const char *path, sftfs_endp_dir *dir)
{
    return SFTFS_ENDP_opendir(endp, path, dir);
}

static inline
int sftfs_endp_readdir(sftfs_endp endp, const char *path, sftfs_endp_dir dir, int flags,
        sftfs_endp_readdir_callee callee, void *user_data)
{
    return SFTFS_ENDP_readdir(endp, path, dir, flags, callee, user_data);
}

static inline
int sftfs_endp_closedir(sftfs_endp endp, sftfs_endp_dir dir)
{
    return SFTFS_ENDP_closedir(endp, dir);
}

static inline
int sftfs_endp_open(sftfs_endp endp, sftfs_endp_file *file, const char *path, int access_flags)
{
    return SFTFS_ENDP_open(endp, file, path, access_flags);
}

static inline
int sftfs_endp_read(sftfs_endp endp, sftfs_endp_file file, char *buf, size_t size, off_t off)
{
    return SFTFS_ENDP_read(endp, file, buf, size, off);
}

static inline
int sftfs_endp_write(sftfs_endp endp, sftfs_endp_file file, const char *buf, size_t size, off_t off)
{
    return SFTFS_ENDP_write(endp, file, buf, size, off);
}

static inline
int sftfs_endp_statfs(sftfs_endp endp, const char *path, struct statvfs *statv)
{
    return SFTFS_ENDP_statfs(endp, path, statv);
}

static inline
int sftfs_endp_close(sftfs_endp endp, sftfs_endp_file file)
{
    return SFTFS_ENDP_close(endp, file);
}

static inline
int sftfs_endp_access(sftfs_endp endp, const char *path, int mode)
{
    return SFTFS_ENDP_access(endp, path, mode);
}

static inline
int sftfs_endp_create(sftfs_endp endp, const char *path, mode_t mode, sftfs_endp_file *file)
{
    return SFTFS_ENDP_create(endp, path, mode, file);
}

static inline
int sftfs_endp_utimens(sftfs_endp endp, const char *path, const struct timespec tv[2])
{
    return SFTFS_ENDP_utimens(endp, path, tv);
}
