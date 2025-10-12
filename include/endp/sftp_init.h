#pragma once

#include "endp/defs.h"
#include "str.h"

#include <libssh/libssh.h>
#include <libssh/sftp.h>

struct sftfs_sftp {
    sftp_session sftp;
    char *work_dir;
    size_t base_dir_len;
    sftfs_str curr_abs_path;
};

struct sftfs_sftp_params {
    ssh_session ssh;
    const char *work_dir;
};

sftfs_endp sftfs_sftp_construct(struct sftfs_sftp *handle, struct sftfs_sftp_params *params);
void sftfs_sftp_destruct(sftfs_endp endp);

sftfs_endp sftfs_sftp_init(struct sftfs_sftp_params *params);
void sftfs_sftp_deinit(sftfs_endp endp);
