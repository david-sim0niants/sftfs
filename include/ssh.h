#pragma once

#include <libssh/libssh.h>

struct sftfs_ssh_config {
    const char *host;
    const char *user;
    int port;
};

int sftfs_ssh_connect(struct sftfs_ssh_config *config, ssh_session *session);
void sftfs_ssh_disconnect(ssh_session ssh);
