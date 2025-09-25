#include "endp.h"

#include <assert.h>
#include <errno.h>
#include <libssh/sftp.h>
#include <string.h>

static inline sftp_session get_sftp(sftfs_endp endp)
{
    return (sftp_session)endp;
}

static int sftp_error_to_errno(int err)
{
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

int sftfs_endp_getattr(sftfs_endp endp, const char *path, struct stat *stat)
{
    sftp_session sftp = get_sftp(endp);
    sftp_attributes attr = sftp_stat(sftp, path);

    if (NULL == attr)
        return -sftp_error_to_errno(sftp_get_error(sftp));

    convert_sftp_attr_to_stat(attr, stat);
    return 0;
}

int sftfs_endp_readlink(sftfs_endp endp, const char *path, char *buf, size_t bufsiz)
{
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
