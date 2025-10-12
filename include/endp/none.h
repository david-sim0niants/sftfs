#pragma once

#include "endp/defs.h"
#include "logging.h"

#include <stdlib.h>

static inline int sftfs_endp_none(sftfs_endp endp, ...)
{
    (void)endp;
    sftfs_fatal("Function NOT implemented.\n");
    abort();
}

#ifndef SFTFS_ENDP_getattr
#define SFTFS_ENDP_getattr  sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_readlink
#define SFTFS_ENDP_readlink sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_mkdir
#define SFTFS_ENDP_mkdir    sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_unlink
#define SFTFS_ENDP_unlink   sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_rmdir
#define SFTFS_ENDP_rmdir    sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_symlink
#define SFTFS_ENDP_symlink  sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_rename
#define SFTFS_ENDP_rename   sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_chmod
#define SFTFS_ENDP_chmod    sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_chown
#define SFTFS_ENDP_chown    sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_opendir
#define SFTFS_ENDP_opendir  sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_readdir
#define SFTFS_ENDP_readdir  sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_closedir
#define SFTFS_ENDP_closedir sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_open
#define SFTFS_ENDP_open     sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_read
#define SFTFS_ENDP_read     sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_write
#define SFTFS_ENDP_write    sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_statfs
#define SFTFS_ENDP_statfs   sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_close
#define SFTFS_ENDP_close    sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_access
#define SFTFS_ENDP_access   sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_create
#define SFTFS_ENDP_create   sftfs_endp_none
#endif

#ifndef SFTFS_ENDP_utimens
#define SFTFS_ENDP_utimens  sftfs_endp_none
#endif
