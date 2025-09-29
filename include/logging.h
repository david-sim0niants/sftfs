#pragma once

#include <stdio.h>
#include <stdarg.h>

#define SFTFS_TRACE_VAL 0
#define SFTFS_DEBUG_VAL 1
#define SFTFS_INFO_VAL  2
#define SFTFS_WARN_VAL  3
#define SFTFS_ERROR_VAL 4
#define SFTFS_FATAL_VAL 5
#define SFTFS_OFF_VAL   6

typedef enum sftfs_log_severity {
    SFTFS_TRACE = SFTFS_TRACE_VAL,
    SFTFS_DEBUG = SFTFS_DEBUG_VAL,
    SFTFS_INFO  = SFTFS_INFO_VAL,
    SFTFS_WARN  = SFTFS_WARN_VAL,
    SFTFS_ERROR = SFTFS_ERROR_VAL,
    SFTFS_FATAL = SFTFS_FATAL_VAL,
    SFTFS_OFF   = SFTFS_OFF_VAL,
} sftfs_log_severity_t;

typedef int (*sftfs_logger_endpoint_t)(sftfs_log_severity_t severity, const char *fmt, ...);

extern sftfs_logger_endpoint_t logger_endpoint;

static inline sftfs_logger_endpoint_t sftfs_get_logger_endpoint(void)
{
    sftfs_logger_endpoint_t endp;
    __atomic_load(&logger_endpoint, &endp, __ATOMIC_SEQ_CST);
    return endp;
}

/** Set another logger endpoint. */
void sftfs_set_logger_endpoint(sftfs_logger_endpoint_t endpoint);

/** Default logger endpoint which prints logs onto stderr. */
int sftfs_default_logger_endpoint(sftfs_log_severity_t severity, const char *fmt, ...);

#ifndef SFTFS_ACTIVE_LOGGING_SEVERITY
    #ifdef NDEBUG
        #define SFTFS_ACTIVE_LOGGING_SEVERITY SFTFS_INFO_VAL
    #else
        #define SFTFS_ACTIVE_LOGGING_SEVERITY SFTFS_TRACE_VAL
    #endif
#endif

static void sftfs_log_check_format(sftfs_log_severity_t severity, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

static inline void sftfs_log_check_format(sftfs_log_severity_t severity, const char *fmt, ...)
{
    (void)severity; (void)fmt;
}

#define sftfs_log(severity, ...) do { \
    sftfs_log_check_format(severity, __VA_ARGS__); \
    logger_endpoint(severity, __VA_ARGS__); \
} while (0)

#if SFTFS_TRACE_VAL >= SFTFS_ACTIVE_LOGGING_SEVERITY
    #define sftfs_trace(...) sftfs_log(SFTFS_TRACE, __VA_ARGS__)
#else
    #define sftfs_trace(...) sftfs_log_check_format(SFTFS_TRACE, __VA_ARGS__)
#endif

#if SFTFS_DEBUG_VAL >= SFTFS_ACTIVE_LOGGING_SEVERITY
    #define sftfs_debug(...) sftfs_log(SFTFS_DEBUG, __VA_ARGS__)
#else
    #define sftfs_debug(...) sftfs_log_check_format(SFTFS_DEBUG, __VA_ARGS__)
#endif

#if SFTFS_INFO_VAL >= SFTFS_ACTIVE_LOGGING_SEVERITY
    #define sftfs_info(...) sftfs_log(SFTFS_INFO, __VA_ARGS__)
#else
    #define sftfs_info(...) sftfs_log_check_format(SFTFS_INFO, __VA_ARGS__)
#endif

#if SFTFS_WARN_VAL >= SFTFS_ACTIVE_LOGGING_SEVERITY
    #define sftfs_warn(...) sftfs_log(SFTFS_WARN, __VA_ARGS__)
#else
    #define sftfs_warn(...) sftfs_log_check_format(SFTFS_WARN, __VA_ARGS__)
#endif

#if SFTFS_ERROR_VAL >= SFTFS_ACTIVE_LOGGING_SEVERITY
    #define sftfs_error(...) sftfs_log(SFTFS_ERROR, __VA_ARGS__)
#else
    #define sftfs_error(...) sftfs_log_check_format(SFTFS_ERROR, __VA_ARGS__)
#endif

#if SFTFS_FATAL_VAL >= SFTFS_ACTIVE_LOGGING_SEVERITY
    #define sftfs_fatal(...) sftfs_log(SFTFS_FATAL, __VA_ARGS__)
#else
    #define sftfs_fatal(...) sftfs_log_check_format(SFTFS_FATAL, __VA_ARGS__)
#endif

#if SFTFS_OFF_VAL >= SFTFS_ACTIVE_LOGGING_SEVERITY
    #define sftfs_print(...) sftfs_log(SFTFS_OFF, __VA_ARGS__)
#else
    #define sftfs_print(...) sftfs_log_check_format(SFTFS_OFF, __VA_ARGS__)
#endif
