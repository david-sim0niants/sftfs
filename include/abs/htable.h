#pragma once

#include <stddef.h>

typedef struct sftfs_htable_entry_s *sftfs_htable_entry;
typedef const struct sftfs_htable_entry_s *sftfs_htable_entry_ro;

typedef sftfs_htable_entry *sftfs_htable_entry_link;
typedef const sftfs_htable_entry *sftfs_htable_entry_link_ro;

typedef struct sftfs_htable_s *sftfs_htable;
typedef const struct sftfs_htable_s *sftfs_htable_ro;

typedef sftfs_htable *sftfs_htable_ptr;

sftfs_htable sftfs_htable_create(size_t nr_buckets);
void sftfs_htable_delete(sftfs_htable table);

sftfs_htable_entry_link sftfs_htable_new_entry(sftfs_htable_ptr table, size_t hash, size_t size);
sftfs_htable_entry_link sftfs_htable_insert(sftfs_htable_ptr table,
        size_t hash, const void *data, size_t size);

sftfs_htable_entry_link sftfs_htable_lookup(sftfs_htable table, size_t hash);
sftfs_htable_entry_link_ro sftfs_htable_lookup_ro(sftfs_htable_ro table, size_t hash);

sftfs_htable_entry_link sftfs_htable_lookup_next(sftfs_htable_entry_link entry_link);
sftfs_htable_entry_link_ro sftfs_htable_lookup_next_ro(sftfs_htable_entry_link_ro entry_link);

sftfs_htable_entry_link sftfs_htable_get_bucket(sftfs_htable table, size_t index);
sftfs_htable_entry_link_ro sftfs_htable_get_bucket_ro(sftfs_htable_ro table, size_t index);

sftfs_htable_entry_link sftfs_htable_next_entry(sftfs_htable_entry_link entry_link);
sftfs_htable_entry_link_ro sftfs_htable_next_entry_ro(sftfs_htable_entry_link_ro entry_link);

void sftfs_htable_remove(sftfs_htable_ptr table, sftfs_htable_entry_link entry_link);
void sftfs_htable_remove_hash(sftfs_htable_ptr table, size_t hash);
void sftfs_htable_clear(sftfs_htable_ptr table);

sftfs_htable_entry_link sftfs_htable_rehash_entry(sftfs_htable_ptr table, size_t new_hash,
        sftfs_htable_entry_link entry_link);

size_t sftfs_htable_nr_buckets(sftfs_htable_ro table);
size_t sftfs_htable_nr_entries(sftfs_htable_ro table);

size_t sftfs_htable_entry_hash(sftfs_htable_entry_ro entry);

void *sftfs_htable_entry_data(sftfs_htable_entry entry);
const void *sftfs_htable_entry_data_ro(sftfs_htable_entry_ro entry);
