#include "abs/htable.h"
#include "ut.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uintptr_t entry_data_t;

struct fixture {
    sftfs_htable table;
    size_t *hashes;
    entry_data_t *data;
    size_t nr_entries;
};

static struct fixture create_fixture(size_t nr_entries)
{
    sftfs_htable table = sftfs_htable_create(nr_entries);
    size_t *hashes = malloc(nr_entries * sizeof(size_t));
    entry_data_t *data = malloc(nr_entries * sizeof(entry_data_t));
    for (size_t i = 0; i < nr_entries; ++i) {
        hashes[i] = rand();
        data[i] = rand();
        UT_HARD_ASSERT(sftfs_htable_insert(&table, hashes[i], &data[i], sizeof(entry_data_t)));
    }
    UT_HARD_ASSERT(sftfs_htable_nr_entries(table) == nr_entries);
    return (struct fixture){table, hashes, data, nr_entries};
}

static void delete_fixture(struct fixture fixture)
{
    free(fixture.data);
    sftfs_htable_delete(fixture.table);
}

static int lookup_succeeds(void)
{
    struct fixture f = create_fixture(rand() % 200 + 100);
    sftfs_htable_ptr ht = &f.table;

    UT_BEGIN;

    size_t idx = rand() % f.nr_entries;
    sftfs_htable_entry_link entry_link = sftfs_htable_lookup(*ht, f.hashes[idx]);

    UT_ASSERT(*entry_link);

    UT_EXPECT(0 ==
              memcmp((entry_data_t *)sftfs_htable_entry_data_ro(*entry_link),
                     &f.data[idx], sizeof(entry_data_t)));

UT_FINAL:
    delete_fixture(f);
    UT_END;
}

static int remove_succeeds(void)
{
    struct fixture f = create_fixture(rand() % 200 + 100);
    sftfs_htable_ptr ht = &f.table;

    UT_BEGIN;

    size_t idx = rand() % f.nr_entries;
    sftfs_htable_entry_link entry_link = sftfs_htable_lookup(*ht, f.hashes[idx]);
    UT_ASSERT(*entry_link);
    sftfs_htable_remove(ht, entry_link);
    UT_EXPECT(f.nr_entries == sftfs_htable_nr_entries(*ht) + 1);

UT_FINAL:
    delete_fixture(f);
    UT_END;
}

static int remove_hash_succeeds(void)
{
    struct fixture f = create_fixture(rand() % 200 + 100);
    sftfs_htable_ptr ht = &f.table;

    UT_BEGIN;

    size_t idx = rand() % f.nr_entries;
    entry_data_t data = rand();
    UT_HARD_ASSERT(sftfs_htable_insert(ht, f.hashes[idx], &data, sizeof(entry_data_t)));
    UT_HARD_ASSERT(sftfs_htable_nr_entries(*ht) == f.nr_entries + 1);

    sftfs_htable_remove_hash(ht, f.hashes[idx]);
    UT_ASSERT(sftfs_htable_nr_entries(*ht) == f.nr_entries - 1);

UT_FINAL:
    delete_fixture(f);
    UT_END;
}

static int clear_succeeds(void)
{
    struct fixture f = create_fixture(rand() % 200 + 100);
    sftfs_htable_ptr ht = &f.table;

    UT_BEGIN;

    sftfs_htable_clear(ht);
    UT_ASSERT(sftfs_htable_nr_entries(*ht) == 0);

UT_FINAL:
    delete_fixture(f);
    UT_END;
}

UT_ON_INIT
{
    srand(42);
    static struct ut_test tests[] = {
        UT_LEAF_TEST("lookup() looks up an existing entry", lookup_succeeds),
        UT_LEAF_TEST("remove() with lookup() removes an existing entry", remove_succeeds),
        UT_LEAF_TEST("remove_hash() removes all entries with same hash", remove_hash_succeeds),
        UT_LEAF_TEST("clear() removes all entries", clear_succeeds),
    };
    static struct ut_test test_suite = { "htable ", NULL, tests, sizeof(tests) / sizeof(tests[0]) };
    ut_register_tests(&test_suite, 1);
}
