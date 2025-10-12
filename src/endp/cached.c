#include "endp/cached_init.h"

#include <stddef.h>
#include <stdlib.h>

static inline struct sftfs_cached *get_cached(sftfs_endp endp)
{
    return (struct sftfs_cached *)(endp) - 1;
}

#define SFTFS_ENDP_OFF offsetof(struct sftfs_cached, base_endp)

sftfs_endp sftfs_cached_init(size_t base_endp_size, struct sftfs_cached_params *params)
{
    struct sftfs_cached *cached = calloc(1, SFTFS_ENDP_OFF + base_endp_size);
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

sftfs_endp sftfs_cached_construct(struct sftfs_cached *handle, struct sftfs_cached_params *params)
{
    (void)params;
    return handle;
}

void sftfs_cached_destruct(sftfs_endp endp)
{
    (void)endp;
}
