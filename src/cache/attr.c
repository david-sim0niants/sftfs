#include "cache/attr.h"
#include "cache/file.h"

struct sftfs_cache *sftfs_cache_attr_construct(
        struct sftfs_cache *cache,
        struct sftfs_cache_attr_config *config)
{
    struct sftfs_cache_file_config file_config = {
        .list = config->list,
        .file_data_dtor = NULL,
        .file_data_size = sizeof(struct stat),
    };
    return sftfs_cache_file_construct(cache, &file_config);
}

void sftfs_cache_attr_destruct(struct sftfs_cache *cache)
{
    return sftfs_cache_file_destruct(cache);
}

bool sftfs_cache_get_attr(struct sftfs_cache *cache, const char *path, struct stat *attr)
{
    const void *data = sftfs_cache_peek_file(cache, path);
    if (data)
        memcpy(attr, data, sizeof(*attr));
    return !!data;
}

int sftfs_cache_put_attr(struct sftfs_cache *cache, const char *path, const struct stat *attr)
{
    void *data = sftfs_cache_take_file(cache, path);
    if (! data)
        return SFTFS_CACHE_ATTR_PUT_FAILED;
    memcpy(data, attr, sizeof(*attr));
    return sftfs_cache_give_file(cache, path, data);
}

int sftfs_cache_invalidate_attr(struct sftfs_cache *cache, const char *path)
{
    return sftfs_cache_invalidate_file(cache, path);
}
