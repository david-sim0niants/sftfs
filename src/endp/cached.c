#include "endp/cached_init.h"
#include "cache/attr.h"
#include "func_trace.h"
#include "logging.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static inline struct sftfs_cached_endp *get_cached(sftfs_endp endp)
{
    return (struct sftfs_cached_endp *)(endp) - 1;
}

#define SFTFS_ENDP_OFF offsetof(struct sftfs_cached_endp, base_endp)

sftfs_endp sftfs_cached_init(size_t base_endp_size, struct sftfs_cached_params *params)
{
    struct sftfs_cached_endp *cached = calloc(1, SFTFS_ENDP_OFF + base_endp_size);
    if (cached && sftfs_cached_construct(cached, params))
        return &cached->base_endp;
    else if (cached)
        free(cached);
    return NULL;
}

void sftfs_cached_deinit(sftfs_endp endp)
{
    sftfs_cached_destruct(endp);
    free(get_cached(endp));
}

sftfs_endp sftfs_cached_construct(struct sftfs_cached_endp *handle, struct sftfs_cached_params *params)
{
    struct sftfs_cache_attr_config attr_config = {
        .list = {
            params->clock,
            params->ttl,
        },
    };
    if (NULL == sftfs_cache_attr_construct(&handle->attr_cache, &attr_config))
        return NULL;
    return handle->base_endp;
}

void sftfs_cached_destruct(sftfs_endp endp)
{
    sftfs_cache_attr_destruct(&get_cached(endp)->attr_cache);
}

bool sftfs_cached_fetch_attr(struct sftfs_cached_endp *endp, const char *path, struct stat *attr)
{
    SFTFS_TRACE_FUNC
    bool hit = sftfs_cache_get_attr(&endp->attr_cache, path, attr);
    if (hit)
        sftfs_trace("Cache HIT: attribute path: %s\n", path);
    else
        sftfs_trace("Cache MISS: attribute path: %s\n", path);
    return hit;
}

void sftfs_cached_store_attr(struct sftfs_cached_endp *endp, const char *path, const struct stat *attr)
{
    SFTFS_TRACE_FUNC
    int rc = sftfs_cache_put_attr(&endp->attr_cache, path, attr);
    if (rc != SFTFS_CACHE_ATTR_OK)
        sftfs_error("Failed to cache attribute for path %s, sftfs_cache_put_attr(...) failed\n", path);
}

void sftfs_cached_inval_attr(struct sftfs_cached_endp *endp, const char *path)
{
    SFTFS_TRACE_FUNC
    sftfs_cache_invalidate_attr(&endp->attr_cache, path);
}
