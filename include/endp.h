#pragma once

#include <stddef.h>

#include <sys/stat.h>

typedef void *sftfs_endp;

int sftfs_endp_getattr(sftfs_endp endp, const char *path, struct stat *stat);
int sftfs_endp_readlink(sftfs_endp endp, const char *path, char *buf, size_t bufsiz);
