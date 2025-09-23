#include "core.h"
#include "logging.h"

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
    KEY_NONOPT,
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

static void destroy(void *private_data)
{
    (void)private_data;
    sftp_free(sftfs.sftp);
    ssh_free(sftfs.ssh);
}

static struct fuse_operations ops = {
    .getattr = sftfs_getattr,
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

    return fuse_main(args.argc, args.argv, &ops, NULL);
}
