#include "logging.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

sftfs_logger_endpoint_t logger_endpoint = sftfs_default_logger_endpoint;

void sftfs_set_logger_endpoint(sftfs_logger_endpoint_t endpoint)
{
    __atomic_store(&logger_endpoint, &endpoint, __ATOMIC_SEQ_CST);
}

static const char *colored_headings[] = {
    "[\033[34mTRACE\033[0m] ",
    "[\033[36mDEBUG\033[0m] ",
    "[\033[32mINFO\033[0m] ",
    "[\033[33mWARN\033[0m] ",
    "[\033[31mERROR\033[0m] ",
    "[\033[37;41mFATAL\033[0m] ",
    ""
};

static const char *uncolored_headings[] = {
    "[TRACE] ",
    "[DEBUG] ",
    "[INFO] ",
    "[WARN] ",
    "[ERROR] ",
    "[FATAL] ",
    ""
};

int sftfs_default_logger_endpoint(sftfs_log_severity_t severity, const char *fmt, ...)
{
    assert(severity >= SFTFS_TRACE);
    assert(severity <= SFTFS_OFF);

    if (isatty(fileno(stderr)))
        fprintf(stderr, "%s", colored_headings[severity]);
    else
        fprintf(stderr, "%s", uncolored_headings[severity]);

    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(stderr, fmt, args);
    va_end(args);
    return ret;
}
