#include "core.h"
#include "endp.h"
#include "func_trace.h"

#include <errno.h>
#include <libssh/sftp.h>

static inline sftfs_endp get_endp()
{
    return fuse_get_context()->private_data;
}

static inline sftfs_endp_dir wrap_dir(uint64_t handle)
{
    sftfs_endp_dir dir = {.handle = handle};
    return dir;
}

static inline sftfs_endp_file wrap_file(uint64_t handle)
{
    sftfs_endp_file file = {.handle = handle};
    return file;
}

int sftfs_getattr(const char *path, struct stat *stat, struct fuse_file_info *fi)
{
    SFTFS_TRACE_FUNC
    sftfs_debug("path=%s, fi=%p\n", path, fi);

    return sftfs_endp_getattr(get_endp(), path, wrap_file(fi ? fi->fh : 0), stat);
}

int sftfs_readlink(const char *path, char *buf, size_t bufsiz)
{
    SFTFS_TRACE_FUNC
    sftfs_debug("path=%s, buf=%p, bufsiz=%zu\n", path, buf, bufsiz);

    assert(bufsiz > 0);
    return sftfs_endp_readlink(get_endp(), path, buf, bufsiz);
}

int sftfs_opendir(const char *path, struct fuse_file_info *fi)
{
    SFTFS_TRACE_FUNC
    sftfs_debug("path=%s, fi=%p\n", path, fi);

    sftfs_endp_dir dir;
    int rc = sftfs_endp_opendir(get_endp(), path, &dir);
    fi->fh = dir.handle;
    return rc;
}

struct readdir_callee_data {
    void *buf;
    fuse_fill_dir_t fill_dir;
    int rc;
};

static int readdir_callee(struct sftfs_endp_direntry *direntry, void *user_data)
{
    SFTFS_TRACE_FUNC
    sftfs_debug("entry='%s'\n", direntry->name);

    struct readdir_callee_data *data = user_data;
    data->rc = data->fill_dir(data->buf, direntry->name, &direntry->stat, 0, 0);
    if (data->rc != 0)
        data->rc = -ENOMEM;
    return data->rc;
}

int sftfs_readdir(const char *path, void *buf, fuse_fill_dir_t fill_dir, off_t off,
        struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
    SFTFS_TRACE_FUNC
    sftfs_debug("path=%s, buf=%p, fi=%p\n", path, buf, fi);
    (void)off;

    struct readdir_callee_data data = {
        .buf = buf,
        .fill_dir = fill_dir,
    };

    int endp_flags = 0;
    endp_flags |= (flags & (SFTFS_ENDP_READDIR_DEFAULT)) ? SFTFS_ENDP_READDIR_DEFAULT : 0;
    endp_flags |= (flags & (SFTFS_ENDP_READDIR_PLUS)) ? SFTFS_ENDP_READDIR_PLUS : 0;

    int rc = sftfs_endp_readdir(get_endp(), wrap_dir(fi->fh), endp_flags, readdir_callee, &data);

    if (data.rc != 0)
        return data.rc;
    else
        return rc;
}

int sftfs_releasedir(const char *path, struct fuse_file_info *fi)
{
    SFTFS_TRACE_FUNC
    sftfs_debug("path=%s, fi=%p\n", path, fi);

    return sftfs_endp_closedir(get_endp(), wrap_dir(fi->fh));
}

int sftfs_open(const char *path, struct fuse_file_info *fi)
{
    SFTFS_TRACE_FUNC
    sftfs_debug("path=%s, fi=%p\n", path, fi);

    sftfs_endp_file file;
    int rc = sftfs_endp_open(get_endp(), &file, path, fi->flags);
    fi->fh = file.handle;
    return rc;
}

int sftfs_read(const char *path, char *buf, size_t size, off_t off, struct fuse_file_info *fi)
{
    (void)path;
    return sftfs_endp_read(get_endp(), wrap_file(fi->fh), buf, size, off);
}

int sftfs_release(const char *path, struct fuse_file_info *fi)
{
    SFTFS_TRACE_FUNC
    sftfs_debug("path=%s, fi=%p\n", path, fi);
    return sftfs_endp_close(get_endp(), wrap_file(fi->fh));
}

int sftfs_access(const char *path, int mode)
{
    SFTFS_TRACE_FUNC
    sftfs_debug("path=%s, mode=%o\n", path, mode);
    return sftfs_endp_access(get_endp(), path, mode);
}
