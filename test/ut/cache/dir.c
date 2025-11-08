#include "ut.h"

#include "cache/dir.h"
#include "cache/cache.h"

static _Thread_local sftfs_cache_time_t clock_time;

static sftfs_cache_time_t clock_mock(void)
{
    return clock_time;
}

struct fixture {
    struct sftfs_cache cache;
};

static struct fixture *create_fixture(sftfs_cache_time_t ttl, int nr_files)
{
    struct fixture *f = calloc(1, sizeof(struct fixture));
    struct sftfs_cache_dir_config config = {
        .list = {
            .clock = clock_mock,
            .ttl = ttl,
        }
    };
    sftfs_cache_dir_construct(&f->cache, &config);

    if (nr_files <= 0)
        return f;
    return f;
}

static void delete_fixture(struct fixture *f)
{
    sftfs_cache_dir_destruct(&f->cache);
    free(f);
}

static int take_fill_give_dir_ok(void)
{
    struct fixture *f = create_fixture(1000, 0);

    UT_BEGIN;

    const char *path = "/foo/bar";
    sftfs_cache_dir *dir = sftfs_cache_take_dir(&f->cache, path);
    UT_ASSERT(dir);

    const char *entries[] = {".", "..", "baz", "abc"};
    const size_t nr_entries = sizeof(entries) / sizeof(entries[0]);
    for (size_t i = 0; i < nr_entries; ++i)
        UT_ASSERT(sftfs_cache_add_dir_entry(dir, entries[i]) == SFTFS_CACHE_DIR_OK);

    UT_ASSERT(sftfs_cache_give_dir(&f->cache, path, dir) == SFTFS_CACHE_DIR_OK);

    sftfs_cache_dir_handle dir_h = sftfs_cache_peek_dir(&f->cache, path);
    UT_ASSERT(sftfs_cache_dir_valid(dir_h));

    size_t i = 0;
    const char *entry = NULL;
    while ((entry = sftfs_cache_read_dir(&dir_h))) {
        UT_ASSERT(i < nr_entries);
        UT_ASSERT(strcmp(entries[i], entry) == 0);
        ++i;
    }

UT_FINAL:
    delete_fixture(f);
    UT_END;
}

UT_ON_INIT
{
    srand(42);
    static struct ut_test tests[] = {
        UT_LEAF_TEST("take() dir, fill it and give() back succeeds", take_fill_give_dir_ok),
    };
    static struct ut_test test_suite = {"cache dir ", NULL, tests, sizeof(tests) / sizeof(tests[0])};
    ut_register_tests(&test_suite, 1);
}
