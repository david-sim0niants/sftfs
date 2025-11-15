#pragma once

#include <time.h>
#include <stdint.h>

#include "list.h"
#include "abs/htable.h"

typedef struct sftfs_cache_entry_s {
    struct sftfs_cache_list_node node;
    char data[0];
} sftfs_cache_entry;

struct sftfs_cache_on_evict {
    void (*func)(void *entry_data, void *user_data);
    void *user_data;
};

struct sftfs_cache {
    sftfs_htable table;
    struct sftfs_cache_list list;
    struct sftfs_cache_on_evict on_evict;
    size_t entry_data_size;
};

struct sftfs_cache_config {
    struct sftfs_cache_list_config list;
    struct sftfs_cache_on_evict on_evict;
    size_t entry_data_size;
};

enum {
    SFTFS_CACHE_OK = 0,
    SFTFS_CACHE_UNEXPECTED_ENTRY,
    SFTFS_CACHE_ERROR_MAX = SFTFS_CACHE_UNEXPECTED_ENTRY,
};

struct sftfs_cache *sftfs_cache_construct(struct sftfs_cache *cache,
        const struct sftfs_cache_config *config);
void sftfs_cache_destruct(struct sftfs_cache *cache);

sftfs_cache_entry *sftfs_cache_alloc(struct sftfs_cache *cache, size_t hash);
int sftfs_cache_free(struct sftfs_cache *cache, sftfs_cache_entry *entry);

sftfs_cache_entry *sftfs_cache_take(struct sftfs_cache *cache, const sftfs_cache_entry *peeked_entry);
int sftfs_cache_give(struct sftfs_cache *cache, sftfs_cache_entry *entry);

int sftfs_cache_invalidate(struct sftfs_cache *cache, const sftfs_cache_entry *entry);

int sftfs_cache_rehash_entry(struct sftfs_cache *cache, sftfs_cache_entry *entry, size_t new_hash);

const sftfs_cache_entry *sftfs_cache_peek(struct sftfs_cache *cache, size_t hash);
const sftfs_cache_entry *sftfs_cache_peek_next(const sftfs_cache_entry *entry);

bool sftfs_cache_contains(const struct sftfs_cache *cache, const sftfs_cache_entry *entry);
bool sftfs_cache_contains_unlisted(const struct sftfs_cache *cache, const sftfs_cache_entry *entry);

void sftfs_cache_clear(struct sftfs_cache *cache);

static inline void *sftfs_cache_entry_data(sftfs_cache_entry *entry)
{
    return entry->data;
}

static inline const void *sftfs_cache_entry_data_ro(const sftfs_cache_entry *entry)
{
    return entry->data;
}
