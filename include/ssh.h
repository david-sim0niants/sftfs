#pragma once

#include <libssh/libssh.h>

struct sftfs_ssh_config {
    const char *host;
    const char *user;
    unsigned short port;
};

int sftfs_ssh_connect(struct sftfs_ssh_config *config, ssh_session *session);
int sftfs_ssh_auth(ssh_session ssh, const char *password);
void sftfs_ssh_disconnect(ssh_session ssh);
