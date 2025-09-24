#include "ssh_cli.h"
#include "logging.h"
#include "ssh.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <pwd.h>
#include <sys/mman.h>
#include <sys/prctl.h>

static int get_line(char *buffer, size_t size)
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

static int handle_host_verification(struct sftfs_ssh_config *config, ssh_session ssh)
{
    if (ssh_session_is_known_server(ssh))
        return EXIT_SUCCESS;

    sftfs_print("The authenticity of host '%s' can't be established.\n"
                "Are you sure you want to continue connecting (yes/no)? ", config->host);

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

static int prompt_password(struct sftfs_ssh_config *config, char *buffer, size_t buflen)
{
    char prompt[4096];
    snprintf(prompt, sizeof(prompt), "%s@%s's password: ",
            config->user ? config->user : get_current_username(), config->host);

    return ssh_getpass(prompt, buffer, buflen, false, false);
}

_Thread_local static int is_dumpable = 1;

static char *create_password_buffer(size_t size)
{
    char *password = calloc(size, 1);
    if (! password) {
        sftfs_fatal("Failed allocating memory to store password\n");
        return NULL;
    }

    if (mlock(password, size) != 0) {
        sftfs_fatal("mlock() failed: %s\n", strerror(errno));
        free(password);
        return NULL;
    }

    // TODO: disable ptrace, signal handling, etc.
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
    explicit_bzero(password, size);
    if (munlock(password, size))
        sftfs_warn("Could not unlock password memory: munlock() failed: %s", strerror(errno));
    if (prctl(PR_SET_DUMPABLE, is_dumpable))
        sftfs_warn(
                "Could not reset PR_SET_DUMPABLE to its initial value: prctl(PR_SET_DUMPABLE) failed: %s",
                strerror(errno));
    free(password);
}

#define MAX_PASSWORD_LEN 4096

static int try_authorize_host(struct sftfs_ssh_config *config, ssh_session ssh)
{
    char *password = create_password_buffer(MAX_PASSWORD_LEN);
    if (! password)
        return EFAULT;

    int rc = 0;
    rc = prompt_password(config, password, MAX_PASSWORD_LEN) == 0;
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

static int handle_host_authorization(struct sftfs_ssh_config *config, ssh_session ssh)
{
#define PASSWORD_ATTEMPTS 3
    for (int i = 0; i < PASSWORD_ATTEMPTS; ++i)
        if (try_authorize_host(config, ssh) == SSH_OK)
            return 0;
    return -1;
}

static ssh_session handle_connection(struct sftfs_ssh_config *config)
{
    ssh_session ssh;
    if (sftfs_ssh_connect(config, &ssh) != SSH_OK)
        return NULL;

    int rc;
    (void)( ! (rc = handle_host_verification(config, ssh)) &&
            ! (rc = handle_host_authorization(config, ssh)));

    if (rc != 0) {
        sftfs_ssh_disconnect(ssh);
        return NULL;
    }

    return ssh;
}

ssh_session sftfs_ssh_cli(const char *user, const char *host, int port)
{
    struct sftfs_ssh_config config = {
        .user = user,
        .host = host,
        .port = port,
    };
    return handle_connection(&config);
}
