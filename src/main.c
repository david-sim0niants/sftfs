#include <stdio.h>

#define FUSE_USE_VERSION 31
#include <fuse.h>
#include <libssh/sftp.h>
#include <libssh/libssh.h>

struct sftfs_handle {
    ssh_session session;
} handle;

typedef int (*vprintf_like_t)(const char *fmt, va_list args);

static int sftfs_log_default_impl(const char *fmt, va_list args)
{
    return vfprintf(stderr, fmt, args);
}

static vprintf_like_t logger = sftfs_log_default_impl;

static void sftfs_log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

static void sftfs_log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    logger(fmt, args);
    va_end(args);
}

#define SFTFS_TRACE   "[\033[34mTRACE\033[0m] "
#define SFTFS_DEBUG   "[\033[36mDEBUG\033[0m] "
#define SFTFS_INFO    "[\033[32mINFO\033[0m] "
#define SFTFS_WARN    "[\033[33mWARN\033[0m] "
#define SFTFS_ERROR   "[\033[31mERROR\033[0m] "
#define SFTFS_FATAL   "[\033[37;41mFATAL\033[0m] "

#define sftfs_trace(...) sftfs_log(SFTFS_TRACE __VA_ARGS__)
#define sftfs_debug(...) sftfs_log(SFTFS_DEBUG __VA_ARGS__)
#define sftfs_info(...)  sftfs_log(SFTFS_INFO  __VA_ARGS__)
#define sftfs_warn(...)  sftfs_log(SFTFS_WARN  __VA_ARGS__)
#define sftfs_error(...) sftfs_log(SFTFS_ERROR __VA_ARGS__)
#define sftfs_fatal(...) sftfs_log(SFTFS_FATAL __VA_ARGS__)

static void sftfs_init(void)
{
    handle.session = ssh_new();
    if (1 || handle.session == NULL)
        sftfs_error("Failed creating SSH session\n");
}

static int sftfs_getattr(const char *path, struct stat *stat)
{
    return 0;
}

static void sftfs_destroy(void *data)
{
    struct sftfs_handle *handle = (struct sftfs_handle *)(data);
    ssh_free(handle->session);
}

static struct fuse_operations ops = {
    .getattr = sftfs_getattr,
    .destroy = sftfs_destroy,
};

// struct sftfs_config {
// }
//
int main(int argc, char *argv[])
{
    // struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    sftfs_init();

    return fuse_main(argc, argv, &ops, &handle);
}
