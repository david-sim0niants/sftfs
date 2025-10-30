#include "config.h"
#include "core.h"
#include "logging.h"
#include "ssh_cli.h"
#include "endp/sftp_init.h"
#ifdef SFTFS_CACHED
#include "endp/cached_init.h"
#endif

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
    if (NULL == sftfs.ssh)
        return EXIT_FAILURE;

    struct sftfs_sftp_params sftp_params = {
        .ssh = sftfs.ssh,
        .work_dir = opts->path,
    };

#ifdef SFTFS_CACHED
    struct sftfs_cached_params cached_params = {
        .ttl = 1000,
    };

    SFTFS_CACHED_INIT(sftfs.endp, &cached_params, struct sftfs_sftp, sftfs_sftp_construct, &sftp_params);
#else
    sftfs.endp = sftfs_sftp_init(&sftp_params);
#endif

    if (NULL == sftfs.endp) {
        ssh_free(sftfs.ssh);
        sftfs.ssh = NULL;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static void deinit_sftfs(void)
{
    if (sftfs.endp) {
#ifdef SFTFS_CACHED
        SFTFS_CACHED_DEINIT(sftfs.endp, sftfs_sftp_destruct);
#else
        sftfs_sftp_deinit(sftfs.endp);
#endif
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
    .mkdir = sftfs_mkdir,
    .unlink = sftfs_unlink,
    .rmdir = sftfs_rmdir,
    .symlink = sftfs_symlink,
    .rename = sftfs_rename,
    .chmod = sftfs_chmod,
    .chown = sftfs_chown,
    .opendir = sftfs_opendir,
    .readdir = sftfs_readdir,
    .releasedir = sftfs_releasedir,
    .open = sftfs_open,
    .read = sftfs_read,
    .write = sftfs_write,
    .statfs = sftfs_statfs,
    .release = sftfs_release,
    .access = sftfs_access,
    .create = sftfs_create,
    .utimens = sftfs_utimens,
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

    if (EXIT_SUCCESS != init_sftfs(&opts))
        return EXIT_FAILURE;

    fuse_opt_add_arg(&args, "-s"); // use single-threaded mode

    return fuse_main(args.argc, args.argv, &ops, sftfs.endp);
}
