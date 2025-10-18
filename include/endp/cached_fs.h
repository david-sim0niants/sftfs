#pragma once

#include <stdbool.h>

#include <sys/stat.h>

#include "abs/htable.h"

typedef struct sftfs_cached_fs_s {
    sftfs_htable file_table;
} *sftfs_cached_fs;

typedef const struct sftfs_cached_fs_s *sftfs_cached_fs_ro;

enum {
    SFTFS_CACHED_OK = 0,
    SFTFS_CACHED_FAIL
};

bool sftfs_cached_fetch_getattr(sftfs_cached_fs fs, const char *path, struct stat *attr);
int  sftfs_cached_cache_getattr(sftfs_cached_fs fs, const char *path, const struct stat *attr);
