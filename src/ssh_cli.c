#include "ssh_cli.h"
#include "logging.h"
#include "ssh.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

struct sftfs_ssh_config config;

struct args {
    int argc;
    char **argv;
};

static void consume_args(struct args *args, int consume_count)
{
    args->argc -= consume_count;
    args->argv += consume_count;
}

static char *parse_ssh_target(char *target)
{
    while (*target && *target != '@') ++target;
    if (*target == '\0')
        return NULL;

    *target = '\0';
    return target + 1;
}

static int get_ssh_target(struct args *args)
{
    char *target = args->argv[1];
    config.host = parse_ssh_target(target);

    if (config.host == NULL) {
        config.host = target;
        config.user = NULL;
    } else {
        config.user = target;
    }

    consume_args(args, 1);

    return EXIT_SUCCESS;
}

#define PORT_MAX 65535
#define DEFAULT_SSH_PORT 22

static int get_ssh_port(struct args *args)
{
    config.port = DEFAULT_SSH_PORT;

    int opt;
    while ((opt = getopt(args->argc, args->argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
            {
                int port = atoi(optarg);
                if (port < 0 || port > PORT_MAX) {
                    sftfs_error("Invalid port number: %d\n", port);
                    return EXIT_FAILURE;
                }
                config.port = (unsigned short)port;
                break;
            }
        }
    }

    consume_args(args, optind - 1);

    if (config.port == DEFAULT_SSH_PORT) {
        struct servent *se = getservbyname("ssh", "tcp");
        if (se)
            config.port = ntohs(se->s_port);
    }

    return EXIT_SUCCESS;
}

static int handle_host_verification(ssh_session ssh)
{
    if (ssh_session_is_known_server(ssh))
        return EXIT_SUCCESS;

    sftfs_print("The authenticity of host '%s' can't be established.\n"
                "Are you sure you want to continue connecting (yes/no)?", config.host);

    char response[4];
    while (scanf("%3s", response) < 1 || (strcmp(response, "yes") != 0 && strcmp(response, "no") != 0))
        sftfs_print("Please type 'yes' or 'no': ");

    if (strcmp(response, "no")) {
        sftfs_print("Host key verification failed\n");
        return EXIT_FAILURE;
    }

    if (ssh_session_update_known_hosts(ssh) != SSH_OK) {
        sftfs_fatal("Failed updating known hosts");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int handle_host_authorization(ssh_session ssh)
{
    sftfs_fatal("Authorization failure. Not implemented yet.\n");
    return EXIT_FAILURE;
}

static int handle_connection(ssh_session *ssh)
{
    if (sftfs_ssh_connect(&config, ssh) != SSH_OK)
        return EXIT_FAILURE;

    int rc;
    (void)(!(rc = handle_host_verification(*ssh)) && !(rc = handle_host_authorization(*ssh)));

    sftfs_ssh_disconnect(*ssh);

    return rc;
}

int sftfs_ssh_cli(int *argc_p, char ***argv_p, ssh_session *ssh)
{
    struct args args = {
        .argc = *argc_p,
        .argv = *argv_p,
    };

    char *cmd = args.argv[0];

    if (args.argc < 2) {
        sftfs_error("Required arguments not passed\n");
        return EXIT_FAILURE;
    }

    int rc;
    bool arg_parsing_succeeded = (! (rc = get_ssh_target(&args)) && !(rc = get_ssh_port(&args)));
    args.argv[0] = cmd;

    if (! arg_parsing_succeeded)
        return rc;

    rc = handle_connection(ssh);

    return rc;
}
