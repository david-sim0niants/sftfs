#pragma once

#include "cache.h"

struct sftfs_cache_file_config {
    struct sftfs_cache_list_config list;
    size_t file_data_size;
};

enum {
    SFTFS_CACHE_FILE_OK,
    SFTFS_CACHE_FILE_PATH_MISMATCH,
};

void sftfs_cache_file_construct(struct sftfs_cache *cache, struct sftfs_cache_file_config *config);
void sftfs_cache_file_destruct(struct sftfs_cache *cache);

void *sftfs_cache_take_file(struct sftfs_cache *cache, const char *path);
int sftfs_cache_give_file(struct sftfs_cache *cache, const char *path, void *data);
const void *sftfs_cache_peek_file(struct sftfs_cache *cache, const char *path);
int sftfs_cache_invalidate_file(struct sftfs_cache *cache, const char *path);
