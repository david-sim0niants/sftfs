#pragma once

#include "cache/file.h"

#include <sys/stat.h>

typedef struct sftfs_cache_dir_s sftfs_cache_dir;

typedef struct {
    const char *curr_entry;
} sftfs_cache_dir_handle;

struct sftfs_cache_dir_config {
    struct sftfs_cache_list_config list;
};

enum {
    SFTFS_CACHE_DIR_OK,
    SFTFS_CACHE_DIR_ADD_ENTRY_FAILED = SFTFS_CACHE_FILE_ERROR_MAX + 1,
};

struct sftfs_cache *sftfs_cache_dir_construct(
        struct sftfs_cache *cache,
        struct sftfs_cache_dir_config *config);
void sftfs_cache_dir_destruct(struct sftfs_cache *cache);

sftfs_cache_dir *sftfs_cache_take_dir(struct sftfs_cache *cache, const char *path);
int sftfs_cache_give_dir(struct sftfs_cache *cache, const char *path, sftfs_cache_dir *dir);
int sftfs_cache_drop_dir(struct sftfs_cache *cache, const char *path, sftfs_cache_dir *dir);
int sftfs_cache_add_dir_entry(sftfs_cache_dir *dir, const char *entry);
int sftfs_cache_clear_dir_entries(sftfs_cache_dir *dir);

sftfs_cache_dir_handle sftfs_cache_peek_dir(struct sftfs_cache *cache, const char *path);
const char *sftfs_cache_read_dir(sftfs_cache_dir_handle *handle);

static inline bool sftfs_cache_dir_valid(const sftfs_cache_dir_handle handle)
{
    return handle.curr_entry != NULL;
}

int sftfs_cache_invalidate_dir(struct sftfs_cache *cache, const char *path);
