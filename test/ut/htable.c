#include "abs/htable.h"
#include "ut.h"

#include <stdio.h>
#include <stdlib.h>

int insert_then_lookup_succeeds()
{
    sftfs_htable ht = sftfs_htable_create(0);

    UT_BEGIN;

    int x = 3;
    size_t hash = x;
    sftfs_htable_insert(&ht, hash, &x, sizeof(x));
    UT_EXPECT(*sftfs_htable_lookup(ht, x));
    UT_ASSERT(*(const int *)sftfs_htable_entry_data_ro(*sftfs_htable_lookup(ht, hash)) == x);

UT_FINAL:
    sftfs_htable_delete(ht);

    UT_END;
}

UT_ON_INIT
{
    static struct ut_test tests[] = {
        UT_LEAF_TEST("lookup() looks up what gets insert()'d", insert_then_lookup_succeeds),
    };
    static struct ut_test test_suite = { "htable ", NULL, tests, sizeof(tests) / sizeof(tests[0]) };
    ut_register_tests(&test_suite, 1);
}
