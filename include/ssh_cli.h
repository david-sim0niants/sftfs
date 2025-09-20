#pragma once

#include <libssh/libssh.h>

int sftfs_ssh_cli(int *argc_p, char ***argv_p, ssh_session *ssh);
