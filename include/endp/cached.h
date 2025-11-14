#pragma once

/* Include this after including the endpoint API you want to cache.
 * Default-included no-op SFTFS_ENDP_* methods will be overridden. */

#include "none.h"
#include "defs.h"
#include "cached_init.h"

#include "func_trace.h"
#include <assert.h>

static inline struct sftfs_cached_endp *sftfs_cached_get(sftfs_endp endp)
{
    return (struct sftfs_cached_endp *)(endp) - 1;
}

void sftfs_cached_inval_all(struct sftfs_cached_endp *endp, const char *path);
void sftfs_cached_inval_all_dir(struct sftfs_cached_endp *endp, const char *path);

bool sftfs_cached_fetch_attr(struct sftfs_cached_endp *endp, const char *path, struct stat *attr);
bool sftfs_cached_store_attr(struct sftfs_cached_endp *endp, const char *path, const struct stat *attr);
void sftfs_cached_inval_attr(struct sftfs_cached_endp *endp, const char *path);

static inline
int sftfs_cached_getattr(sftfs_endp endp, const char *path, sftfs_endp_file file, struct stat *attr)
{
    SFTFS_TRACE_FUNC
    assert(path[0] == '/');
    if (sftfs_cached_fetch_attr(sftfs_cached_get(endp), path, attr))
        return 0;
    int rc = SFTFS_ENDP_getattr(endp, path, file, attr);
    if (rc == 0)
        sftfs_cached_store_attr(sftfs_cached_get(endp), path, attr);
    return rc;
}
#undef SFTFS_ENDP_getattr
#define SFTFS_ENDP_getattr sftfs_cached_getattr

static inline
int sftfs_cached_mkdir(sftfs_endp endp, const char *path, mode_t mode)
{
    SFTFS_TRACE_FUNC
    int rc = SFTFS_ENDP_mkdir(endp, path, mode);
    if (rc == 0)
        sftfs_cached_inval_all_dir(sftfs_cached_get(endp), path);
    return rc;
}
#undef SFTFS_ENDP_mkdir
#define SFTFS_ENDP_mkdir   sftfs_cached_mkdir

static inline
int sftfs_cached_unlink(sftfs_endp endp, const char *path)
{
    SFTFS_TRACE_FUNC
    int rc = SFTFS_ENDP_unlink(endp, path);
    if (rc == 0)
        sftfs_cached_inval_all_dir(sftfs_cached_get(endp), path);
    return rc;
}
#undef SFTFS_ENDP_unlink
#define SFTFS_ENDP_unlink   sftfs_cached_unlink

static inline
int sftfs_cached_rmdir(sftfs_endp endp, const char *path)
{
    SFTFS_TRACE_FUNC
    int rc = SFTFS_ENDP_rmdir(endp, path);
    if (rc == 0)
        sftfs_cached_inval_all_dir(sftfs_cached_get(endp), path);
    return rc;
}
#undef SFTFS_ENDP_rmdir
#define SFTFS_ENDP_rmdir   sftfs_cached_rmdir

static inline
int sftfs_cached_symlink(sftfs_endp endp, const char *target, const char *linkpath)
{
    SFTFS_TRACE_FUNC
    int rc = SFTFS_ENDP_symlink(endp, target, linkpath);
    if (rc == 0)
        sftfs_cached_inval_all_dir(sftfs_cached_get(endp), linkpath);
    return rc;
}
#undef SFTFS_ENDP_symlink
#define SFTFS_ENDP_symlink sftfs_cached_symlink

static inline
int sftfs_cached_chmod(sftfs_endp endp, const char *path, mode_t mode)
{
    SFTFS_TRACE_FUNC
    int rc = SFTFS_ENDP_chmod(endp, path, mode);
    if (rc == 0)
        sftfs_cached_inval_attr(sftfs_cached_get(endp), path);
    return rc;
}
#undef SFTFS_ENDP_chmod
#define SFTFS_ENDP_chmod sftfs_cached_chmod

static inline
int sftfs_cached_chown(sftfs_endp endp, const char *path, uid_t uid, gid_t gid)
{
    SFTFS_TRACE_FUNC
    int rc = SFTFS_ENDP_chown(endp, path, uid, gid);
    if (rc == 0)
        sftfs_cached_inval_attr(sftfs_cached_get(endp), path);
    return rc;
}
#undef SFTFS_ENDP_chown
#define SFTFS_ENDP_chown sftfs_cached_chown

