#include "ut.h"
#include "cache/file.h"

static _Thread_local sftfs_cache_time_t clock_time;

static sftfs_cache_time_t clock_mock(void)
{
    return clock_time;
}

struct fixture {
    struct sftfs_cache cache;
    char **paths;
    size_t nr_paths;
};

static void generate_data(char *data, size_t size)
{
    for (size_t i = 0; i < size; ++i)
        data[i] = rand() % 256;
}

static char *generate_path(void)
{
    const size_t size = rand() % 100 + 4;
    char *path = malloc(size + 1);
    path[0] = '/';

    for (size_t i = 1; i < size; ++i) {
        int r = rand() % (path[i - 1] == '/' ? 64 : 75);
        char c = 0;
        if (r < 10)
            c = '0' + r;
        else if (r < 36)
            c = 'a' + r - 10;
        else if (r < 36 + 26)
            c = 'A' + r - 36;
        else if (r < 36 + 26 + 2)
            c = r == 62 ? '.' : '-';
        else
            c = '/';
        path[i] = c;
    }
    path[size] = '\0';
    return path;
}

static struct fixture *create_fixture(sftfs_cache_time_t ttl, size_t data_size, int nr_files)
{
    struct sftfs_cache_file_config config = {
        .list = {
            .clock = clock_mock,
            .ttl = ttl,
        },
        .file_data_size = data_size,
    };
    struct fixture *f = calloc(1, sizeof(struct fixture));
    sftfs_cache_file_construct(&f->cache, &config);

    if (nr_files <= 0)
        return f;

    f->paths = calloc(nr_files, sizeof(char *));
    f->nr_paths = nr_files;
    for (int i = 0; i < nr_files; ++i) {
        f->paths[i] = generate_path();
        void *file_data = sftfs_cache_take_file(&f->cache, f->paths[i]);
        generate_data(file_data, data_size);
        sftfs_cache_give_file(&f->cache, f->paths[i], file_data);
    }

    return f;
}

static void delete_fixture(struct fixture *f)
{
    sftfs_cache_file_destruct(&f->cache);
    for (size_t i = 0; i < f->nr_paths; ++i)
        free(f->paths[i]);
    free(f->paths);
    f->nr_paths = 0;
    free(f);
}

static int take_give_non_exist(int nr_initial_nodes)
{
    const size_t size = rand() % 1000 + 1000;
    struct fixture *f = create_fixture(1000, size, nr_initial_nodes);
    struct sftfs_cache *cache = &f->cache;

    char *data = malloc(size);
    generate_data(data, size);

    UT_BEGIN;

    const char *path = "/foo/bar";
    char *file_data = sftfs_cache_take_file(cache, path);
    memcpy(file_data, data, size);
    UT_ASSERT(sftfs_cache_give_file(cache, path, file_data) == SFTFS_CACHE_FILE_OK);

    const char *file_data_ro = sftfs_cache_peek_file(cache, path);
    UT_ASSERT(file_data_ro);
    UT_ASSERT(memcmp(data, file_data_ro, size) == 0);

UT_FINAL:
    free(data);
    delete_fixture(f);
    UT_END;
}

static int take_give_first_entry(void)
{
    return take_give_non_exist(0);
}

static int take_give_non_exist_non_first(void)
{
    return take_give_non_exist(rand() % 90 + 10);
}

UT_ON_INIT
{
    srand(42);
    static struct ut_test tests[] = {
        UT_LEAF_TEST("take() and give() first time succeeds ", take_give_first_entry),
        UT_LEAF_TEST("take() and give() on non-existing path succeeds ", take_give_non_exist_non_first),
    };
    static struct ut_test test_suite = {"cache file ", NULL, tests, sizeof(tests) / sizeof(tests[0])};
    ut_register_tests(&test_suite, 1);
}
