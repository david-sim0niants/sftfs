#include "core.h"
#include "logging.h"
#include "ssh.h"
#include "ssh_cli.h"

#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <syslog.h>

#include <stdlib.h>

#define FUSE_USE_VERSION 31
#include <fuse.h>

struct sftfs {
    ssh_session ssh;
    sftp_session sftp;
};

static int init_sftp(sftp_session *session, ssh_session ssh)
{
    *session = NULL;
    sftp_session sftp = sftp_new(ssh);
    if (NULL == sftp) {
        sftfs_fatal("Failed allocating SFTP session: %s\n", ssh_get_error(ssh));
        return EXIT_FAILURE;
    }

    if (sftp_init(sftp) != SSH_OK) {
        sftfs_fatal("Failed initializing SFTP session: [ %d ]\n", sftp_get_error(sftp));
        sftp_free(sftp);
        return EXIT_FAILURE;
    }

    *session = sftp;

    return EXIT_SUCCESS;
}

static int syslog_endpoint(sftfs_log_severity_t severity, const char *fmt, va_list args)
{
    int syslog_pri;
    switch (severity) {
        case SFTFS_TRACE:
        case SFTFS_DEBUG:
            syslog_pri = LOG_DEBUG;
            break;
        case SFTFS_INFO:
            syslog_pri = LOG_INFO;
            break;
        case SFTFS_WARN:
            syslog_pri = LOG_WARNING;
            break;
        case SFTFS_ERROR:
            syslog_pri = LOG_ERR;
            break;
        case SFTFS_FATAL:
            syslog_pri = LOG_CRIT;
            break;
        case SFTFS_OFF:
            syslog_pri = LOG_ALERT;
            break;
        default:
            syslog_pri = LOG_EMERG;
            break;
    }

    if (severity == SFTFS_TRACE)
        syslog(LOG_DEBUG, "[TRACE] ");
    vsyslog(syslog_pri, fmt, args);

    return 0;
}

struct fuse_operations ops = {
    .getattr = sftfs_getattr,
};

int main(int argc, char *argv[])
{
    struct sftfs sftfs;

    int rc = sftfs_ssh_cli(&argc, &argv, &sftfs.ssh);
    if (! sftfs.ssh)
        goto ssh_failed;

    rc = init_sftp(&sftfs.sftp, sftfs.ssh);
    if (! sftfs.sftp)
        goto sftp_failed;

    openlog(argv[0], LOG_PID | LOG_CONS, LOG_DAEMON);
    sftfs_set_logger_endpoint(syslog_endpoint);

    rc = fuse_main(argc, argv, &ops, sftfs.sftp);

    sftp_free(sftfs.sftp);
sftp_failed:
    sftfs_ssh_disconnect(sftfs.ssh);
ssh_failed:

    return rc;
}
