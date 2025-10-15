#pragma once

#include <stddef.h>

typedef struct sftfs_htable_entry_s *sftfs_htable_entry;
typedef const struct sftfs_htable_entry_s *sftfs_htable_entry_ro;

typedef sftfs_htable_entry *sftfs_htable_entry_link;
typedef const sftfs_htable_entry *sftfs_htable_entry_link_ro;

typedef struct sftfs_htable_s *sftfs_htable;
typedef const struct sftfs_htable_s *sftfs_htable_ro;

sftfs_htable sftfs_htable_create(size_t nr_buckets);
void sftfs_htable_delete(sftfs_htable table);
sftfs_htable sftfs_htable_insert(sftfs_htable table, size_t hash, void *data, size_t size);

sftfs_htable_entry_link sftfs_htable_lookup(sftfs_htable table, size_t hash);
sftfs_htable_entry_link_ro sftfs_htable_lookup_ro(sftfs_htable_ro table, size_t hash);

sftfs_htable_entry_link sftfs_htable_lookup_next(sftfs_htable_entry_link entry_link);
sftfs_htable_entry_link_ro sftfs_htable_lookup_next_ro(sftfs_htable_entry_link_ro entry_link);

sftfs_htable sftfs_htable_remove(sftfs_htable table, sftfs_htable_entry_link entry_link);
sftfs_htable sftfs_htable_remove_hash(sftfs_htable table, size_t hash);
sftfs_htable sftfs_htable_clear(sftfs_htable table);

sftfs_htable sftfs_htable_size(sftfs_htable_ro size);
