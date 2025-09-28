#include "core.h"
#include "logging.h"
#include "ssh.h"
#include "ssh_cli.h"

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>

struct sftfs_options {
    char *user_host_buffer;

    char *user;
    char *host;
    int port;
    int help;
};

enum {
    KEY_PORT,
    KEY_HELP,
};

static struct fuse_opt opt_specs[] = {
    FUSE_OPT_KEY("-p %d", KEY_PORT),
    FUSE_OPT_KEY("-h", KEY_HELP),
    FUSE_OPT_KEY("--help", KEY_HELP),
    FUSE_OPT_END
};

static void parse_user_host(char *user_host, char **user, char **host)
{
    char *sep = strchr(user_host, '@');
    if (sep) {
        *sep = '\0';
        *user = user_host;
        *host = sep + 1;
    } else {
        *user = NULL;
        *host = sep + 1;
    }
}

static int parse_port(const char *arg)
{
    char *endptr;
    errno = 0;
    long val = strtol(arg, &endptr, 10);
    if (errno != 0 || *endptr != '\0' || val < 0 || val > 65535)
        return -1;
    else
        return val;
}

static int process_ssh_options(void *data, const char *arg, int key,
                       struct fuse_args *outargs)
{
    (void)outargs;
    struct sftfs_options *opts = data;

    switch (key) {
        case KEY_PORT:
            opts->port = parse_port(arg + 2);
            if (opts->port < 0) {
                sftfs_error("Invalid port: %s\n", arg);
                return -1;
            }
            return 0;
        case FUSE_OPT_KEY_NONOPT:
            if (opts->user_host_buffer)
                return 1;
            opts->user_host_buffer = strdup(arg);
            parse_user_host(opts->user_host_buffer, &opts->user, &opts->host);
            return 0;
        case KEY_HELP:
            opts->help = 1;
            return 0;
        default:
            return 1;
    }
}

static void usage(const char *cmd)
{
    sftfs_print(
            "Usage: %s user@host mountpoint [options]\n\n"
            "Options:\n"
            "    -p PORT                specify port\n"
            "    -h, --help             show this help\n\n", cmd);
}

struct sftfs {
    ssh_session ssh;
    sftp_session sftp;
} sftfs;

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

static void deinit_sftp_and_ssh(void)
{
    sftp_free(sftfs.sftp);
    sftfs.sftp = NULL;
    sftfs_ssh_disconnect(sftfs.ssh);
    sftfs.ssh = NULL;
}

static void destroy(void *private_data)
{
    (void)private_data;
    deinit_sftp_and_ssh();
}

static struct fuse_operations ops = {
    .getattr = sftfs_getattr,
    .readlink = sftfs_readlink,
    .opendir = sftfs_opendir,
    .readdir = sftfs_readdir,
    .releasedir = sftfs_releasedir,
    .open = sftfs_open,
    .release = sftfs_release,
    .access = sftfs_access,
    .destroy = destroy,
};

int main(int argc, char *argv[])
{
    struct sftfs_options opts = {0};
    opts.port = 22;

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    if (fuse_opt_parse(&args, &opts, opt_specs, process_ssh_options) == -1)
        return EXIT_FAILURE;

    if (opts.help) {
        usage(argv[0]);
        sftfs_print("FUSE options\n");
        fuse_lib_help(&args);
        return EXIT_SUCCESS;
    }

    if (NULL == opts.host) {
        sftfs_print("Missing host, see `%s -h`\n", argv[0]);
        return EXIT_FAILURE;
    }

    sftfs.ssh = sftfs_ssh_cli(opts.user, opts.host, opts.port);
    if (! sftfs.ssh)
        return EXIT_FAILURE;

    int rc = init_sftp(&sftfs.sftp, sftfs.ssh);
    if (rc != EXIT_SUCCESS)
        deinit_sftp_and_ssh();

    fuse_opt_add_arg(&args, "-s"); // use single-threaded mode

    return fuse_main(args.argc, args.argv, &ops, sftfs.sftp);
}
