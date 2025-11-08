#pragma once

#include <stddef.h>

struct sftfs_str_struct {
    size_t size;
    size_t capacity;
    char c[0];
};

typedef struct sftfs_str_struct *sftfs_str;
typedef const struct sftfs_str_struct *sftfs_str_ro;

sftfs_str sftfs_str_create(const char *from, size_t size);
void sftfs_str_delete(sftfs_str str);

sftfs_str sftfs_str_reserve(sftfs_str str, size_t capacity);
sftfs_str sftfs_str_resize(sftfs_str str, size_t size);
sftfs_str sftfs_str_shrink(sftfs_str str, size_t shrink_size);

sftfs_str sftfs_str_extend_cstr(sftfs_str str, const char *cstr);
sftfs_str sftfs_str_extend(sftfs_str str, const char *from, size_t size);
sftfs_str sftfs_str_append(sftfs_str str, char c);

static inline size_t sftfs_str_size(sftfs_str_ro str)
{
    return str->size;
}

static inline size_t sftfs_str_capacity(sftfs_str_ro str)
{
    return str->capacity;
}

static inline const char *sftfs_str_c_ro(sftfs_str_ro str)
{
    return str->c;
}

static inline char *sftfs_str_c(sftfs_str str)
{
    return str->c;
}
