#include "endp.h"

#include "func_trace.h"

#include <assert.h>
#include <errno.h>
#include <libssh/sftp.h>
#include <string.h>

static inline sftp_session get_sftp(sftfs_endp endp)
{
    SFTFS_TRACE_FUNC
    return (sftp_session)endp;
}

static int to_errno(int err)
{
    SFTFS_TRACE_FUNC
    switch (err) {
        case SSH_FX_OK:
            return 0;
        case SSH_FX_NO_SUCH_FILE:
            return ENOENT;
        case SSH_FX_PERMISSION_DENIED:
            return EACCES;
        case SSH_FX_FAILURE:
            return EIO;
        case SSH_FX_FILE_ALREADY_EXISTS:
            return EEXIST;
        case SSH_FX_INVALID_HANDLE:
            return EBADF;
        case SSH_FX_NO_CONNECTION:
            return ENOTCONN;
        case SSH_FX_NO_MEDIA:
            return ENOMEDIUM;
        case SSH_FX_NO_SUCH_PATH:
            return ENOENT;
        case SSH_FX_OP_UNSUPPORTED:
            return ENOTSUP;
        case SSH_FX_BAD_MESSAGE:
            return EBADMSG;
        case SSH_FX_WRITE_PROTECT:
            return EROFS;
        case SSH_FX_EOF:
        default:
            return -1;
    }
}

static void convert_sftp_attr_to_stat(sftp_attributes attr, struct stat *stat)
{
    SFTFS_TRACE_FUNC
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
}

static inline sftfs_endp_file to_endp_file(sftp_file file)
{
    sftfs_endp_file endp_file = { .handle = (uintptr_t)file, };
    return endp_file;
}
static inline sftp_file from_endp_file(sftfs_endp_file file)
{
    return (sftp_file)file.handle;
}

int sftfs_endp_getattr(sftfs_endp endp, const char *path, sftfs_endp_file file, struct stat *stat)
{
    SFTFS_TRACE_FUNC
    sftp_session sftp = get_sftp(endp);

    sftp_attributes attr = file.handle ? sftp_fstat(from_endp_file(file)) : sftp_lstat(sftp, path);

    if (NULL == attr)
        if (NULL == (attr = sftp_stat(sftp, path)))
            return -to_errno(sftp_get_error(sftp));

    convert_sftp_attr_to_stat(attr, stat);
    sftp_attributes_free(attr);

    return 0;
}

int sftfs_endp_readlink(sftfs_endp endp, const char *path, char *buf, size_t bufsiz)
{
    SFTFS_TRACE_FUNC
    assert(bufsiz > 0);

    sftp_session sftp = get_sftp(endp);

    char *target = sftp_readlink(sftp, path);
    if (NULL == target)
        return -sftp_get_error(sftp);

    size_t copy_size = strnlen(target, bufsiz - 1);
    strncpy(buf, path, copy_size);
    buf[copy_size] = '\0';

    ssh_string_free_char(target);

    return 0;
}

static inline sftfs_endp_dir to_endp_dir(sftp_dir dir)
{
    SFTFS_TRACE_FUNC
    sftfs_endp_dir endp_dir = { .handle = (uintptr_t)dir };
    return endp_dir;
}

static inline sftp_dir from_endp_dir(sftfs_endp_dir dir)
{
    SFTFS_TRACE_FUNC
    return (sftp_dir)dir.handle;
}

int sftfs_endp_opendir(sftfs_endp endp, const char *path, sftfs_endp_dir *dir)
{
    SFTFS_TRACE_FUNC
    sftp_session sftp = get_sftp(endp);

    sftp_dir sftp_dir = sftp_opendir(sftp, path);
    if (NULL == sftp_dir)
        return -to_errno(sftp_get_error(sftp));

    *dir = to_endp_dir(sftp_dir);
    return 0;
}

int sftfs_endp_readdir(sftfs_endp endp, const sftfs_endp_dir dir, int flags,
        sftfs_endp_readdir_callee callee, void *user_data)
{
    SFTFS_TRACE_FUNC
    sftp_session sftp = get_sftp(endp);

    sftp_attributes attr = NULL;

    while (NULL != (attr = sftp_readdir(sftp, from_endp_dir(dir)))) {
        struct sftfs_endp_direntry dentry = {
            .name = attr->name,
        };

        if (flags & SFTFS_ENDP_READDIR_PLUS)
            convert_sftp_attr_to_stat(attr, &dentry.stat);

        int should_stop = callee(&dentry, user_data);
        sftp_attributes_free(attr);

        if (should_stop)
            return 0;
    }

    if (sftp_dir_eof(from_endp_dir(dir)))
        return 0;
    else
        return -to_errno(sftp_get_error(sftp));
}

int sftfs_endp_closedir(sftfs_endp endp, sftfs_endp_dir dir)
{
    SFTFS_TRACE_FUNC
    (void)endp;
    return sftp_closedir(from_endp_dir(dir)) == 0 ? 0 : -EIO;
}

int sftfs_endp_open(sftfs_endp endp, sftfs_endp_file *file, const char *path, int access_flags)
{
    SFTFS_TRACE_FUNC
    sftp_session sftp = get_sftp(endp);

    sftp_file sftp_file = sftp_open(sftp, path, access_flags, 0);
    if (NULL == sftp_file)
        return -to_errno(sftp_get_error(sftp));

    *file = to_endp_file(sftp_file);
    return 0;
}

int sftfs_endp_close(sftfs_endp endp, sftfs_endp_file file)
{
    SFTFS_TRACE_FUNC
    (void)endp;
    return sftp_close(from_endp_file(file)) == 0 ? 0 : -EIO;
}
