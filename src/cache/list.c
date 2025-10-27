#include "cache/list.h"

#include <string.h>

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
    node->mod_time = list->config.clock();
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
    bool lru_invalid = list->lru->mod_time + list->config.ttl <= list->config.clock();
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
