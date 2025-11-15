#include "abs/str.h"
#include "endp/cached_init.h"
#include "endp/cached.h"

#include "cache/attr.h"
#include "cache/dir.h"

#include "func_trace.h"
#include "logging.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static inline struct sftfs_cached_endp *get_cached(sftfs_endp endp)
{
    return (struct sftfs_cached_endp *)(endp) - 1;
}

#define SFTFS_ENDP_OFF offsetof(struct sftfs_cached_endp, base_endp)

sftfs_endp sftfs_cached_init(size_t base_endp_size, struct sftfs_cached_params *params)
{
    struct sftfs_cached_endp *cached = calloc(1, SFTFS_ENDP_OFF + base_endp_size);
    if (cached && sftfs_cached_construct(cached, params))
        return &cached->base_endp;
    else if (cached)
        free(cached);
    return NULL;
}

void sftfs_cached_deinit(sftfs_endp endp)
{
    sftfs_cached_destruct(endp);
    free(get_cached(endp));
}

sftfs_endp sftfs_cached_construct(struct sftfs_cached_endp *handle, struct sftfs_cached_params *params)
{
    struct sftfs_cache_attr_config attr_config = {
        .list = {
            params->clock,
            params->attr_ttl,
        },
    };
    if (NULL == sftfs_cache_attr_construct(&handle->attr_cache, &attr_config))
        goto attr_cache_construct_failed;

    struct sftfs_cache_dir_config dir_config = {
        .list = {
            params->clock,
            params->dir_ttl,
        },
    };
    if (NULL == sftfs_cache_dir_construct(&handle->dir_cache, &dir_config))
        goto dir_cache_construct_failed;

    handle->handle_path_map = sftfs_htable_create(0);
    if (NULL == handle->handle_path_map)
        goto handle_path_map_create_failed;

    return handle->base_endp;

handle_path_map_create_failed:
    sftfs_cache_dir_destruct(&handle->dir_cache);
dir_cache_construct_failed:
    sftfs_cache_attr_destruct(&handle->attr_cache);
attr_cache_construct_failed:
    return NULL;
}

void sftfs_cached_destruct(sftfs_endp endp)
{
    sftfs_htable_delete(get_cached(endp)->handle_path_map);
    sftfs_cache_dir_destruct(&get_cached(endp)->dir_cache);
    sftfs_cache_attr_destruct(&get_cached(endp)->attr_cache);
}

void sftfs_cached_inval_all(struct sftfs_cached_endp *endp, const char *path)
{
    SFTFS_TRACE_FUNC
    sftfs_cache_invalidate_attr(&endp->attr_cache, path);
    sftfs_cache_invalidate_dir(&endp->dir_cache, path);
}

static inline const char *find_last_seperator(const char *path)
{
    const char *sep = path;
    assert(*sep == '/');
    while (*++path)
        if (*path == '/')
            sep = path;
    return sep;
}

void sftfs_cached_inval_all_dir(struct sftfs_cached_endp *endp, const char *path)
{
    SFTFS_TRACE_FUNC
    sftfs_cached_inval_all(endp, path);
    const char *last_sep = find_last_seperator(path);

    if (last_sep == path)
        return;

    char *base_dir = strndup(path, last_sep - path - 1);
    sftfs_cached_inval_all(endp, base_dir);
    free(base_dir);
}

const char *sftfs_cached_get_handled_path(struct sftfs_cached_endp *endp, uintptr_t handle)
{
    sftfs_htable_entry_link_ro entry_link = sftfs_htable_lookup_ro(endp->handle_path_map, handle);
    sftfs_htable_entry_ro entry = entry_link ? *entry_link : NULL;
    return entry ? sftfs_htable_entry_data_ro(entry) : NULL;
}

void sftfs_cached_handle_path(struct sftfs_cached_endp *endp, uintptr_t handle, const char *path)
{
    sftfs_htable_remove_hash(&endp->handle_path_map, handle);
    sftfs_htable_insert(&endp->handle_path_map, handle, path, strlen(path) + 1);
}

void sftfs_cached_unhandle_path(struct sftfs_cached_endp *endp, uintptr_t handle)
{
    sftfs_htable_remove_hash(&endp->handle_path_map, handle);
}

bool sftfs_cached_fetch_attr(struct sftfs_cached_endp *endp, const char *path, struct stat *attr)
{
    SFTFS_TRACE_FUNC
    bool hit = sftfs_cache_get_attr(&endp->attr_cache, path, attr);
    if (hit)
        sftfs_debug("Cache HIT: attribute path: %s\n", path);
    else
        sftfs_debug("Cache MISS: attribute path: %s\n", path);
    return hit;
}

