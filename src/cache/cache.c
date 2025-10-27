#include "cache/cache.h"

#include <assert.h>
#include <string.h>

struct sftfs_cache *sftfs_cache_construct(struct sftfs_cache *cache,
        const struct sftfs_cache_config *config)
{
    cache->table = sftfs_htable_create(0);
    if (! cache->table)
        return NULL;
    sftfs_cache_list_construct(&cache->list, &config->list);
    cache->on_evict = config->on_evict;
    cache->entry_data_size = config->entry_data_size;
    return cache;
}

void sftfs_cache_destruct(struct sftfs_cache *cache)
{
    sftfs_cache_clear(cache);
    sftfs_cache_list_destruct(&cache->list);
    sftfs_htable_delete(cache->table);
}

static inline sftfs_cache_entry *from_htable_entry(sftfs_htable_entry entry)
{
    return (sftfs_cache_entry *)sftfs_htable_entry_data(entry);
}

static inline sftfs_htable_entry to_htable_entry(sftfs_cache_entry *entry)
{
    return (sftfs_htable_entry)entry->node.data;
}

static inline sftfs_htable_entry_ro to_htable_entry_ro(const sftfs_cache_entry *entry)
{
    return (sftfs_htable_entry_ro)entry->node.data;
}

static inline bool is_on_evict_set(const struct sftfs_cache_on_evict *on_evict)
{
    return !!on_evict->func;
}

static inline void call_on_evict(struct sftfs_cache_on_evict *on_evict, sftfs_cache_entry *entry)
{
    on_evict->func(entry->data, on_evict->user_data);
}

static void evict_invalid_lru_entries(struct sftfs_cache *cache)
{
    struct sftfs_cache_list_node *node = NULL;
    while ((node = sftfs_cache_list_evict_invalid_lru(&cache->list))) {
        sftfs_htable_entry entry = (sftfs_htable_entry)node->data;
        if (is_on_evict_set(&cache->on_evict))
            call_on_evict(&cache->on_evict, from_htable_entry(entry));
        sftfs_htable_find_and_remove(&cache->table, entry);
    }
}

static inline void self_refer(sftfs_htable_entry entry)
{
    // cache_entry->node.data holds a pointer to the entry that holds the cache entry
    from_htable_entry(entry)->node.data = entry;
}

sftfs_cache_entry *sftfs_cache_alloc(struct sftfs_cache *cache, size_t hash)
{
    evict_invalid_lru_entries(cache);

    const size_t entry_size = sizeof(sftfs_cache_entry) + cache->entry_data_size;
    sftfs_htable_entry_link entry_link = sftfs_htable_new_entry(&cache->table, hash, entry_size);
    if (! entry_link || !*entry_link)
        return NULL;

    sftfs_cache_entry *entry = from_htable_entry(*entry_link);
    sftfs_cache_list_reset_node(&entry->node);
    self_refer(*entry_link);
    return entry;
}

static inline bool unlisted_entry(const sftfs_cache_entry *entry)
{
    return entry->node.next == NULL && entry->node.prev == NULL;
}

void sftfs_cache_free(struct sftfs_cache *cache, sftfs_cache_entry *entry)
{
    assert(unlisted_entry(entry));
    sftfs_htable_find_and_remove(&cache->table, to_htable_entry(entry));
}

sftfs_cache_entry *sftfs_cache_take(struct sftfs_cache *cache, const sftfs_cache_entry *entry_ro)
{
    sftfs_htable_entry_link entry_link =
        sftfs_htable_find_entry_link(cache->table, to_htable_entry_ro(entry_ro));
    if (! entry_link || !*entry_link)
        return NULL;
    sftfs_cache_entry *entry = from_htable_entry(*entry_link);
    sftfs_cache_list_remove(&cache->list, &entry->node);
    return entry;
}

void sftfs_cache_give(struct sftfs_cache *cache, sftfs_cache_entry *entry)
{
    evict_invalid_lru_entries(cache);
    sftfs_cache_list_insert(&cache->list, &entry->node);
}

void sftfs_cache_invalidate(struct sftfs_cache *cache, sftfs_cache_entry *entry)
{
    assert(! unlisted_entry(entry));
    sftfs_cache_list_remove(&cache->list, &entry->node);
    sftfs_cache_free(cache, entry);
    evict_invalid_lru_entries(cache);
}

const sftfs_cache_entry *sftfs_cache_peek(struct sftfs_cache *cache, size_t hash)
{
    evict_invalid_lru_entries(cache);
    sftfs_htable_entry_link_ro entry_link = sftfs_htable_lookup_ro(cache->table, hash);
    if (! entry_link || !*entry_link)
        return NULL;
    return from_htable_entry(*entry_link);
}

const sftfs_cache_entry *sftfs_cache_peek_next(const sftfs_cache_entry *entry)
{
    sftfs_htable_entry_link_ro entry_link = sftfs_htable_lookup_next_ro(to_htable_entry_ro(entry));
    if (! entry_link || !*entry_link)
        return NULL;
    return from_htable_entry(*entry_link);
}

void sftfs_cache_clear(struct sftfs_cache *cache)
{
    if (is_on_evict_set(&cache->on_evict)) {
        sftfs_htable_entry_link entry_link = sftfs_htable_first_entry(cache->table);
        if (entry_link) {
            for (; *entry_link; entry_link = sftfs_htable_next_entry(cache->table, *entry_link))
                call_on_evict(&cache->on_evict, from_htable_entry(*entry_link));
        }
    }

    cache->list.lru = cache->list.mru = NULL;
    sftfs_htable_clear(&cache->table);
}
