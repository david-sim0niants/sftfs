#pragma once

#include <stdio.h>
#include <stdarg.h>

typedef enum sftfs_log_severity {
    SFTFS_TRACE = 0,
    SFTFS_DEBUG,
    SFTFS_INFO,
    SFTFS_WARN,
    SFTFS_ERROR,
    SFTFS_FATAL,
    SFTFS_OFF,
} sftfs_log_severity_t;

typedef int (*sftfs_logger_endpoint_t)(sftfs_log_severity_t severity, const char *fmt, va_list args);

extern _Thread_local sftfs_logger_endpoint_t logger_endpoint;

/** Set another logger endpoint. */
void sftfs_set_logger_endpoint(sftfs_logger_endpoint_t endpoint);

/** Default logger endpoint which prints logs onto stderr. */
int sftfs_default_logger_endpoint(sftfs_log_severity_t severity, const char *fmt, va_list args);

#ifndef SFTFS_ACTIVE_LOGGING_SEVERITY
    #ifdef NDEBUG
        #define SFTFS_ACTIVE_LOGGING_SEVERITY SFTFS_INFO
    #else
        #define SFTFS_ACTIVE_LOGGING_SEVERITY SFTFS_TRACE
    #endif
#endif

static inline void sftfs_log(sftfs_log_severity_t severity, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

static inline void sftfs_log(sftfs_log_severity_t severity, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (severity >= SFTFS_ACTIVE_LOGGING_SEVERITY)
        logger_endpoint(severity, fmt, args);
    va_end(args);
}

#define sftfs_trace(...) sftfs_log(SFTFS_TRACE, __VA_ARGS__)
#define sftfs_debug(...) sftfs_log(SFTFS_DEBUG, __VA_ARGS__)
#define sftfs_info(...)  sftfs_log(SFTFS_INFO,  __VA_ARGS__)
#define sftfs_warn(...)  sftfs_log(SFTFS_WARN,  __VA_ARGS__)
#define sftfs_error(...) sftfs_log(SFTFS_ERROR, __VA_ARGS__)
#define sftfs_fatal(...) sftfs_log(SFTFS_FATAL, __VA_ARGS__)
#define sftfs_print(...) sftfs_log(SFTFS_OFF, __VA_ARGS__)
