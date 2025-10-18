#pragma once

/* Include this after including the endpoint API you want to cache.
 * Default-included no-op SFTFS_ENDP_* methods will be overridden. */

#include "none.h"
#include "defs.h"
#include "cached_init.h"

#include "func_trace.h"

static inline struct sftfs_cached *sftfs_cached_get(sftfs_endp endp)
{
    return (struct sftfs_cached *)(endp) - 1;
}

static inline sftfs_cached_fs sftfs_cached_fs_get(sftfs_endp endp)
{
    return &sftfs_cached_get(endp)->fs;
}

static inline
int sftfs_cached_getattr(sftfs_endp endp, const char *path, sftfs_endp_file file, struct stat *attr)
{
    SFTFS_TRACE_FUNC
    if (sftfs_cached_fetch_getattr(sftfs_cached_fs_get(endp), path, attr))
        return 0;
    int rc = SFTFS_ENDP_getattr(endp, path, file, attr);
    if (rc == 0)
        sftfs_cached_cache_getattr(sftfs_cached_fs_get(endp), path, attr);
    return rc;
}

#undef SFTFS_ENDP_getattr
#define SFTFS_ENDP_getattr sftfs_cached_getattr
