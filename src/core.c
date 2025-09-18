#include "core.h"

#include <errno.h>
#define FUSE_USE_VERSION 31
#include <fuse.h>

static sftfs_handle_t sftfs_get_handle(struct fuse_context *ctx)
{
    return *(sftfs_handle_t *)(ctx->private_data);
}

int sftfs_getattr(const char *path, struct stat *stat)
{
    struct fuse_context *ctx = fuse_get_context();
    sftfs_handle_t handle = sftfs_get_handle(ctx);
    sftp_attributes attr = sftp_stat(handle.session, path);

    if (NULL == attr)
        return errno;

    stat->st_mode = attr->permissions;
    stat->st_size = attr->size;
    stat->st_uid = attr->uid;
    stat->st_gid = attr->gid;
    stat->st_nlink = 1;

    stat->st_atim.tv_sec = (time_t)attr->atime;
    stat->st_atim.tv_nsec = (time_t)attr->atime_nseconds;
    stat->st_mtim.tv_sec = (time_t)attr->mtime;
    stat->st_mtim.tv_nsec = (time_t)attr->mtime_nseconds;
    stat->st_ctim.tv_sec = stat->st_mtim.tv_sec;
    stat->st_ctim.tv_nsec = stat->st_mtim.tv_nsec;

    return 0;
}
