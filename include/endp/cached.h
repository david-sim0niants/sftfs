#pragma once

/* Include this after including the endpoint API you want to cache.
 * Default-included no-op SFTFS_ENDP_* methods will be overridden. */

#include "none.h"
#include "defs.h"
#include "cached_init.h"

#include "func_trace.h"

static inline struct sftfs_cached_endp *sftfs_cached_get(sftfs_endp endp)
{
    return (struct sftfs_cached_endp *)(endp) - 1;
}

bool sftfs_cached_fetch_attr(struct sftfs_cached_endp *endp, const char *path, struct stat *attr);
void sftfs_cached_store_attr(struct sftfs_cached_endp *endp, const char *path, const struct stat *attr);
void sftfs_cached_inval_attr(struct sftfs_cached_endp *endp, const char *path);

static inline
int sftfs_cached_getattr(sftfs_endp endp, const char *path, sftfs_endp_file file, struct stat *attr)
{
    SFTFS_TRACE_FUNC
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
int sftfs_cached_chmod(sftfs_endp endp, const char *path, mode_t mode)
{
    SFTFS_TRACE_FUNC
    sftfs_cached_inval_attr(sftfs_cached_get(endp), path);
    return SFTFS_ENDP_chmod(endp, path, mode);
}

#undef SFTFS_ENDP_chmod
#define SFTFS_ENDP_chmod sftfs_cached_chmod

static inline
int sftfs_cached_chown(sftfs_endp endp, const char *path, uid_t uid, gid_t gid)
{
    SFTFS_TRACE_FUNC
    sftfs_cached_inval_attr(sftfs_cached_get(endp), path);
    return SFTFS_ENDP_chown(endp, path, uid, gid);
}

#undef SFTFS_ENDP_chown
#define SFTFS_ENDP_chown sftfs_cached_chown
