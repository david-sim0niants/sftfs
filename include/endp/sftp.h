#pragma once

#include "endp/defs.h"

int sftfs_sftp_getattr(sftfs_endp endp, const char *path, sftfs_endp_file file, struct stat *stat);

int sftfs_sftp_readlink(sftfs_endp endp, const char *path, char *buf, size_t bufsiz);

int sftfs_sftp_mkdir(sftfs_endp endp, const char *path, mode_t mode);

int sftfs_sftp_unlink(sftfs_endp endp, const char *path);

int sftfs_sftp_rmdir(sftfs_endp endp, const char *path);

int sftfs_sftp_symlink(sftfs_endp endp, const char *target, const char *linkpath);

int sftfs_sftp_rename(sftfs_endp endp, const char *oldpath, const char *newpath, unsigned int flags);

int sftfs_sftp_chmod(sftfs_endp endp, const char *path, mode_t mode);

int sftfs_sftp_chown(sftfs_endp endp, const char *path, uid_t uid, gid_t gid);

int sftfs_sftp_opendir(sftfs_endp endp, const char *path, sftfs_endp_dir *dir);

int sftfs_sftp_readdir(sftfs_endp endp, const char *path, sftfs_endp_dir dir, int flags,
        sftfs_endp_readdir_callee callee, void *user_data);

int sftfs_sftp_closedir(sftfs_endp endp, sftfs_endp_dir dir);

int sftfs_sftp_open(sftfs_endp endp, sftfs_endp_file *file, const char *path, int access_flags);

int sftfs_sftp_read(sftfs_endp endp, sftfs_endp_file file, char *buf, size_t size, off_t off);

int sftfs_sftp_write(sftfs_endp endp, sftfs_endp_file file, const char *buf, size_t size, off_t off);

int sftfs_sftp_statfs(sftfs_endp endp, const char *path, struct statvfs *statv);

int sftfs_sftp_close(sftfs_endp endp, sftfs_endp_file file);

int sftfs_sftp_access(sftfs_endp endp, const char *path, int mode);

int sftfs_sftp_create(sftfs_endp endp, const char *path, mode_t mode, sftfs_endp_file *file);

int sftfs_sftp_utimens(sftfs_endp endp, const char *path, const struct timespec tv[2]);

#undef  SFTFS_ENDP_getattr
#define SFTFS_ENDP_getattr  sftfs_sftp_getattr

#undef  SFTFS_ENDP_readlink
#define SFTFS_ENDP_readlink sftfs_sftp_readlink

#undef  SFTFS_ENDP_mkdir
#define SFTFS_ENDP_mkdir    sftfs_sftp_mkdir

#undef  SFTFS_ENDP_unlink
#define SFTFS_ENDP_unlink   sftfs_sftp_unlink

#undef  SFTFS_ENDP_rmdir
#define SFTFS_ENDP_rmdir    sftfs_sftp_rmdir

#undef  SFTFS_ENDP_symlink
#define SFTFS_ENDP_symlink  sftfs_sftp_symlink

#undef  SFTFS_ENDP_rename
#define SFTFS_ENDP_rename   sftfs_sftp_rename

#undef  SFTFS_ENDP_chmod
#define SFTFS_ENDP_chmod    sftfs_sftp_chmod

#undef  SFTFS_ENDP_chown
#define SFTFS_ENDP_chown    sftfs_sftp_chown

#undef  SFTFS_ENDP_opendir
#define SFTFS_ENDP_opendir  sftfs_sftp_opendir

#undef  SFTFS_ENDP_readdir
#define SFTFS_ENDP_readdir  sftfs_sftp_readdir

#undef  SFTFS_ENDP_closedir
#define SFTFS_ENDP_closedir sftfs_sftp_closedir

#undef  SFTFS_ENDP_open
#define SFTFS_ENDP_open     sftfs_sftp_open

#undef  SFTFS_ENDP_read
#define SFTFS_ENDP_read     sftfs_sftp_read

#undef  SFTFS_ENDP_write
#define SFTFS_ENDP_write    sftfs_sftp_write

#undef  SFTFS_ENDP_statfs
#define SFTFS_ENDP_statfs   sftfs_sftp_statfs

#undef  SFTFS_ENDP_close
#define SFTFS_ENDP_close    sftfs_sftp_close

#undef  SFTFS_ENDP_access
#define SFTFS_ENDP_access   sftfs_sftp_access

#undef  SFTFS_ENDP_create
#define SFTFS_ENDP_create   sftfs_sftp_create

#undef  SFTFS_ENDP_utimens
#define SFTFS_ENDP_utimens  sftfs_sftp_utimens
