#include "core.h"
#include "endp.h"

#define FUSE_USE_VERSION 31
#include <fuse.h>

int sftfs_getattr(const char *path, struct stat *stat)
{
    return sftfs_endp_getattr(fuse_get_context()->private_data, path, stat);
}
