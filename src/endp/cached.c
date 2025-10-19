#include "endp/cached_init.h"
#include "endp/cached_fs.h"

#include "abs/htable.h"
#include "abs/str.h"
#include "logging.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static inline struct sftfs_cached *get_cached(sftfs_endp endp)
{
    return (struct sftfs_cached *)(endp) - 1;
}

#define SFTFS_ENDP_OFF offsetof(struct sftfs_cached, base_endp)

sftfs_endp sftfs_cached_init(size_t base_endp_size, struct sftfs_cached_params *params)
{
    struct sftfs_cached *cached = calloc(1, SFTFS_ENDP_OFF + base_endp_size);
    if (cached && sftfs_cached_construct(cached, params))
        return &cached->base_endp;
    else if (cached)
        free(cached);
    return NULL;
}

void sftfs_cached_deinit(sftfs_endp endp)
{
    sftfs_cached_destruct(endp);
    free(get_cached(endp));
}

static inline void construct_fs(sftfs_cached_fs fs)
{
    fs->file_table = sftfs_htable_create(1);
}

static void del_all_files(sftfs_htable_ptr file_table);

static inline void destruct_fs(sftfs_cached_fs fs)
{
    del_all_files(&fs->file_table);
    sftfs_htable_delete(fs->file_table);
}

sftfs_endp sftfs_cached_construct(struct sftfs_cached *handle, struct sftfs_cached_params *params)
{
    (void)params;
    construct_fs(&handle->fs);
    return handle;
}

void sftfs_cached_destruct(sftfs_endp endp)
{
    destruct_fs(&get_cached(endp)->fs);
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

typedef struct sftfs_cached_file_s {
    sftfs_str path;
    struct stat attr;
} *sftfs_cached_file;
typedef const struct sftfs_cached_file_s *sftfs_cached_file_ro;

static inline sftfs_cached_file view_file(sftfs_htable_entry entry)
{
    return entry ? sftfs_htable_entry_data(entry) : NULL;
}

static inline sftfs_cached_file_ro view_file_ro(sftfs_htable_entry_ro entry)
{
    return entry ? sftfs_htable_entry_data_ro(entry) : NULL;
}

static sftfs_htable_entry_link find_file(sftfs_htable file_table, const char *path)
{
    sftfs_htable_entry_link entry_link = sftfs_htable_lookup(file_table, fnv1a_hash(path));
    for (; *entry_link; entry_link = sftfs_htable_lookup_next(*entry_link))
        if (str_eq(view_file(*entry_link)->path, path))
            break;
    return entry_link;
}

static sftfs_htable_entry_link
    new_file(sftfs_htable_ptr file_table, const char *path, const struct stat *attr)
{
    sftfs_htable_entry_link entry_link =
        sftfs_htable_new_entry(file_table, fnv1a_hash(path), sizeof(struct sftfs_cached_file_s));
    if (entry_link && *entry_link) {
        view_file(*entry_link)->path = sftfs_str_create(path, strlen(path));
        view_file(*entry_link)->attr = *attr;
    }
    return entry_link;
}

static void del_file(sftfs_htable_ptr file_table, sftfs_htable_entry_link entry_link)
{
    sftfs_str_delete(view_file(*entry_link)->path);
    sftfs_htable_remove(file_table, entry_link);
}

static void del_all_files(sftfs_htable_ptr file_table)
{
    for (size_t i = 0; i < sftfs_htable_nr_buckets(*file_table); ++i) {
        sftfs_htable_entry_link entry_link = sftfs_htable_get_bucket(*file_table, i);
        while (*entry_link)
            del_file(file_table, entry_link);
    }
}

bool sftfs_cached_fetch_getattr(sftfs_cached_fs fs, const char *path, struct stat *attr)
{
    sftfs_htable_entry_link entry_link = find_file(fs->file_table, path);

    sftfs_cached_file_ro file = view_file_ro(*entry_link);
    if (file) {
        *attr = file->attr;
        sftfs_info("Cache hit - getattr\n");
    // TODO: below line is temporary, create an LRU list and better handle lifetime of caches
        del_file(&fs->file_table, entry_link);
    // TEMPORARY WRONG CACHE HANDLING ABOVE
    } else {
        sftfs_info("Cache miss - getattr\n");
    }

    return !!file;
}

int sftfs_cached_cache_getattr(sftfs_cached_fs fs, const char *path, const struct stat *attr)
{
    sftfs_cached_file file = view_file(*find_file(fs->file_table, path));
    if (file) {
        file->attr = *attr;
        return SFTFS_CACHED_OK;
    }

    sftfs_htable_entry_link entry_link = new_file(&fs->file_table, path, attr);
    if (! *entry_link) {
        sftfs_error("Caching failed\n");
        return SFTFS_CACHED_FAIL;
    }
    return SFTFS_CACHED_OK;
}
