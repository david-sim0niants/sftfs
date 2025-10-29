#include "endp/cached_init.h"
#include "cache/attrib.h"

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