bool sftfs_cached_store_attr(struct sftfs_cached_endp *endp, const char *path, const struct stat *attr)
{
    SFTFS_TRACE_FUNC
    int rc = sftfs_cache_put_attr(&endp->attr_cache, path, attr);
    sftfs_debug("Cache PUT: attribute path: %s\n", path);
    if (rc != SFTFS_CACHE_ATTR_OK)
        sftfs_error("Failed to cache attribute for path %s, sftfs_cache_put_attr(...) failed\n", path);
    return rc == 0;
}

void sftfs_cached_inval_attr(struct sftfs_cached_endp *endp, const char *path)
{
    SFTFS_TRACE_FUNC
    sftfs_debug("Cache INVALIDATE: attribute path: %s\n", path);
    sftfs_cache_invalidate_attr(&endp->attr_cache, path);
}

void sftfs_cached_rename_all(struct sftfs_cached_endp *endp, const char *oldpath, const char *newpath)
{
    SFTFS_TRACE_FUNC
    sftfs_cache_rename_file(&endp->attr_cache, oldpath, newpath);
    sftfs_cache_rename_file(&endp->dir_cache, oldpath, newpath);
}

bool sftfs_cached_dir_exists(struct sftfs_cached_endp *endp, const char *path)
{
    SFTFS_TRACE_FUNC
    return !!sftfs_cache_peek_dir(&endp->dir_cache, path).curr_entry;
}

static void fetch_dir_entries_without_attr(
        sftfs_cache_dir_handle dir, sftfs_endp_readdir_callee callee, void *user_data)
{
    SFTFS_TRACE_FUNC
    const char *entry = NULL;
    while ((entry = sftfs_cache_read_dir(&dir))) {
        struct sftfs_endp_direntry dirent = {
            .name = entry,
        };
        if (callee(&dirent, user_data))
            break;
    }
}

/* Assumes path is canonicalized. Replace the last path separator with a null char. */
static void mark_base_dir(char *path_end)
{
    char *c = path_end;
    while (*--c != '/') ;
    if (*(c + 1) != '\0')
        *c = '\0';
}

static void unmark_base_dir(char *path)
{
    path[strlen(path)] = '/';
}

static bool push_entry(sftfs_str *path_ref, const char *entry)
{
    sftfs_str path = *path_ref;
    if (strcmp(entry, "..") == 0)
        mark_base_dir(sftfs_str_c(path) + sftfs_str_size(path));
    else if (strcmp(entry, ".") != 0)
        (void) ((path = sftfs_str_append(path, '/')) &&
                (path = sftfs_str_extend_cstr(path, entry)));

    if (path)
        *path_ref = path;
    else
        sftfs_error("Failed to push an entry to a directory path\n");
    return !!path;
}

static void pop_entry(sftfs_str *path_ref, size_t original_path_size)
{
    sftfs_str path = *path_ref;

    if (original_path_size < sftfs_str_size(path))
        path = sftfs_str_resize(path, original_path_size);
    else if (strnlen(sftfs_str_c(path), original_path_size) < original_path_size)
        unmark_base_dir(sftfs_str_c(path));

    if (path)
        *path_ref = path;
}

static bool fetch_dir_entries_with_attr(
        sftfs_endp base_endp, const char *path, sftfs_cache_dir_handle dir,
        sftfs_endp_readdir_callee callee, void *user_data,
        int (*getattr)(sftfs_endp endp, const char *path, sftfs_endp_file file, struct stat *attr))
{
    SFTFS_TRACE_FUNC

    const size_t path_size = strlen(path);
    sftfs_str entry_path = sftfs_str_create(path, path_size);

    if (! entry_path) {
        sftfs_fatal("Failed to create a non-C string path\n");
        return false;
    }

    bool success = true;

    const char *entry = NULL;
    while ((entry = sftfs_cache_read_dir(&dir))) {
        sftfs_debug("current entry='%s'\n", entry);

        struct sftfs_endp_direntry dirent = {
            .name = entry,
        };

        if (! push_entry(&entry_path, entry)) {
            success = false;
            break;
        }

        sftfs_debug("current entry_path='%s'\n", sftfs_str_c(entry_path));

        const char *entry_path_c = sftfs_str_c(entry_path);
        if (!*entry_path_c)
            entry_path_c = "/";
        int rc = getattr(base_endp, entry_path_c, (sftfs_endp_file){0}, &dirent.stat);
        if (rc != 0) {
            sftfs_error("Getting attribute during entry lookup failed (%d)\n", rc);
            success = false;
            break;
        }

        int should_stop = callee(&dirent, user_data);

        pop_entry(&entry_path, path_size);

        if (should_stop)
            break;
    }

    sftfs_str_delete(entry_path);
    return success;
}

