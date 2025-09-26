#include "core.h"
#include "endp.h"

#include <errno.h>

static inline sftfs_endp get_endp()
{
    return fuse_get_context()->private_data;
}

int sftfs_getattr(const char *path, struct stat *stat, struct fuse_file_info *fi)
{
    (void)fi;
    return sftfs_endp_getattr(get_endp(), path, stat);
}

int sftfs_readlink(const char *path, char *buf, size_t bufsiz)
{
    assert(bufsiz > 0);
    return sftfs_endp_readlink(get_endp(), path, buf, bufsiz);
}

int sftfs_opendir(const char *path, struct fuse_file_info *fi)
{
    sftfs_endp_dir dir;
    int e = sftfs_endp_opendir(get_endp(), path, &dir);
    fi->fh = dir;
    return e;
}

struct readdir_callee_data {
    void *buf;
    fuse_fill_dir_t fill_dir;
    int rc;
};

static int readdir_callee(struct sftfs_endp_direntry *direntry, void *user_data)
{
    struct readdir_callee_data *data = user_data;
    data->rc = data->fill_dir(data->buf, direntry->name, &direntry->stat, 0, 0);
    if (data->rc != 0)
        data->rc = -ENOMEM;
    return data->rc;
}

int sftfs_readdir(const char *path, void *buf, fuse_fill_dir_t fill_dir, off_t off,
        struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
    (void)path; (void)off;

    struct readdir_callee_data data = {
        .buf = buf,
        .fill_dir = fill_dir,
    };

    int endp_flags = 0;
    endp_flags |= (flags & (SFTFS_ENDP_READDIR_DEFAULT)) ? SFTFS_ENDP_READDIR_DEFAULT : 0;
    endp_flags |= (flags & (SFTFS_ENDP_READDIR_PLUS)) ? SFTFS_ENDP_READDIR_PLUS : 0;

    int rc = sftfs_endp_readdir(get_endp(), (sftfs_endp_dir)fi->fh, endp_flags, readdir_callee, &data);

    if (data.rc != 0)
        return data.rc;
    else
        return rc;
}

int sftfs_releasedir(const char *path, struct fuse_file_info *fi)
{
    (void)path;
    return sftfs_endp_closedir(get_endp(), (sftfs_endp_dir)fi->fh);
}
