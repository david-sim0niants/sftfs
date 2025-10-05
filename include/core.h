#pragma once

#include <sys/stat.h>

#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>

int sftfs_getattr(const char *path, struct stat *stat, struct fuse_file_info *fi);
int sftfs_readlink(const char *, char *, size_t);
int sftfs_mkdir(const char *path, mode_t mode);
int sftfs_unlink(const char *path);
int sftfs_rmdir(const char *path);
int sftfs_symlink(const char *target, const char *linkpath);
int sftfs_rename(const char *oldpath, const char *newpath, unsigned int flags);
int sftfs_chmod(const char *path, mode_t mode, struct fuse_file_info *fi);
int sftfs_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi);
int sftfs_opendir(const char *path, struct fuse_file_info *fi);
int sftfs_readdir(const char *path, void *buf, fuse_fill_dir_t fill_dir, off_t off,
        struct fuse_file_info *fi, enum fuse_readdir_flags flags);
int sftfs_releasedir(const char *path, struct fuse_file_info *fi);
int sftfs_open(const char *path, struct fuse_file_info *fi);
int sftfs_read(const char *path, char *buf, size_t size, off_t off, struct fuse_file_info *fi);
int sftfs_statfs(const char *path, struct statvfs *statv);
int sftfs_release(const char *path, struct fuse_file_info *fi);
int sftfs_access(const char *path, int mode);
