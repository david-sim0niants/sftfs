#include "core.h"
#include "logging.h"
#include "ssh_cli.h"

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include <stdlib.h>

#define FUSE_USE_VERSION 31
#include <fuse.h>

struct sftfs {
    ssh_session ssh;
    sftp_session sftp;
};

struct fuse_operations ops = {
    .getattr = sftfs_getattr,
};

int main(int argc, char *argv[])
{
    struct sftfs sftfs;

    int rc = sftfs_ssh_cli(&argc, &argv, &sftfs.ssh);
    if (rc != EXIT_SUCCESS)
        return rc;

    sftfs.sftp = sftp_new(sftfs.ssh);
    if (NULL == sftfs.sftp) {
        sftfs_fatal("Failed allocating SFTP session: %s\n", ssh_get_error(sftfs.sftp));
        return EXIT_FAILURE;
    }

    rc = sftp_init(sftfs.sftp);
    if (rc != SSH_OK) {
        sftfs_fatal("Failed initializing SFTP session: [ %d ]\n", sftp_get_error(sftfs.sftp));
        return EXIT_FAILURE;
    }

    rc = fuse_main(argc, argv, &ops, sftfs.sftp);

    sftp_free(sftfs.sftp);
    return rc;
}
