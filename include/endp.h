#pragma once

#include <sys/stat.h>

typedef void *sftfs_endp;

int sftfs_endp_getattr(sftfs_endp endp, const char *path, struct stat *stat);
