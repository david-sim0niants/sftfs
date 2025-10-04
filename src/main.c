#include "core.h"
#include "endp.h"
#include "endp/sftp.h"
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
    char *target_buffer;

    char *user;
    char *host;
    char *path;
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

static void parse_target(char *target, char **user, char **host, char **path)
{
    char *sep = strchr(target, '@');
    if (sep) {
        *sep = '\0';
        *user = target;
        *host = sep + 1;
    } else {
        *user = NULL;
        *host = sep + 1;
    }

    sep = strchr(*host, ':');
    if (sep) {
        *path = sep + 1;
        *sep = '\0';
    } else {
        path = NULL;
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
            if (opts->target_buffer)
                return 1;
            opts->target_buffer = strdup(arg);
            parse_target(opts->target_buffer, &opts->user, &opts->host, &opts->path);
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
            "Usage: %s [user@]host:[directory] mountpoint [options]\n\n"
            "Options:\n"
            "    -p PORT                specify port\n"
            "    -h, --help             show this help\n\n", cmd);
}

struct sftfs {
    ssh_session ssh;
    sftfs_endp endp;
} sftfs;

static int init_sftfs(struct sftfs_options *opts)
{
    sftfs.ssh = sftfs_ssh_cli(opts->user, opts->host, opts->port);
    if (! sftfs.ssh)
        return EXIT_FAILURE;

    struct sftfs_endp_sftp_config config = {
        .work_dir = opts->path,
    };
    sftfs.endp = sftfs_endp_init(sftfs.ssh, &config);

    if (! sftfs.endp) {
        ssh_free(sftfs.ssh);
        sftfs.ssh = NULL;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static void deinit_sftfs(void)
{
    if (sftfs.endp) {
        sftfs_endp_deinit(sftfs.endp);
        sftfs.endp = NULL;
    }

    if (sftfs.ssh) {
        ssh_free(sftfs.ssh);
        sftfs.ssh = NULL;
    }
}

static void destroy(void *private_data)
{
    (void)private_data;
    deinit_sftfs();
}

static struct fuse_operations ops = {
    .getattr = sftfs_getattr,
    .readlink = sftfs_readlink,
    .opendir = sftfs_opendir,
    .readdir = sftfs_readdir,
    .releasedir = sftfs_releasedir,
    .open = sftfs_open,
    .read = sftfs_read,
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

    if (NULL == opts.path) {
        sftfs_print("Missing path, see `%s -h`\n", argv[0]);
        return EXIT_FAILURE;
    }

    init_sftfs(&opts);

    fuse_opt_add_arg(&args, "-s"); // use single-threaded mode

    return fuse_main(args.argc, args.argv, &ops, sftfs.endp);
}
