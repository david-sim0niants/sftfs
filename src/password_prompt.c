#include "password_prompt.h"
#include "logging.h"

#include <assert.h>
#include <stdatomic.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <sys/mman.h>
#include <sys/prctl.h>

#include <libssh/libssh.h>

struct sftfs_password_prompt_s {
    char *password;
    size_t password_buffer_size;
    int flags;
    int is_dumpable;
    struct sigaction handlers[NSIG];
};

static int set_nodumpable()
{
    int is_dumpable = prctl(PR_GET_DUMPABLE);
    if (is_dumpable == -1)
        sftfs_fatal("prctl(PR_GET_DUMPABLE) failed: %s\n", strerror(errno));

    if (prctl(PR_SET_DUMPABLE, 0) != 0)
        sftfs_fatal("prctl(PR_SET_DUMPABLE, ...) failed: %s\n", strerror(errno));

    return is_dumpable;
}

static void reset_nodumpable(int is_dumpable)
{
    if (! prctl(PR_SET_DUMPABLE, is_dumpable))
        return;
    sftfs_warn("Could not reset PR_SET_DUMPABLE to its initial value: prctl(PR_SET_DUMPABLE) failed: %s",
               strerror(errno));
}

static inline bool is_safe_signal(int sig)
{
    switch (sig) {
        case SIGCHLD:
        case SIGCONT:
        case SIGURG:
        case SIGWINCH:
        case SIGPOLL:
        case SIGPWR:
        case 32:
        case 33:
            return true;
        default:
            if (sig == SIGIO)
                return true;
#ifdef SIGINFO
            if (sig == SIGINFO)
                return true;
#endif
            return false;
    }
}

static sftfs_password_prompt ongoing_prompt;

static void password_erasing_signal_handler(int sig)
{
    if (! ongoing_prompt)
        raise(SIGKILL); // very bad

    explicit_bzero(ongoing_prompt->password, ongoing_prompt->password_buffer_size);

    struct sigaction *prev_sigaction = &ongoing_prompt->handlers[sig];

    if (prev_sigaction->sa_handler != SIG_DFL && prev_sigaction->sa_handler != SIG_IGN) {
        prev_sigaction->sa_handler(sig);
    } else {
        sigaction(sig, prev_sigaction, NULL);
        raise(sig);
    }
}

static int uncover_unsafe_signals(sftfs_password_prompt prompt, int failed_signal)
{
    for (int sig = 1; sig < failed_signal; ++sig) {
        if (is_safe_signal(sig) || sig == SIGKILL || sig == SIGSTOP)
            continue;

        assert(prompt->handlers[sig].sa_handler != password_erasing_signal_handler);
        if (sigaction(sig, &prompt->handlers[sig], NULL) != 0) {
            sftfs_fatal("In %s sigaction() failed for signal %d: %s\n", __func__, sig, strerror(errno));
            ongoing_prompt = NULL;
            return -1;
        }
        memset(&prompt->handlers[sig], 0, sizeof(prompt->handlers[sig]));
    }
    ongoing_prompt = NULL;
    return 0;
}

static int cover_unsafe_signals(sftfs_password_prompt prompt)
{
    struct sigaction covered_handler = {
        .sa_flags = 0,
        .sa_handler = password_erasing_signal_handler,
    };

    if (sigemptyset(&covered_handler.sa_mask) != 0) {
        sftfs_fatal("sigemptyset() failed: %s\n", strerror(errno));
        return -1;
    }

    ongoing_prompt = prompt;

    for (int sig = 1; sig < NSIG; ++sig) {
        if (is_safe_signal(sig) || sig == SIGKILL || sig == SIGSTOP)
            continue;

        if (sigaction(sig, &covered_handler, &prompt->handlers[sig]) != 0) {
            sftfs_fatal("In %s sigaction() failed for signal %d: %s\n", __func__, sig, strerror(errno));
            uncover_unsafe_signals(prompt, sig);
            return -1;
        }
    }

    return 0;
}

sftfs_password_prompt sftfs_password_prompt_start(struct sftfs_password_prompt_config *config)
{
    sftfs_password_prompt prompt = calloc(1, sizeof(struct sftfs_password_prompt_s));
    if (! prompt) {
        sftfs_fatal("Failed allocating prompt handle memory\n");
        goto prompt_alloc_failed;
    }

    prompt->flags = config->flags;

    prompt->password = calloc(config->buffer_size, 1);
    if (! prompt->password) {
        sftfs_fatal("Failed allocating memory to store password\n");
        goto password_alloc_failed;
    }
    prompt->password_buffer_size = config->buffer_size;

    if (mlock(prompt->password, prompt->password_buffer_size) != 0) {
        sftfs_fatal("mlock() failed: %s\n", strerror(errno));
        goto mlock_failed;
    }

    prompt->is_dumpable = set_nodumpable();
    if (prompt->is_dumpable < 0)
        goto set_dumpable_failed;

    if (! (prompt->flags & SFTFS_PASSWORD_PROMPT_DISABLE_SIGNAL_HANDLING))
        if (cover_unsafe_signals(prompt) != 0)
            goto signal_handling_failed;

    return prompt;

signal_handling_failed:
    reset_nodumpable(prompt->is_dumpable);
set_dumpable_failed:
    munlock(prompt->password, prompt->password_buffer_size);
mlock_failed:
    free(prompt->password);
password_alloc_failed:
    free(prompt);
prompt_alloc_failed:
    return NULL;
}

int sftfs_password_prompt_stop(sftfs_password_prompt prompt)
{
    int rc = 0;

    explicit_bzero(prompt->password, prompt->password_buffer_size);

    if (! (prompt->flags & SFTFS_PASSWORD_PROMPT_DISABLE_SIGNAL_HANDLING))
        rc = uncover_unsafe_signals(prompt, NSIG);

    reset_nodumpable(prompt->is_dumpable);

    if (munlock(prompt->password, prompt->password_buffer_size)) {
        sftfs_warn("Could not unlock password memory: munlock() failed: %s", strerror(errno));
        rc = -1;
    }

    free(prompt->password);

    explicit_bzero(prompt, sizeof(*prompt));
    free(prompt);

    return rc;
}

int sftfs_password_prompt_do(sftfs_password_prompt prompt, const char *prompt_text)
{
    int verify = !!(prompt->flags & SFTFS_PASSWORD_PROMPT_VERIFY);
    return ssh_getpass(prompt_text, prompt->password, prompt->password_buffer_size, false, verify);
}

const char *sftfs_password_prompt_get_password(sftfs_password_prompt prompt)
{
    return prompt->password;
}
