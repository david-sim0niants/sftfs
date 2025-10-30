#include "cache/list.h"
#include <time.h>

#include <string.h>

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
}

void sftfs_cache_list_remove(struct sftfs_cache_list *list, struct sftfs_cache_list_node *node)
{
    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;
    if (list->lru == node)
        list->lru = node->next;
    if (list->mru == node)
        list->mru = node->prev;
    if (NULL == list->lru)
        list->mru = NULL;

    sftfs_cache_list_reset_node(node);
}

struct sftfs_cache_list_node *sftfs_cache_list_evict_invalid_lru(struct sftfs_cache_list *list)
{
    if (sftfs_cache_list_empty(list))
        return NULL;
    bool lru_invalid = list->lru->mod_time + list->config.ttl < get_cache_time(list);
    if (lru_invalid) {
        struct sftfs_cache_list_node *evicted = list->lru;
        list->lru = list->lru->next;
        if (NULL == list->lru)
            list->mru = NULL;
        sftfs_cache_list_reset_node(evicted);
        return evicted;
    } else {
        return NULL;
    }
}
