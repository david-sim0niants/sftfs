#pragma once

#include <sys/stat.h>

int sftfs_getattr(const char *path, struct stat *stat);
