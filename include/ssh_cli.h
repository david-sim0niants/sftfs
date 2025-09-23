#pragma once

#include <libssh/libssh.h>

ssh_session sftfs_ssh_cli(const char *user, const char *host, int port);