bool sftfs_cached_fetch_dir_entries(struct sftfs_cached_endp *endp, const char *path,
        int flags, sftfs_endp_readdir_callee callee, void *user_data,
        int (*getattr)(sftfs_endp endp, const char *path, sftfs_endp_file file, struct stat *attr))
{
    SFTFS_TRACE_FUNC
    sftfs_cache_dir_handle dir = sftfs_cache_peek_dir(&endp->dir_cache, path);
    if (! sftfs_cache_dir_valid(dir))
        return false;
    else if (flags & SFTFS_ENDP_READDIR_PLUS)
        return fetch_dir_entries_with_attr(endp->base_endp, path, dir, callee, user_data, getattr);
    else
        return fetch_dir_entries_without_attr(dir, callee, user_data), true;
}

struct cached_readdir_callee_context {
    sftfs_endp_readdir_callee cachee_callee;
    void *cachee_callee_data;
    int cachee_rc;

    struct sftfs_cached_endp *endp;
    sftfs_str path;
    sftfs_cache_dir *dir;

    int failed;
};

static int cached_readdir_callee(struct sftfs_endp_direntry *direntry, void *user_data)
{
    SFTFS_TRACE_FUNC

    struct cached_readdir_callee_context *data = user_data;
    if (! data->cachee_rc)
        data->cachee_rc = data->cachee_callee(direntry, data->cachee_callee_data);

    int rc = sftfs_cache_add_dir_entry(data->dir, direntry->name);
    if (rc != SFTFS_CACHE_DIR_OK) {
        sftfs_error("Failed to cache directory entry\n");
        data->failed = 1;
        return -1;
    }

    const size_t path_size = sftfs_str_size(data->path);

    if (! push_entry(&data->path, direntry->name)) {
        data->failed = 1;
        return -1;
    }

    const char *entry_path = sftfs_str_c(data->path);
    if (!*entry_path)
        entry_path = "/";

    sftfs_debug("entry_path=%s\n", entry_path);
    bool success = sftfs_cached_store_attr(data->endp, entry_path, &direntry->stat);

    pop_entry(&data->path, path_size);

    if (! success) {
        data->failed = 1;
        return -1;
    }

    return 0;
}

static struct cached_readdir_callee_context callee_ctx = {0};

bool sftfs_cached_setup_dir_caching(struct sftfs_cached_endp *endp,
        const char *path, sftfs_endp_readdir_callee *callee, void **user_data)
{
    SFTFS_TRACE_FUNC
    assert(callee_ctx.cachee_callee == NULL);
    assert(callee_ctx.cachee_callee_data == NULL);
    assert(callee_ctx.cachee_rc == 0);
    assert(callee_ctx.endp == NULL);
    assert(callee_ctx.path == NULL);
    assert(callee_ctx.dir == NULL);

    sftfs_str path_str = sftfs_str_create(path, strlen(path));
    if (! path_str) {
        sftfs_fatal("Failed to create a non-C string path\n");
        return false;
    }

    sftfs_cache_dir *dir = sftfs_cache_take_dir(&endp->dir_cache, path);
    if (! dir) {
        sftfs_str_delete(path_str);
        sftfs_fatal("Failed to take a cache directory\n");
        return false;
    }

    callee_ctx.cachee_callee = *callee;
    callee_ctx.cachee_callee_data = *user_data;
    callee_ctx.cachee_rc = 0;

    callee_ctx.endp = endp;
    callee_ctx.path = path_str;
    callee_ctx.dir = dir;

    callee_ctx.failed = 0;

    *callee = cached_readdir_callee;
    *user_data = &callee_ctx;
    return true;
}

void sftfs_cached_teardown_dir_caching(struct sftfs_cached_endp *endp, const char *path)
{
    SFTFS_TRACE_FUNC
    assert(callee_ctx.cachee_callee);
    assert(callee_ctx.endp);

    if (callee_ctx.failed)
        sftfs_cache_drop_dir(&endp->dir_cache, path, callee_ctx.dir);
    else
        sftfs_cache_give_dir(&endp->dir_cache, path, callee_ctx.dir);
    sftfs_str_delete(callee_ctx.path);
    memset(&callee_ctx, 0, sizeof(callee_ctx));
}
