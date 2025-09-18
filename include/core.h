#pragma once

#include <libssh/sftp.h>
#include <sys/stat.h>

typedef struct sftfs_handle {
    sftp_session session;
} sftfs_handle_t;

int sftfs_getattr(const char *path, struct stat *stat);
