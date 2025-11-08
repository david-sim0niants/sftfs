#pragma once

#include "config.h"
#include "logging.h"

#ifdef SFTFS_FUNCTION_TRACING

inline static void _sftfs_trace_on_exit(const char **func_name)
{
    sftfs_trace("[LEAVE] %s(...)\n", *func_name);
}

#define SFTFS_TRACE_FUNC_IMPL(func_name) \
    sftfs_trace("[ENTER] %s(...)\n", func_name); \
    const char *_on_trace_exit __attribute__((cleanup(_sftfs_trace_on_exit))) = func_name;

#define SFTFS_TRACE_FUNC SFTFS_TRACE_FUNC_IMPL(__PRETTY_FUNCTION__)
#else
#define SFTFS_TRACE_FUNC
#endif