bool sftfs_cached_dir_exists(struct sftfs_cached_endp *endp, const char *path);

static inline bool sftfs_cached_dir_is_cached(sftfs_endp_dir dir)
{
    return dir.handle == 0;
}

bool sftfs_cached_fetch_dir_entries(struct sftfs_cached_endp *endp, const char *path,
        int flags, sftfs_endp_readdir_callee callee, void *user_data,
        int (*getattr)(sftfs_endp endp, const char *path, sftfs_endp_file file, struct stat *attr));

/* Pass pointers to cachee callee and user data; get the wrapped cached callee and user data.
 * IMPORTANT: returned pointers are valid only until the next call. No thread-safety whatsoever. */
bool sftfs_cached_setup_dir_caching(struct sftfs_cached_endp *endp,
        const char *path, sftfs_endp_readdir_callee *callee, void **user_data);
void sftfs_cached_teardown_dir_caching(struct sftfs_cached_endp *endp, const char *path);

static inline
int sftfs_cached_opendir(sftfs_endp endp, const char *path, sftfs_endp_dir *dir)
{
    SFTFS_TRACE_FUNC
    if (sftfs_cached_dir_exists(sftfs_cached_get(endp), path)) {
        dir->handle = 0;
        return 0;
    }
    return SFTFS_ENDP_opendir(endp, path, dir);
}

static inline
int sftfs_cached_readdir(sftfs_endp endp, const char *path, sftfs_endp_dir dir, int flags,
        sftfs_endp_readdir_callee callee, void *user_data)
{
    SFTFS_TRACE_FUNC

    bool dir_is_cached = sftfs_cached_dir_is_cached(dir);
    if (dir_is_cached) {
        if (sftfs_cached_fetch_dir_entries(sftfs_cached_get(endp),
                    path, flags, callee, user_data, (void*)SFTFS_ENDP_getattr))
            return 0;
        int rc = SFTFS_ENDP_opendir(endp, path, &dir);
        if (rc)
            return rc;
    }

    sftfs_endp_readdir_callee cached_callee = callee; void *cached_user_data = user_data;
    const bool dir_caching_failed =
        sftfs_cached_setup_dir_caching(sftfs_cached_get(endp),
                path, &cached_callee, &cached_user_data);

    int rc = SFTFS_ENDP_readdir(endp, path, dir, flags, cached_callee, cached_user_data);

    if (dir_caching_failed)
        sftfs_cached_teardown_dir_caching(sftfs_cached_get(endp), path);

    if (dir_is_cached)
        SFTFS_ENDP_closedir(endp, dir);

    return rc;
}

static inline
int sftfs_cached_closedir(sftfs_endp endp, sftfs_endp_dir dir)
{
    SFTFS_TRACE_FUNC
    if (sftfs_cached_dir_is_cached(dir))
        return 0;
    else
        return SFTFS_ENDP_closedir(endp, dir);
}

#undef SFTFS_ENDP_opendir
#define SFTFS_ENDP_opendir  sftfs_cached_opendir

#undef SFTFS_ENDP_readdir
#define SFTFS_ENDP_readdir  sftfs_cached_readdir

#undef SFTFS_ENDP_closedir
#define SFTFS_ENDP_closedir sftfs_cached_closedir

static inline
int sftfs_cached_create(sftfs_endp endp, const char *path, mode_t mode, sftfs_endp_file *file)
{
    SFTFS_TRACE_FUNC
    int rc = SFTFS_ENDP_create(endp, path, mode, file);
    if (rc == 0)
        sftfs_cached_inval_all_dir(sftfs_cached_get(endp), path);
    return rc;
}
#undef SFTFS_ENDP_create
#define SFTFS_ENDP_create sftfs_cached_create

static inline
int sftfs_cached_utimens(sftfs_endp endp, const char *path, const struct timespec tv[2])
{
    SFTFS_TRACE_FUNC
    int rc = SFTFS_ENDP_utimens(endp, path, tv);
    if (rc == 0)
        sftfs_cached_inval_attr(sftfs_cached_get(endp), path);
    return rc;
}
#undef SFTFS_ENDP_utimens
#define SFTFS_ENDP_utimens  sftfs_cached_utimens
