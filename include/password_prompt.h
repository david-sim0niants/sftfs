#pragma once

#include <stdbool.h>
#include <stdlib.h>

enum sftfs_password_prompt_flags {
    SFTFS_PASSWORD_PROMPT_VERIFY = 1,
};

struct sftfs_password_prompt_config {
    size_t buffer_size;
    int flags;
};

typedef struct sftfs_password_prompt_s *sftfs_password_prompt;

sftfs_password_prompt sftfs_password_prompt_start(struct sftfs_password_prompt_config *config);
int sftfs_password_prompt_stop(sftfs_password_prompt prompt);

int sftfs_password_prompt_do(sftfs_password_prompt prompt, const char *prompt_text);

const char *sftfs_password_prompt_get_password(sftfs_password_prompt prompt);

typedef int (*sftfs_password_prompt_crit_func)(const char *password, void *user_data);

static inline int sftfs_password_prompt_wrap_critical_call(
        struct sftfs_password_prompt_config *config,
        const char *prompt_text,
        sftfs_password_prompt_crit_func crit_func,
        void *user_data)
{
    sftfs_password_prompt prompt = sftfs_password_prompt_start(config);
    if (! prompt)
        return -1;

    int rc;
    (void)( ! (rc = sftfs_password_prompt_do(prompt, prompt_text)) &&
            ! (rc = crit_func(sftfs_password_prompt_get_password(prompt), user_data)) );

    sftfs_password_prompt_stop(prompt);

    return rc;
}
