#include "cache/file.h"

#include "abs/str.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct file_cache_entry {
    sftfs_str path;
    char data[0];
};

static inline
struct file_cache_entry *construct_file(const char *path, struct file_cache_entry *file)
{
    file->path = sftfs_str_create(path, strlen(path));
    if (file->path)
        return file;
    else
        return NULL;
}

static inline void destruct_file(struct file_cache_entry *file)
{
    sftfs_str_delete(file->path);
    file->path = NULL;
}

static void on_evict_file_without_data_dtor(void *entry_data, void *user_data)
{
    (void)user_data;
    destruct_file((struct file_cache_entry *)entry_data);
}

static void on_evict_file_with_data_dtor(void *entry_data, void *user_data)
{
    struct file_cache_entry *file_entry = entry_data;
    void (*file_data_dtor)(void *) = user_data;
    file_data_dtor(file_entry->data);
    destruct_file(file_entry);
}

struct sftfs_cache *sftfs_cache_file_construct(
        struct sftfs_cache *cache, struct sftfs_cache_file_config *config)
{
    struct sftfs_cache_config core_config = {
        .list = config->list,
        .on_evict = config->file_data_dtor
            ? (struct sftfs_cache_on_evict)
                {on_evict_file_with_data_dtor, (void *)config->file_data_dtor}
            : (struct sftfs_cache_on_evict)
                {on_evict_file_without_data_dtor, NULL},
        .entry_data_size = sizeof(struct file_cache_entry) + config->file_data_size,
    };
    return sftfs_cache_construct(cache, &core_config);
}

void sftfs_cache_file_destruct(struct sftfs_cache *cache)
{
    sftfs_cache_destruct(cache);
}

static struct file_cache_entry *view_file(sftfs_cache_entry *entry)
{
    return entry ? sftfs_cache_entry_data(entry) : NULL;
}

static const struct file_cache_entry *view_file_ro(const sftfs_cache_entry *entry)
{
    return entry ? sftfs_cache_entry_data_ro(entry) : NULL;
}

static size_t fnv1a_hash(const char *str)
{
    size_t hash = 2166136261UL;
    while (*str) {
        hash ^= (unsigned char)(*str++);
        hash *= 16777619;
    }
    return hash;
}

static inline bool str_eq(sftfs_str_ro s, const char *c)
{
    return !strncmp(s->c, c, s->size);
}

static const sftfs_cache_entry *find_entry(struct sftfs_cache *cache, const char *path)
{
    const sftfs_cache_entry *entry = sftfs_cache_peek(cache, fnv1a_hash(path));
    for (; entry; entry = sftfs_cache_peek_next(entry))
        if (str_eq(view_file_ro(entry)->path, path))
            break;
    return entry;
}

void *sftfs_cache_take_file(struct sftfs_cache *cache, const char *path, int *is_new)
{
    const sftfs_cache_entry *entry = find_entry(cache, path);
    bool newly_allocated = false;

    sftfs_cache_entry *taken_entry = NULL;
    if (entry) {
        taken_entry = sftfs_cache_take(cache, entry);
    } else {
        taken_entry = sftfs_cache_alloc(cache, fnv1a_hash(path));
        newly_allocated = true;
    }

    if (! taken_entry)
        return NULL;

    struct file_cache_entry *file_entry = view_file(taken_entry);
    if (newly_allocated)
        file_entry = construct_file(path, file_entry);

    if (! file_entry) {
        sftfs_cache_free(cache, taken_entry);
        return NULL;
    }

    if (is_new)
        *is_new = newly_allocated;

    return file_entry->data;
}

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

int sftfs_cache_give_file(struct sftfs_cache *cache, const char *path, void *data)
{
    struct file_cache_entry *file_entry = container_of(data, struct file_cache_entry, data);
    if (! str_eq(file_entry->path, path))
        return SFTFS_CACHE_FILE_PATH_MISMATCH;

    sftfs_cache_entry *entry = container_of(file_entry, sftfs_cache_entry, data);
    return sftfs_cache_give(cache, entry);
}

int sftfs_cache_drop_file(struct sftfs_cache *cache, const char *path, void *data)
{
    struct file_cache_entry *file_entry = container_of(data, struct file_cache_entry, data);
    if (! str_eq(file_entry->path, path))
        return SFTFS_CACHE_FILE_PATH_MISMATCH;

    destruct_file(file_entry);
    sftfs_cache_entry *entry = container_of(file_entry, sftfs_cache_entry, data);
    return sftfs_cache_free(cache, entry);
}

const void *sftfs_cache_peek_file(struct sftfs_cache *cache, const char *path)
{
    const sftfs_cache_entry *entry = find_entry(cache, path);
    return entry ? view_file_ro(entry)->data : NULL;
}

int sftfs_cache_invalidate_file(struct sftfs_cache *cache, const char *path)
{
    const sftfs_cache_entry *entry = find_entry(cache, path);
    return entry ? sftfs_cache_invalidate(cache, entry) : SFTFS_CACHE_FILE_OK;
}

int sftfs_cache_rename_file(struct sftfs_cache *cache, const char *oldpath, const char *newpath)
{
    const sftfs_cache_entry *entry_ro = find_entry(cache, oldpath);
    if (! entry_ro)
        return SFTFS_CACHE_FILE_DOES_NOT_EXIST;
    sftfs_cache_entry *entry = sftfs_cache_take(cache, entry_ro);
    if (! entry)
        return SFTFS_CACHE_FILE_DOES_NOT_EXIST;

    sftfs_str newpath_str = sftfs_str_assign_cstr(view_file(entry)->path, newpath);
    if (! newpath_str)
        return SFTFS_CACHE_FILE_RENAME_FAILED;
    view_file(entry)->path = newpath_str;

    int rc = sftfs_cache_rehash_entry(cache, entry, fnv1a_hash(sftfs_str_c(newpath_str)));
    if (rc == SFTFS_CACHE_OK)
        rc = sftfs_cache_give(cache, entry);
    return rc;
}
