#include "abs/htable.h"
#include "abs/htable_primes.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct sftfs_htable_entry_s {
    struct sftfs_htable_entry_s *next;
    size_t hash;
    char payload[0];
};

struct sftfs_htable_s {
    size_t nr_buckets;
    size_t nr_entries;
    sftfs_htable_entry buckets[0];
};

static size_t prime_lower_bound(size_t target)
{
    int left = 0,
        right = sizeof(sftfs_htable_prime_list) / sizeof(sftfs_htable_prime_list[0]) - 1;
    while (left < right) {
        const int mid = (right - left) / 2 + left;
        const unsigned long midval = sftfs_htable_prime_list[mid];
        if (midval < target)
            left = mid + 1;
        else
            right = mid;
    }
    return sftfs_htable_prime_list[left];
}

static sftfs_htable create_htable(size_t nr_buckets)
{
    const size_t buckets_size = sizeof(struct sftfs_htable_entry_s) * nr_buckets;
    sftfs_htable table = calloc(1, sizeof(struct sftfs_htable_s) + buckets_size);
    if (NULL == table)
        return NULL;

    table->nr_buckets = nr_buckets;
    table->nr_entries = 0;
    return table;
}

sftfs_htable sftfs_htable_create(size_t nr_buckets)
{
    return create_htable(prime_lower_bound(nr_buckets));
}

void sftfs_htable_delete(sftfs_htable table)
{
    sftfs_htable_clear(&table);
    free(table);
}

static sftfs_htable_entry create_entry(size_t hash, size_t size)
{
    sftfs_htable_entry entry = malloc(sizeof(*entry) + size);
    if (! entry)
        return NULL;
    entry->next = NULL;
    entry->hash = hash;
    return entry;
}

static void delete_entry(sftfs_htable_entry entry)
{
    free(entry);
}

static sftfs_htable_entry_link insert_entry(sftfs_htable table, sftfs_htable_entry entry)
{
    sftfs_htable_entry_link bucket = &table->buckets[entry->hash % table->nr_buckets];
    entry->next = *bucket;
    *bucket = entry;
    ++table->nr_entries;
    return bucket;
}

static sftfs_htable_entry extract_entry(sftfs_htable table, sftfs_htable_entry_link entry_link)
{
    assert(table->nr_entries > 0);
    sftfs_htable_entry entry = *entry_link;

    *entry_link = entry->next;
    entry->next = NULL;
    --table->nr_entries;

    return entry;
}

static void migrate(sftfs_htable from, sftfs_htable to)
{
    for (size_t i = 0; i < from->nr_buckets; ++i) {
        sftfs_htable_entry_link entry_link = &from->buckets[i];
        while (*entry_link)
            insert_entry(to, extract_entry(from, entry_link));
    }
}

static sftfs_htable extend_and_rehash(sftfs_htable table)
{
    sftfs_htable new_table = sftfs_htable_create(table->nr_buckets * 2);
    migrate(table, new_table);
    sftfs_htable_delete(table);
    return new_table;
}

sftfs_htable_entry_link sftfs_htable_new_entry(sftfs_htable_ptr table_ptr, size_t hash, size_t size)
{
    sftfs_htable table = *table_ptr;

    sftfs_htable_entry entry = create_entry(hash, size);
    if (NULL == entry)
        return NULL;

    if ((table->nr_buckets / 2 + table->nr_buckets) / 2 < table->nr_entries)
        table = extend_and_rehash(table);

    if (NULL == table) {
        delete_entry(entry);
        return NULL;
    }

    *table_ptr = table;
    return insert_entry(table, entry);
}

sftfs_htable_entry_link sftfs_htable_insert(
        sftfs_htable_ptr table, size_t hash,
        const void *data, size_t size)
{
    sftfs_htable_entry_link entry_link = sftfs_htable_new_entry(table, hash, size);
    if (entry_link && *entry_link)
        memcpy(entry_link[0]->payload, data, size);
    return entry_link;
}

#define FIND_ENTRY_WITH_HASH(entry_link, hash) \
    for (; *entry_link; entry_link = &(*entry_link)->next) \
        if ((*entry_link)->hash == hash) \
            break;

sftfs_htable_entry_link sftfs_htable_lookup(sftfs_htable table, size_t hash)
{
    sftfs_htable_entry_link entry_link = &table->buckets[hash % table->nr_buckets];
    FIND_ENTRY_WITH_HASH(entry_link, hash);
    return entry_link;
}

sftfs_htable_entry_link_ro sftfs_htable_lookup_ro(sftfs_htable_ro table, size_t hash)
{
    sftfs_htable_entry_link_ro entry_link = &table->buckets[hash % table->nr_buckets];
    FIND_ENTRY_WITH_HASH(entry_link, hash);
    return entry_link;
}

sftfs_htable_entry_link sftfs_htable_lookup_next(sftfs_htable_entry entry)
{
    const size_t hash = entry->hash;
    sftfs_htable_entry_link entry_link = &entry->next;
    FIND_ENTRY_WITH_HASH(entry_link, hash);
    return entry_link;
}

