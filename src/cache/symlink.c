#include "cache/symlink.h"
#include "abs/str.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

struct symlink_cache_entry {
    sftfs_str symlink;
};

void destruct_entry(void *data)
{
    return sftfs_str_delete(((struct symlink_cache_entry *)(data))->symlink);
}

struct sftfs_cache *sftfs_cache_symlink_construct(
        struct sftfs_cache *cache,
        struct sftfs_cache_symlink_config *config)
{
    struct sftfs_cache_file_config file_config = {
        .list = config->list,
        .file_data_dtor = destruct_entry,
        .file_data_size = sizeof(struct symlink_cache_entry),
    };

    return sftfs_cache_file_construct(cache, &file_config);
}

void sftfs_cache_symlink_destruct(struct sftfs_cache *cache)
{
    return sftfs_cache_file_destruct(cache);
}

bool sftfs_cache_get_symlink(struct sftfs_cache *cache, const char *path,
        char *buffer, size_t buffer_size)
{
    assert(buffer_size > 0);
    const struct symlink_cache_entry *entry = sftfs_cache_peek_file(cache, path);
    if (entry) {
        strncpy(buffer, sftfs_str_c(entry->symlink), buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }
    return !!entry;
}

int sftfs_cache_put_symlink(struct sftfs_cache *cache, const char *path, const char *symlink)
{
    int is_new = 0;
    struct symlink_cache_entry *entry = sftfs_cache_take_file(cache, path, &is_new);
    if (! entry)
        return SFTFS_CACHE_SYMLINK_PUT_FAILED;

    sftfs_str symlink_str = is_new ? sftfs_str_create(symlink, strlen(symlink))
                                   : sftfs_str_assign_cstr(entry->symlink, symlink);
    if (! symlink_str) {
        sftfs_cache_drop_file(cache, path, entry);
        return SFTFS_CACHE_SYMLINK_PUT_FAILED;
    }

    entry->symlink = symlink_str;
    return sftfs_cache_give_file(cache, path, entry);
}

int sftfs_cache_invalidate_symlink(struct sftfs_cache *cache, const char *path)
{
    return sftfs_cache_invalidate_file(cache, path);
}
