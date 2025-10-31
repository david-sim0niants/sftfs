#include "ut.h"
#include "cache/list.h"

struct fixture {
    struct sftfs_cache_list list;
    struct sftfs_cache_list_node *initial_nodes;
};

static _Thread_local sftfs_cache_time_t clock_time;

static sftfs_cache_time_t clock_mock(void)
{
    return clock_time;
}

static struct fixture create_fixture(sftfs_cache_time_t ttl, int nr_nodes)
{
    struct sftfs_cache_list_config config = {
        .clock = clock_mock,
        .ttl = ttl,
    };
    struct fixture f = {0};
    sftfs_cache_list_construct(&f.list, &config);
    UT_HARD_ASSERT(sftfs_cache_list_empty(&f.list));

    if (0 >= nr_nodes)
        return f;

    clock_time = 0;
    f.initial_nodes = malloc(nr_nodes * sizeof(struct sftfs_cache_list_node));
    while (nr_nodes--) {
        struct sftfs_cache_list_node *node = &f.initial_nodes[nr_nodes];
        sftfs_cache_list_insert(&f.list, node);
        node->data = (void *)(long)nr_nodes;
        ++clock_time;
    }

    return f;
}

static void delete_fixture(struct fixture f)
{
    free(f.initial_nodes);
    sftfs_cache_list_destruct(&f.list);
}

static int insert_first_node_succeeds(void)
{
    const int ttl = 1000;
    struct fixture f = create_fixture(ttl, 0);
    struct sftfs_cache_list *list = &f.list;

    UT_BEGIN;

    struct sftfs_cache_list_node node = {0};
    sftfs_cache_list_insert(list, &node);
    UT_ASSERT(! sftfs_cache_list_empty(list));

    UT_EXPECT(node.prev == NULL);
    UT_EXPECT(node.next == NULL);
    UT_EXPECT(node.mod_time == clock_time);

    clock_time += ttl + 1;

    UT_ASSERT(sftfs_cache_list_evict_invalid_lru(list) == &node);
    UT_EXPECT(sftfs_cache_list_empty(list));

UT_FINAL:
    delete_fixture(f);
    UT_END;
}

static int insert_node_succeeds(void)
{
    const int ttl = 1000;
    int nr_nodes = 12;
    struct fixture f = create_fixture(ttl, nr_nodes);
    struct sftfs_cache_list *list = &f.list;

    UT_BEGIN;

    struct sftfs_cache_list_node node = {0};
    sftfs_cache_list_insert(list, &node);

    UT_EXPECT(node.next == NULL);
    UT_EXPECT(node.mod_time == clock_time);

    clock_time += ttl + 1;

    while (nr_nodes--)
        UT_HARD_ASSERT(sftfs_cache_list_evict_invalid_lru(list));

    UT_ASSERT(sftfs_cache_list_evict_invalid_lru(list) == &node);
    UT_EXPECT(sftfs_cache_list_empty(list));

UT_FINAL:
    delete_fixture(f);
    UT_END;
}

static int remove_node_succeeds(void)
{
    struct fixture f = create_fixture(1000, rand() % 100);
    struct sftfs_cache_list *list = &f.list;

    UT_BEGIN;

    (void)list;
    UT_ASSERT(false); // TODO

UT_FINAL:
    delete_fixture(f);
    UT_END;
}

UT_ON_INIT
{
    srand(42);

    static struct ut_test tests[] = {
        UT_LEAF_TEST("insert() first node succeeds", insert_first_node_succeeds),
        UT_LEAF_TEST("insert() node succeeds", insert_node_succeeds),
        UT_LEAF_TEST("remove() node succeeds", remove_node_succeeds),
    };

    static struct ut_test test_suite = {"cache list ", NULL, tests, sizeof(tests) / sizeof(tests[0])};
    ut_register_tests(&test_suite, 1);
}
