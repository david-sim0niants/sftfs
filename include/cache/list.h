#pragma once

#include <stdbool.h>
#include <string.h>

typedef unsigned long sftfs_cache_time_t; // milliseconds
typedef sftfs_cache_time_t (*sftfs_cache_clock_t)();

struct sftfs_cache_list_node {
    struct sftfs_cache_list_node *prev, *next;
    sftfs_cache_time_t mod_time;
    void *data;
};

struct sftfs_cache_list_config {
    sftfs_cache_clock_t clock;
    sftfs_cache_time_t ttl;
};

struct sftfs_cache_list {
    struct sftfs_cache_list_node *lru, *mru;
    struct sftfs_cache_list_config config;
#ifndef NDEBUG
    size_t nr_nodes;
#endif
};

void sftfs_cache_list_construct(struct sftfs_cache_list *list,
        const struct sftfs_cache_list_config *config);
void sftfs_cache_list_destruct(struct sftfs_cache_list *list);
void sftfs_cache_list_insert(struct sftfs_cache_list *list, struct sftfs_cache_list_node *node);
void sftfs_cache_list_remove(struct sftfs_cache_list *list, struct sftfs_cache_list_node *node);
struct sftfs_cache_list_node *sftfs_cache_list_evict_invalid_lru(struct sftfs_cache_list *list);

static inline
void sftfs_cache_list_refresh(struct sftfs_cache_list *list, struct sftfs_cache_list_node *node)
{
    sftfs_cache_list_remove(list, node);
    sftfs_cache_list_insert(list, node);
}

static inline bool sftfs_cache_list_empty(const struct sftfs_cache_list *list)
{
    return NULL == list->lru;
}

static inline bool sftfs_cache_list_node_unlinked(
        const struct sftfs_cache_list *list,
        const struct sftfs_cache_list_node *node)
{
    return node->prev == NULL && node->next == NULL && node != list->lru;
}

static inline void sftfs_cache_list_reset_node(struct sftfs_cache_list_node *node)
{
    node->prev = node->next = NULL;
    node->mod_time = 0;
}
