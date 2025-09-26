#pragma once

#include <sys/stat.h>

#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>

int sftfs_getattr(const char *path, struct stat *stat, struct fuse_file_info *fi);
int sftfs_readlink(const char *, char *, size_t);
int sftfs_opendir(const char *path, struct fuse_file_info *fi);
int sftfs_readdir(const char *path, void *buf, fuse_fill_dir_t fill_dir, off_t off,
        struct fuse_file_info *fi, enum fuse_readdir_flags flags);
int sftfs_releasedir(const char *path, struct fuse_file_info *fi);
