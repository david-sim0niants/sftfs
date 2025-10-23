#pragma once

#include "abs/htable.h"

typedef const char *sftfs_cache_key;

struct sftfs_cache_node {
    sftfs_cache_key key;
    unsigned long mod_time;
    size_t i_next;
};

struct sftfs_cache_list {
    struct sftfs_cache_node *mru, *lru;
};

typedef struct sftfs_cache_entry_s {
    sftfs_cache_key key;
    char data[0];
} *sftfs_cache_entry;

struct sftfs_cache {
    void (*reader)(struct sftfs_cache *cache);
    void (*writer)(struct sftfs_cache *cache);
    sftfs_htable entrys_map;
    struct sftfs_cache_list list;
};
