#include "logging.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

_Thread_local sftfs_logger_endpoint_t logger_endpoint = sftfs_default_logger_endpoint;

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

#define TAG "sftfs: "

int sftfs_default_logger_endpoint(sftfs_log_severity_t severity, const char *fmt, va_list args)
{
    assert(severity >= SFTFS_TRACE);
    assert(severity <= SFTFS_OFF);

    if (isatty(fileno(stderr)))
        fprintf(stderr, "%s" TAG, colored_headings[severity]);
    else
        fprintf(stderr, "%s" TAG, uncolored_headings[severity]);

    return vfprintf(stderr, fmt, args);
}
