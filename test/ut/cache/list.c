#include "ut.h"
#include "cache/list.h"

struct fixture {
    struct sftfs_cache_list list;
    struct sftfs_cache_list_node *initial_nodes;
    struct sftfs_cache_list_node *initial_lru;
    struct sftfs_cache_list_node *initial_mru;
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

    f.initial_lru = &f.initial_nodes[nr_nodes - 1];
    f.initial_mru = &f.initial_nodes[0];

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

static int remove_middle_node_succeeds(void)
{
    const int ttl = 1000;
    int nr_nodes = rand() % 100 + 50;
    struct fixture f = create_fixture(ttl, nr_nodes);
    struct sftfs_cache_list *list = &f.list;

    UT_BEGIN;

    struct sftfs_cache_list_node *node = &f.initial_nodes[nr_nodes / 2];

    UT_ASSERT(node->prev != NULL);
    UT_ASSERT(node->next != NULL);

    sftfs_cache_list_remove(list, node);
    UT_ASSERT(node->prev == NULL);
    UT_ASSERT(node->next == NULL);

    clock_time += ttl + 1;

    while (--nr_nodes)
        UT_HARD_ASSERT(sftfs_cache_list_evict_invalid_lru(list));
    UT_HARD_ASSERT(NULL == sftfs_cache_list_evict_invalid_lru(list));

    UT_EXPECT(sftfs_cache_list_empty(list));

UT_FINAL:
    delete_fixture(f);
    UT_END;
}

static int remove_lru_node_succeeds(void)
{
    const int ttl = 1000;
    int nr_nodes = rand() % 100 + 50;
    struct fixture f = create_fixture(ttl, nr_nodes);
    struct sftfs_cache_list *list = &f.list;

    UT_BEGIN;

    clock_time += ttl;
    struct sftfs_cache_list_node *expected_lru = f.initial_lru->next;

    UT_HARD_ASSERT(expected_lru->prev != NULL);

    sftfs_cache_list_remove(list, f.initial_lru);

    UT_ASSERT(sftfs_cache_list_evict_invalid_lru(list) == expected_lru);
    UT_EXPECT(expected_lru->prev == NULL);

UT_FINAL:
    delete_fixture(f);
    UT_END;
}

static int remove_mru_node_succeeds(void)
{
    const int ttl = 1000;
    int nr_nodes = rand() % 100 + 50;
    struct fixture f = create_fixture(ttl, nr_nodes);
    struct sftfs_cache_list *list = &f.list;

    UT_BEGIN;

    clock_time += ttl;
    struct sftfs_cache_list_node *expected_mru = f.initial_mru->prev;

    UT_HARD_ASSERT(expected_mru->next != NULL);

    sftfs_cache_list_remove(list, f.initial_mru);
    --nr_nodes;
    while (--nr_nodes)
        UT_HARD_ASSERT(sftfs_cache_list_evict_invalid_lru(list));

    UT_ASSERT(sftfs_cache_list_evict_invalid_lru(list) == expected_mru);
    UT_EXPECT(expected_mru->next == NULL);

UT_FINAL:
    delete_fixture(f);
    UT_END;
}

static int remove_last_node_succeeds(void)
{
    struct fixture f = create_fixture(1000, 1);
    struct sftfs_cache_list *list = &f.list;

    UT_BEGIN;

    sftfs_cache_list_remove(list, &f.initial_nodes[0]);
    UT_ASSERT(sftfs_cache_list_empty(list));

UT_FINAL:
    delete_fixture(f);
    UT_END;
}

static int evict_invalid_lru_nodes_succeeds(void)
{
    const int ttl = rand() % 1000 + 800;
    int nr_nodes = rand() % 100 + 50;
    struct fixture f = create_fixture(ttl, nr_nodes);
    struct sftfs_cache_list *list = &f.list;

    UT_BEGIN;

    clock_time = ttl;

    for (int i = 0; i < nr_nodes; ++i) {
        UT_ASSERT(! sftfs_cache_list_evict_invalid_lru(list));
        ++clock_time;
        UT_ASSERT(sftfs_cache_list_evict_invalid_lru(list));
    }

    UT_EXPECT(sftfs_cache_list_empty(list));
    UT_EXPECT(! sftfs_cache_list_evict_invalid_lru(list));

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
        UT_LEAF_TEST("remove() node succeeds", remove_middle_node_succeeds),
        UT_LEAF_TEST("remove() lru node succeeds", remove_lru_node_succeeds),
        UT_LEAF_TEST("remove() mru node succeeds", remove_mru_node_succeeds),
        UT_LEAF_TEST("remove() last node succeeds", remove_last_node_succeeds),
        UT_LEAF_TEST("evict_invalid_lru() nodes succeeds", evict_invalid_lru_nodes_succeeds),
    };

    static struct ut_test test_suite = {"cache list ", NULL, tests, sizeof(tests) / sizeof(tests[0])};
    ut_register_tests(&test_suite, 1);
}
