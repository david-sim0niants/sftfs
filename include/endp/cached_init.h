#pragma once

#include "defs.h"
#include "abs/htable.h"
#include "cache/cache.h"

struct sftfs_cached_endp {
    sftfs_htable handle_path_map;
    struct sftfs_cache attr_cache;
    struct sftfs_cache dir_cache;
    char base_endp[0];
};

struct sftfs_cached_params {
    sftfs_cache_clock_t clock;
    sftfs_cache_time_t attr_ttl;
    sftfs_cache_time_t dir_ttl;
};

sftfs_endp sftfs_cached_init(size_t base_endp_size, struct sftfs_cached_params *params);

#define SFTFS_CACHED_INIT(endp, params, base_endp_type, base_endp_ctor, ...) \
    (void)((endp = sftfs_cached_init(sizeof(base_endp_type), params)) && \
     (base_endp_ctor((base_endp_type *)endp, __VA_ARGS__) || (sftfs_cached_deinit(endp), endp = NULL)))

void sftfs_cached_deinit(sftfs_endp endp);

#define SFTFS_CACHED_DEINIT(endp, base_endp_dtor) (base_endp_dtor(endp), sftfs_cached_deinit(endp))

sftfs_endp sftfs_cached_construct(struct sftfs_cached_endp *handle, struct sftfs_cached_params *params);
void sftfs_cached_destruct(sftfs_endp endp);
