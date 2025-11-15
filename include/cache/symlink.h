#pragma once

#include "cache/file.h"

struct sftfs_cache_symlink_config {
    struct sftfs_cache_list_config list;
};

enum {
    SFTFS_CACHE_SYMLINK_OK = SFTFS_CACHE_FILE_OK,
    SFTFS_CACHE_SYMLINK_PUT_FAILED = SFTFS_CACHE_FILE_ERROR_MAX + 1,
};

struct sftfs_cache *sftfs_cache_symlink_construct(
        struct sftfs_cache *cache,
        struct sftfs_cache_symlink_config *config);
void sftfs_cache_symlink_destruct(struct sftfs_cache *cache);

bool sftfs_cache_get_symlink(struct sftfs_cache *cache, const char *path,
        char *buffer, size_t buffer_size);
int sftfs_cache_put_symlink(struct sftfs_cache *cache, const char *path, const char *symlink);
int sftfs_cache_invalidate_symlink(struct sftfs_cache *cache, const char *path);
