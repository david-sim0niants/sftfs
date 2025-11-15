#pragma once

#include <stdbool.h>
#include <sys/stat.h>

#include "cache/file.h"

struct sftfs_cache_attr_config {
    struct sftfs_cache_list_config list;
};

enum {
    SFTFS_CACHE_ATTR_OK = SFTFS_CACHE_FILE_OK,
    SFTFS_CACHE_ATTR_PUT_FAILED = SFTFS_CACHE_FILE_ERROR_MAX + 1,
};

struct sftfs_cache *sftfs_cache_attr_construct(
        struct sftfs_cache *cache,
        struct sftfs_cache_attr_config *config);
void sftfs_cache_attr_destruct(struct sftfs_cache *cache);

bool sftfs_cache_get_attr(struct sftfs_cache *cache, const char *path, struct stat *attr);
int sftfs_cache_put_attr(struct sftfs_cache *cache, const char *path, const struct stat *attr);
int sftfs_cache_invalidate_attr(struct sftfs_cache *cache, const char *path);
