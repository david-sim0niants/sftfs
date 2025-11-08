#include "cache/dir.h"
#include "cache/file.h"
#include "abs/str.h"

struct sftfs_cache_dir_s {
    sftfs_str entries;
};

static void construct_dir(sftfs_cache_dir *dir)
{
    dir->entries = sftfs_str_create("", 0);
}

static void destruct_dir(sftfs_cache_dir *dir)
{
    sftfs_str_delete(dir->entries);
    dir->entries = NULL;
}

static void destruct_dir_data(void *dir)
{
    return destruct_dir(dir);
}

struct sftfs_cache *sftfs_cache_dir_construct(
        struct sftfs_cache *cache,
        struct sftfs_cache_dir_config *config)
{
    struct sftfs_cache_file_config file_config = {
        .list = config->list,
        .file_data_size = sizeof(sftfs_cache_dir),
        .file_data_dtor = destruct_dir_data,
    };
    return sftfs_cache_file_construct(cache, &file_config);
}

void sftfs_cache_dir_destruct(struct sftfs_cache *cache)
{
    sftfs_cache_file_destruct(cache);
}

sftfs_cache_dir *sftfs_cache_take_dir(struct sftfs_cache *cache, const char *path)
{
    int is_new = 0;
    sftfs_cache_dir *dir = sftfs_cache_take_file(cache, path, &is_new);
    if (dir && is_new)
        construct_dir(dir);
    return dir;
}

int sftfs_cache_give_dir(struct sftfs_cache *cache, const char *path, sftfs_cache_dir *dir)
{
    return sftfs_cache_give_file(cache, path, dir);
}

int sftfs_cache_add_dir_entry(sftfs_cache_dir *dir, const char *entry)
{
    sftfs_str new_entries = sftfs_str_extend(dir->entries, entry, strlen(entry) + 1);
    if (! new_entries)
        return SFTFS_CACHE_DIR_ADD_ENTRY_FAILED;
    dir->entries = new_entries;
    return SFTFS_CACHE_DIR_OK;
}

int sftfs_cache_clear_dir_entries(sftfs_cache_dir *dir)
{
    dir->entries = sftfs_str_resize(dir->entries, 0);
    return SFTFS_CACHE_DIR_OK;
}

sftfs_cache_dir_handle sftfs_cache_peek_dir(struct sftfs_cache *cache, const char *path)
{
    const sftfs_cache_dir *dir = sftfs_cache_peek_file(cache, path);
    return (sftfs_cache_dir_handle){.curr_entry = dir ? sftfs_str_c(dir->entries) : NULL};
}

const char *sftfs_cache_read_dir(sftfs_cache_dir_handle *handle)
{
    if (!*handle->curr_entry)
        return NULL;

    const char *entry = handle->curr_entry;
    const char *next_entry = entry;

    while (*++next_entry);
    ++next_entry;
    handle->curr_entry = next_entry;
    return entry;
}

int sftfs_cache_invalidate_dir(struct sftfs_cache *cache, const char *path)
{
    return sftfs_cache_invalidate_file(cache, path);
}
