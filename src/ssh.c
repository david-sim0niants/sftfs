#include "ssh.h"
#include "logging.h"

static int decide_ssh_log_severity(void)
{
    return SSH_LOG_WARN;
}

int sftfs_ssh_connect(struct sftfs_ssh_config *config, ssh_session *session)
{
    ssh_session ssh = ssh_new();
    if (ssh == NULL) {
        sftfs_error("Failed creating a new ssh session\n");
        return -1;
    }

    if (ssh_options_set(ssh, SSH_OPTIONS_HOST, config->host) != SSH_OK) {
        sftfs_error("Failed setting ssh host\n");
        return -1;
    }

    if (ssh_options_set(ssh, SSH_OPTIONS_USER, config->user) != SSH_OK) {
        sftfs_error("Failed setting ssh user\n");
        return -1;
    }

    if (ssh_options_set(ssh, SSH_OPTIONS_PORT, &config->port) != SSH_OK) {
        sftfs_error("Failed setting ssh port\n");
        return -1;
    }

    int log_severity = decide_ssh_log_severity();
    if (ssh_options_set(ssh, SSH_OPTIONS_LOG_VERBOSITY, &log_severity) != SSH_OK) {
        sftfs_warn("Failed setting ssh log severity\n");
        return -1;
    }

    int rc = ssh_connect(ssh);
    if (rc != SSH_OK) {
        sftfs_error("SSH connection failed: %s\n", ssh_get_error(ssh));
        return rc;
    }

    *session = ssh;

    return 0;
}

void sftfs_ssh_disconnect(ssh_session ssh)
{
    ssh_disconnect(ssh);
    ssh_free(ssh);
}
