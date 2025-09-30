#include "ssh_cli.h"
#include "ssh.h"
#include "logging.h"
#include "password_prompt.h"

#include <stdbool.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <pwd.h>

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

#define PROMPT_TEXT_MAXLEN 4096
static inline void sprint_prompt_text(char prompt_text[], const char *user, const char *host)
{
    snprintf(prompt_text, PROMPT_TEXT_MAXLEN,
             "%s@%s's password: ", user ? user : get_current_username(), host);
}

static int password_prompt_cb(const char *password, void *user_data)
{
    ssh_session ssh = user_data;
    enum ssh_auth_e auth_result = ssh_userauth_password(ssh, NULL, password);
    switch (auth_result) {
        case SSH_AUTH_SUCCESS:
            return 0;
        case SSH_AUTH_DENIED:
            sftfs_print("Permission denied: %s\n", ssh_get_error(ssh));
            return EAGAIN;
        case SSH_AUTH_ERROR:
        default:
            sftfs_fatal("Authentication failed: %s\n", ssh_get_error(ssh));
            return EPERM;
    }
}

static int try_authorize_host_with_password(struct sftfs_ssh_config *config, ssh_session ssh)
{
    char prompt_text[4096];
    sprint_prompt_text(prompt_text, config->user, config->host);

#define MAX_PASSWORD_LEN 4096
    struct sftfs_password_prompt_config prompt_config = {
        .buffer_size = MAX_PASSWORD_LEN,
        .flags = 0,
    };

#ifndef NDEBUG
    prompt_config.flags = SFTFS_PASSWORD_PROMPT_DISABLE_SIGNAL_HANDLING;
#endif

    return sftfs_password_prompt_wrap_critical_call(&prompt_config, prompt_text, password_prompt_cb, ssh);
}

static int handle_host_authorization(struct sftfs_ssh_config *config, ssh_session ssh)
{
#define PASSWORD_ATTEMPTS 3
    for (int i = 0; i < PASSWORD_ATTEMPTS; ++i) {
        int rc = try_authorize_host_with_password(config, ssh);
        if (rc == 0)
            return 0;
        else if (rc == EAGAIN)
            continue;
        else
            break;
    }
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
