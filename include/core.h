#pragma once

#include <sys/stat.h>

#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>

int sftfs_getattr(const char *path, struct stat *stat, struct fuse_file_info *fi);
