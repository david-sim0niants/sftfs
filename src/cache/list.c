#include "cache/list.h"

#include <assert.h>
#include <time.h>
#include <string.h>

#ifndef NDEBUG
static inline void construct_debug(struct sftfs_cache_list *list)
{
    list->nr_nodes = 0;
}

static inline void increment_nr_nodes(struct sftfs_cache_list *list)
{
    ++list->nr_nodes;
}

static inline void decrement_nr_nodes(struct sftfs_cache_list *list)
{
    assert(list->nr_nodes > 0);
    --list->nr_nodes;
}
#else
static inline void construct_debug(struct sftfs_cache_list *list) { (void)list; }
static inline void increment_nr_nodes(struct sftfs_cache_list *list) { (void)list; }
static inline void decrement_nr_nodes(struct sftfs_cache_list *list) { (void)list; }
#endif

static inline sftfs_cache_time_t default_cache_clock(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL;
}

static inline sftfs_cache_time_t get_cache_time(const struct sftfs_cache_list *list)
{
    if (list->config.clock)
        return list->config.clock();
    else
        return default_cache_clock();
}

void sftfs_cache_list_construct(struct sftfs_cache_list *list,
        const struct sftfs_cache_list_config *config)
{
    list->lru = list->mru = NULL;
    list->config = *config;
    construct_debug(list);
}

void sftfs_cache_list_destruct(struct sftfs_cache_list *list)
{
    list->lru = list->mru = NULL;
    memset(&list->config, 0, sizeof(list->config));
}

void sftfs_cache_list_insert(struct sftfs_cache_list *list, struct sftfs_cache_list_node *node)
{
    if (list->mru) {
        list->mru->next = node;
        node->prev = list->mru;
    } else {
        list->lru = list->mru = node;
        node->prev = NULL;
    }
    node->next = NULL;
    list->mru = node;
    node->mod_time = get_cache_time(list);
    increment_nr_nodes(list);
}

void sftfs_cache_list_remove(struct sftfs_cache_list *list, struct sftfs_cache_list_node *node)
{
    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;

    if (list->lru == node && list->mru == node) {
        list->lru = list->mru = NULL;
    } else {
        if (list->lru == node)
            list->lru = node->next;
        if (list->mru == node)
            list->mru = node->prev;
    }

    sftfs_cache_list_reset_node(node);
    decrement_nr_nodes(list);
}

/* Assumes non-empty list. */
static struct sftfs_cache_list_node *evict_lru(struct sftfs_cache_list *list)
{
    struct sftfs_cache_list_node *evicted = list->lru;

    list->lru = evicted->next;
    if (NULL == list->lru)
        list->mru = NULL;
    else
        list->lru->prev = NULL;

    sftfs_cache_list_reset_node(evicted);
    decrement_nr_nodes(list);
    return evicted;
}

static inline bool lru_invalid(struct sftfs_cache_list *list)
{
    return list->lru->mod_time + list->config.ttl < get_cache_time(list);
}

struct sftfs_cache_list_node *sftfs_cache_list_evict_invalid_lru(struct sftfs_cache_list *list)
{
    if (sftfs_cache_list_empty(list))
        return NULL;
    else if (lru_invalid(list))
        return evict_lru(list);
    else
        return NULL;
}
