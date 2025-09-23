#include "ssh_cli.h"
#include "logging.h"
#include "ssh.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <getopt.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/mman.h>
#include <sys/prctl.h>

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

    int last_valid_optind = optind;
    opterr = 0;
    int opt;
    while ((opt = getopt(args->argc, args->argv, "+p:")) != -1) {
        switch (opt) {
            case 'p':
            {
                int port = atoi(optarg);
                if (port < 0 || port > PORT_MAX) {
                    sftfs_error("Invalid port number: %d\n", port);
                    return EXIT_FAILURE;
                }
                config.port = (unsigned short)port;
                last_valid_optind = optind;
                break;
            }
        }
    }

    consume_args(args, last_valid_optind - 1);

    if (config.port == DEFAULT_SSH_PORT) {
        struct servent *se = getservbyname("ssh", "tcp");
        if (se)
            config.port = ntohs(se->s_port);
    }

    return EXIT_SUCCESS;
}

int get_line(char *buffer, size_t size)
{
    if (NULL == fgets(buffer, size, stdin))
        return EOF;

    size_t len;
    for (len = 0; len < size; ++len)
        if (buffer[len] == '\n')
            break;

    int c;
    if (buffer[len] != '\n')
        while (c = getc(stdin), c != '\n' && c != EOF)
            ;
    else
        *buffer = '\0';

    return len;
}

static int handle_host_verification(ssh_session ssh)
{
    if (ssh_session_is_known_server(ssh))
        return EXIT_SUCCESS;

    sftfs_print("The authenticity of host '%s' can't be established.\n"
                "Are you sure you want to continue connecting (yes/no)? ", config.host);

    char response[4];
    bool is_yes = false, is_no = false;

    while (get_line(response, sizeof(response)) != EOF) {

        is_yes = ! strncmp(response, "yes", 3);
        is_no  = ! strncmp(response, "no", 2);

        if (is_yes || is_no)
            break;

        sftfs_print("Please type 'yes' or 'no': ");
    }

    if (is_no) {
        sftfs_print("Host key verification failed\n");
        return EXIT_FAILURE;
    }

    if (! is_yes)
        return EXIT_FAILURE;

    if (ssh_session_update_known_hosts(ssh) != SSH_OK) {
        sftfs_fatal("Failed updating known hosts");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

const char *get_current_username(void)
{
    const char *user = getenv("USER");
    if (user)
        return user;
    user = getenv("LOGNAME");
    if (user)
        return user;
    user = getlogin();
    if (! user)
        user = "";
    return user;
}

static int prompt_password(char *buffer, size_t buflen)
{
    char prompt[4096];
    snprintf(prompt, sizeof(prompt), "%s@%s's password: ",
            config.user ? config.user : get_current_username(), config.host);

    return ssh_getpass(prompt, buffer, buflen, false, false);
}

_Thread_local static int is_dumpable = 1;

static char *create_password_buffer(size_t size)
{
    char *password = malloc(size);
    if (! password) {
        sftfs_fatal("Failed allocating memory to store password\n");
        return NULL;
    }

    memset(password, 0, size);

    if (mlock(password, size) != 0) {
        sftfs_fatal("mlock() failed: %s\n", strerror(errno));
        free(password);
        return NULL;
    }

    if ((is_dumpable = prctl(PR_GET_DUMPABLE)) == -1 || prctl(PR_SET_DUMPABLE, 0) != 0) {
        if (is_dumpable == -1) {
            sftfs_fatal("prctl(PR_GET_DUMPABLE) failed: %s\n", strerror(errno));
            is_dumpable = 1;
        } else {
            sftfs_fatal("prctl(PR_SET_DUMPABLE, ...) failed: %s\n", strerror(errno));
        }
        munlock(password, size);
        free(password);
        return NULL;
    }

    return password;
}

static void delete_password_buffer(char *password, size_t size)
{
    memset(password, 0, size);
    if (munlock(password, size))
        sftfs_warn("Could not unlock password memory: munlock() failed: %s", strerror(errno));
    if (prctl(PR_SET_DUMPABLE, is_dumpable))
        sftfs_warn(
                "Could not reset PR_SET_DUMPABLE to its initial value: prctl(PR_SET_DUMPABLE) failed: %s",
                strerror(errno));
    free(password);
}

#define MAX_PASSWORD_LEN 4096

static int try_authorize_host(ssh_session ssh)
{
    char *password = create_password_buffer(MAX_PASSWORD_LEN);
    if (! password)
        return EFAULT;

    int rc = 0;
    rc = prompt_password(password, MAX_PASSWORD_LEN) == 0;
    if (rc >= 0) {
        enum ssh_auth_e auth_result = ssh_userauth_password(ssh, NULL, password);
        switch (auth_result) {
            case SSH_AUTH_SUCCESS:
                rc = 0;
                break;
            case SSH_AUTH_DENIED:
                sftfs_print("Permission denied: %s\n", ssh_get_error(ssh));
                rc = EAGAIN;
                break;
            case SSH_AUTH_ERROR:
            default:
                sftfs_fatal("Authentication failed: %s\n", ssh_get_error(ssh));
                rc = -1;
                break;
        }
    } else {
        sftfs_fatal("Password prompt failed\n");
    }

    delete_password_buffer(password, MAX_PASSWORD_LEN);

    return rc;
}

static int handle_host_authorization(ssh_session ssh)
{
#define PASSWORD_ATTEMPTS 3
    for (int i = 0; i < PASSWORD_ATTEMPTS; ++i)
        if (try_authorize_host(ssh) == SSH_OK)
            return 0;
    return -1;
}

static int handle_connection(ssh_session *ssh)
{
    if (sftfs_ssh_connect(&config, ssh) != SSH_OK)
        return EXIT_FAILURE;

    int rc;
    (void)( ! (rc = handle_host_verification(*ssh)) &&
            ! (rc = handle_host_authorization(*ssh)));

    if (rc != 0) {
        sftfs_ssh_disconnect(*ssh);
        *ssh = NULL;
    }

    return rc;
}

int sftfs_ssh_cli(int *argc_p, char ***argv_p, ssh_session *ssh)
{
    struct args args = {
        .argc = *argc_p,
        .argv = *argv_p,
    };
    *ssh = NULL;

    char *cmd = args.argv[0];

    if (args.argc < 2) {
        sftfs_error("Required arguments not passed\n");
        return EXIT_FAILURE;
    }

    int rc;
    bool arg_parsing_succeeded = (! (rc = get_ssh_target(&args)) && !(rc = get_ssh_port(&args)));
    args.argv[0] = cmd;

    if (arg_parsing_succeeded)
        rc = handle_connection(ssh);

    *argc_p = args.argc;
    *argv_p = args.argv;

    return rc;
}
