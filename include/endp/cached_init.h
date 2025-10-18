#pragma once

#include "defs.h"
#include "cached_fs.h"

struct sftfs_cached {
    struct sftfs_cached_fs_s fs;
    char base_endp[0];
};

struct sftfs_cached_params {};

sftfs_endp sftfs_cached_init(size_t base_endp_size, struct sftfs_cached_params *params);

#define SFTFS_CACHED_INIT(endp, params, base_endp_type, base_endp_ctor, ...) \
    (void)((endp = sftfs_cached_init(sizeof(base_endp_type), params)) && \
     (base_endp_ctor((base_endp_type *)endp, __VA_ARGS__) || (sftfs_cached_deinit(endp), endp = NULL)))

void sftfs_cached_deinit(sftfs_endp endp);

#define SFTFS_CACHED_DEINIT(endp, base_endp_dtor) (base_endp_dtor(endp), sftfs_cached_deinit(endp))

sftfs_endp sftfs_cached_construct(struct sftfs_cached *handle, struct sftfs_cached_params *params);

void sftfs_cached_destruct(sftfs_endp endp);
