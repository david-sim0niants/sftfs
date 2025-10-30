#include "ut.h"
#include "cache/list.h"

struct fixture {
    struct sftfs_cache_list list;
};

static struct fixture create_fixture(sftfs_cache_time_t ttl)
{
    struct sftfs_cache_list_config config = {
        .ttl = ttl,
    };
    struct fixture f;
    sftfs_cache_list_construct(&f.list, &config);
    return f;
}

static void delete_fixture(struct fixture f)
{
    sftfs_cache_list_destruct(&f.list);
}

static int insert_first_node_succeeds(void)
{
    struct fixture f = create_fixture(1000);
    UT_BEGIN;

    UT_ASSERT(false);

UT_FINAL:
    delete_fixture(f);
    UT_END;
}

UT_ON_INIT
{
    static struct ut_test tests[] = {
        UT_LEAF_TEST("insert first node", insert_first_node_succeeds),
    };

    static struct ut_test test_suite = {"cache list ", NULL, tests, sizeof(tests) / sizeof(tests[0])};
    ut_register_tests(&test_suite, 1);
}
