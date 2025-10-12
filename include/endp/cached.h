#pragma once

/* Include this after including the endpoint API you want to cache.
 * Default-included no-op SFTFS_ENDP_* methods will be overriden. */

#include "none.h"
#include "defs.h"
#include "cached_init.h"

#include "func_trace.h"

static inline struct sftfs_cached *sftfs_cached_get(sftfs_endp endp)
{
    return (struct sftfs_cached *)(endp) - 1;
}

static inline
int sftfs_cached_getattr(sftfs_endp endp, const char *path, sftfs_endp_file file, struct stat *stat)
{
    SFTFS_TRACE_FUNC
    return SFTFS_ENDP_getattr(endp, path, file, stat);
}

#undef SFTFS_ENDP_getattr
#define SFTFS_ENDP_getattr sftfs_cached_getattr
