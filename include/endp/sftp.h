#pragma once

#include "endp.h"

#include <libssh/libssh.h>

struct sftfs_endp_sftp_config {
    const char *work_dir;
};

sftfs_endp sftfs_endp_init(ssh_session ssh, struct sftfs_endp_sftp_config *config);
void sftfs_endp_deinit(sftfs_endp endp_sftp);
