#include "endp.h"

#include <errno.h>
#include <libssh/sftp.h>

static inline sftp_session get_sftp_session(sftfs_endp endp)
{
    return (sftp_session)endp;
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
    sftp_session sftp = get_sftp_session(endp);
    sftp_attributes attr = sftp_stat(sftp, path);

    if (NULL == attr)
        return errno;

    convert_sftp_attr_to_stat(attr, stat);
    return 0;
}
