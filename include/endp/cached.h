#pragma once

/* Include this after including the endpoint API you want to cache.
 * Default-included no-op SFTFS_ENDP_* methods will be overridden. */

#include "none.h"
#include "defs.h"
#include "cached_init.h"
#include "cache/attrib.h"

#include "func_trace.h"

static inline struct sftfs_cached_endp *sftfs_cached_get(sftfs_endp endp)
{
    return (struct sftfs_cached_endp *)(endp) - 1;
}

static inline
int sftfs_cached_getattr(sftfs_endp endp, const char *path, sftfs_endp_file file, struct stat *attr)
{
    SFTFS_TRACE_FUNC
    if (sftfs_cache_get_attr(&sftfs_cached_get(endp)->attr_cache, path, attr))
        return 0;
    int rc = SFTFS_ENDP_getattr(endp, path, file, attr);
    if (rc == 0)
        sftfs_cache_put_attr(&sftfs_cached_get(endp)->attr_cache, path, attr);
    return rc;
}

#undef SFTFS_ENDP_getattr
#define SFTFS_ENDP_getattr sftfs_cached_getattr

static inline
int sftfs_cached_chmod(sftfs_endp endp, const char *path, mode_t mode)
{
    sftfs_cache_invalidate_attr(&sftfs_cached_get(endp)->attr_cache, path);
    return SFTFS_ENDP_chmod(endp, path, mode);
}

static inline
int sftfs_cached_chown(sftfs_endp endp, const char *path, uid_t uid, gid_t gid)
{
    sftfs_cache_invalidate_attr(&sftfs_cached_get(endp)->attr_cache, path);
    return SFTFS_ENDP_chown(endp, path, uid, gid);
}
