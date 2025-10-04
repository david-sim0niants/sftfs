#include "endp.h"
#include "endp/sftp.h"

#include "func_trace.h"
#include "str.h"

#include <assert.h>
#include <errno.h>
#include <libssh/sftp.h>
#include <stdlib.h>
#include <string.h>

struct sftfs_endp_handle {
    sftp_session sftp;
    char *work_dir;
    size_t work_dir_len;
    sftfs_str curr_abs_path;
};

static inline struct sftfs_endp_handle *get_handle(sftfs_endp endp)
{
    return (struct sftfs_endp_handle *)endp;
}

static inline sftp_session get_sftp(sftfs_endp endp)
{
    return get_handle(endp)->sftp;
}

static sftp_session sftfs_endp_init_sftp_session(ssh_session ssh)
{
    SFTFS_TRACE_FUNC
    sftp_session sftp = sftp_new(ssh);

    if (NULL == sftp) {
        sftfs_fatal("Failed allocating SFTP session: %s\n", ssh_get_error(ssh));
        return NULL;
    }

    if (sftp_init(sftp) != SSH_OK) {
        sftfs_fatal("Failed initializing SFTP session: [ %d ]\n", sftp_get_error(sftp));
        sftp_free(sftp);
        return NULL;
    }

    return sftp;
}

sftfs_endp sftfs_endp_init(ssh_session ssh, struct sftfs_endp_sftp_config *config)
{
    SFTFS_TRACE_FUNC
    if (! config->work_dir)
        return NULL;

    struct sftfs_endp_handle *handle = calloc(1, sizeof(struct sftfs_endp_handle));
    if (! handle)
        goto handle_alloc_failed;

    handle->sftp = sftfs_endp_init_sftp_session(ssh);
    if (! handle->sftp)
        goto sftp_init_failed;

    handle->work_dir = sftp_canonicalize_path(handle->sftp, config->work_dir);
    if (! handle->work_dir) {
        sftfs_fatal("Failed to canonicalize working directory: %d\n", sftp_get_error(handle->sftp));
        goto canonocalize_work_dir_failed;
    }
    handle->work_dir_len = strlen(handle->work_dir);

    sftfs_debug("Working directory: %s\n", handle->work_dir);

    handle->curr_abs_path = sftfs_str_create(handle->work_dir, handle->work_dir_len);
    if (! handle->curr_abs_path) {
        sftfs_fatal("Failed to create current absolute path\n");
        goto current_path_alloc_failed;
    }

    return handle;

current_path_alloc_failed:
    ssh_string_free_char(handle->work_dir);
canonocalize_work_dir_failed:
    sftp_free(handle->sftp);
sftp_init_failed:
    free(handle);
handle_alloc_failed:
    return NULL;
}

void sftfs_endp_deinit(sftfs_endp endp_sftp)
{
    SFTFS_TRACE_FUNC
    sftfs_str_delete(get_handle(endp_sftp)->curr_abs_path);
    ssh_string_free_char(get_handle(endp_sftp)->work_dir);
    sftp_free(get_sftp(endp_sftp));
    free((struct sftfs_endp_handle *)endp_sftp);
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
            return ENODATA;
        default:
            return 255;
    }
}

static inline int ret_sftp_err(sftp_session sftp)
{
    return -to_errno(sftp_get_error(sftp));
}

static const char *get_abs_path(sftfs_endp endp, const char *rel_path)
{
    SFTFS_TRACE_FUNC
    struct sftfs_endp_handle *handle = get_handle(endp);

    sftfs_str abs_path = sftfs_str_resize(handle->curr_abs_path, handle->work_dir_len);
    if (! abs_path) {
        sftfs_fatal("Resizing absolute path string failed\n");
        return NULL;
    }
    handle->curr_abs_path = abs_path;

    abs_path = sftfs_str_extend_cstr(handle->curr_abs_path, rel_path);
    if (! abs_path) {
        sftfs_fatal("Extending absolute path with relative path failed\n");
        return NULL;
    }
    handle->curr_abs_path = abs_path;

    sftfs_debug("current absolute path: %s\n", sftfs_str_c_ro(handle->curr_abs_path));

    return sftfs_str_c_ro(handle->curr_abs_path);
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

    if (NULL == (path = get_abs_path(endp, path)))
        return -ENOMEM;

    sftp_attributes attr = file.handle ? sftp_fstat(from_endp_file(file)) : sftp_lstat(sftp, path);

    if (NULL == attr)
        if (NULL == (attr = sftp_stat(sftp, path)))
            return ret_sftp_err(sftp);

    convert_sftp_attr_to_stat(attr, stat);
    sftp_attributes_free(attr);

    return 0;
}

int sftfs_endp_readlink(sftfs_endp endp, const char *path, char *buf, size_t bufsiz)
{
    SFTFS_TRACE_FUNC
    assert(bufsiz > 0);
    sftp_session sftp = get_sftp(endp);

    if (NULL == (path = get_abs_path(endp, path)))
        return -ENOMEM;

    char *target = sftp_readlink(sftp, path);
    if (NULL == target)
        return ret_sftp_err(sftp);

    size_t copy_size = strnlen(target, bufsiz - 1);
    strncpy(buf, target, copy_size);
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

    if (NULL == (path = get_abs_path(endp, path)))
        return -ENOMEM;

    sftp_dir sftp_dir = sftp_opendir(sftp, path);
    if (NULL == sftp_dir)
        return ret_sftp_err(sftp);

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
        return ret_sftp_err(sftp);
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

    if (NULL == (path = get_abs_path(endp, path)))
        return -ENOMEM;

    sftp_file sftp_file = sftp_open(sftp, path, access_flags, 0);
    if (NULL == sftp_file)
        return ret_sftp_err(sftp);

    *file = to_endp_file(sftp_file);
    return 0;
}

int sftfs_endp_close(sftfs_endp endp, sftfs_endp_file file)
{
    SFTFS_TRACE_FUNC
    (void)endp;
    return sftp_close(from_endp_file(file)) == 0 ? 0 : -EIO;
}

int sftfs_endp_access(sftfs_endp endp, const char *path, int mode)
{
    SFTFS_TRACE_FUNC
    sftp_session sftp = get_sftp(endp);

    if (NULL == (path = get_abs_path(endp, path)))
        return -ENOMEM;

    sftp_attributes attr = sftp_stat(sftp, path);
    if (NULL == attr)
        return ret_sftp_err(sftp);

    sftfs_debug("attr->permissions=%o\n", attr->permissions);
    int rc = ((attr->permissions & (uint32_t)mode) == (uint32_t)mode) ? 0 : -EACCES;

    sftp_attributes_free(attr);
    return rc;
}
