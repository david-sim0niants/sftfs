#include "abs/str.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static inline void null_term(sftfs_str str)
{
    str->c[str->size] = '\0';
}

sftfs_str sftfs_str_create(const char *from, size_t size)
{
    sftfs_str str = malloc(sizeof(struct sftfs_str_struct) + size + 1);
    if (NULL == str)
        return NULL;

    str->capacity = str->size = size;
    if (from)
        memcpy(str->c, from, size);

    null_term(str);
    return str;
}

void sftfs_str_delete(sftfs_str str)
{
    free(str);
}

sftfs_str sftfs_str_reserve(sftfs_str str, size_t capacity)
{
    if (capacity <= str->capacity)
        return str;

    str = realloc(str, sizeof(struct sftfs_str_struct) + capacity + 1);

    if (NULL == str)
        return NULL;

    str->capacity = capacity;
    return str;
}

sftfs_str sftfs_str_resize(sftfs_str str, size_t size)
{
    str = sftfs_str_reserve(str, size);
    if (NULL == str)
        return NULL;
    str->size = size;
    null_term(str);
    return str;
}

sftfs_str sftfs_str_shrink(sftfs_str str, size_t shrink_size)
{
    if (shrink_size > str->size)
        return NULL;
    else
        return sftfs_str_resize(str, str->size - shrink_size);
}

sftfs_str sftfs_str_extend_cstr(sftfs_str str, const char *cstr)
{
    return sftfs_str_extend(str, cstr, strlen(cstr));
}

sftfs_str sftfs_str_extend(sftfs_str str, const char *from, size_t size)
{
    size_t prev_size = str->size;
    str = sftfs_str_resize(str, prev_size + size);
    if (NULL == str)
        return NULL;

    memcpy(&str->c[prev_size], from, size);
    null_term(str);
    return str;
}

sftfs_str sftfs_str_append(sftfs_str str, char c)
{
    if (str->size >= str->capacity) {
        size_t new_capacity = str->capacity == 0 ? 1 : str->capacity * 2;
        str = sftfs_str_reserve(str, new_capacity);

        if (NULL == str)
            return NULL;
    }

    str->c[str->size] = c;
    ++str->size;
    null_term(str);

    return str;
}

sftfs_str sftfs_str_assign_cstr(sftfs_str str, const char *path)
{
    return sftfs_str_assign(str, path, strlen(path));
}

sftfs_str sftfs_str_assign(sftfs_str str, const char *path, size_t size)
{
    str = sftfs_str_resize(str, size);
    if (str) {
        memcpy(&str->c, path, size);
        null_term(str);
    }
    return str;
}