sftfs_htable_entry_link_ro sftfs_htable_lookup_next_ro(sftfs_htable_entry_ro entry)
{
    const size_t hash = entry->hash;
    sftfs_htable_entry_link_ro entry_link = &entry->next;
    FIND_ENTRY_WITH_HASH(entry_link, hash);
    return entry_link;
}

sftfs_htable_entry_link sftfs_htable_get_bucket(sftfs_htable table, size_t index)
{
    return &table->buckets[index];
}

sftfs_htable_entry_link_ro sftfs_htable_get_bucket_ro(sftfs_htable_ro table, size_t index)
{
    return &table->buckets[index];
}

sftfs_htable_entry_link sftfs_htable_first_entry(sftfs_htable table)
{
    for (size_t i = 0; i < table->nr_buckets; ++i)
        if (table->buckets[i])
            return &table->buckets[i];
    return NULL;
}

sftfs_htable_entry_link_ro sftfs_htable_first_entry_ro(sftfs_htable_ro table)
{
    for (size_t i = 0; i < table->nr_buckets; ++i)
        if (table->buckets[i])
            return &table->buckets[i];
    return NULL;
}

sftfs_htable_entry_link sftfs_htable_next_entry(sftfs_htable table, sftfs_htable_entry entry)
{
    sftfs_htable_entry_link entry_link = NULL;
    if (entry) {
        entry_link = &entry->next;
        if (*entry_link)
            return entry_link;
    }

    size_t index = 0;
    if (entry_link)
        index = (entry_link[0]->hash % table->nr_buckets) + 1;

    for (; index < table->nr_buckets; ++index)
        if (table->buckets[index])
            return &table->buckets[index];

    return NULL;
}

sftfs_htable_entry_link_ro sftfs_htable_next_entry_ro(sftfs_htable_ro table, sftfs_htable_entry_ro entry)
{
    sftfs_htable_entry_link_ro entry_link = NULL;
    if (entry) {
        entry_link = &entry->next;
        if (*entry_link)
            return entry_link;
    }

    size_t index = 0;
    if (entry_link)
        index = (entry_link[0]->hash % table->nr_buckets) + 1;

    for (; index < table->nr_buckets; ++index)
        if (table->buckets[index])
            return &table->buckets[index];

    return NULL;
}

sftfs_htable_entry_link sftfs_htable_find_entry_link(sftfs_htable table, sftfs_htable_entry_ro entry)
{
    sftfs_htable_entry_link entry_link = sftfs_htable_lookup(table, entry->hash);
    for (; *entry_link; entry_link = sftfs_htable_lookup_next(*entry_link))
        if (*entry_link == entry)
            break;
    return entry_link;
}

sftfs_htable_entry_link_ro
    sftfs_htable_find_entry_link_ro(sftfs_htable_ro table, sftfs_htable_entry_ro entry)
{
    sftfs_htable_entry_link_ro entry_link = sftfs_htable_lookup_ro(table, entry->hash);
    for (; *entry_link; entry_link = sftfs_htable_lookup_next_ro(*entry_link))
        if (*entry_link == entry)
            break;
    return entry_link;
}

void sftfs_htable_remove(sftfs_htable_ptr table, sftfs_htable_entry_link entry_link)
{
    delete_entry(extract_entry(*table, entry_link));
}

void sftfs_htable_remove_hash(sftfs_htable_ptr table, size_t hash)
{
    sftfs_htable_entry_link entry_link = sftfs_htable_lookup(*table, hash);
    while (*entry_link)
        sftfs_htable_remove(table, entry_link);
}

void sftfs_htable_clear(sftfs_htable_ptr table)
{
    for (size_t i = 0; i < (*table)->nr_buckets; ++i) {
        sftfs_htable_entry entry = (*table)->buckets[i];
        while (entry) {
            sftfs_htable_entry next_entry = entry->next;
            delete_entry(entry);
            entry = next_entry;
        }
    }
}

sftfs_htable_entry_link sftfs_htable_rehash_entry(sftfs_htable_ptr table, size_t new_hash,
        sftfs_htable_entry_link entry_link)
{
    sftfs_htable_entry entry = extract_entry(*table, entry_link);
    assert(entry);
    entry->hash = new_hash;
    return insert_entry(*table, entry);
}

size_t sftfs_htable_nr_buckets(sftfs_htable_ro table)
{
    return table->nr_buckets;
}

size_t sftfs_htable_nr_entries(sftfs_htable_ro table)
{
    return table->nr_entries;
}

size_t sftfs_htable_entry_hash(sftfs_htable_entry_ro entry)
{
    return entry->hash;
}

void *sftfs_htable_entry_data(sftfs_htable_entry entry)
{
    return entry->payload;
}

const void *sftfs_htable_entry_data_ro(sftfs_htable_entry_ro entry)
{
    return entry->payload;
}
