#include "core.h"
#include "endp.h"

int sftfs_getattr(const char *path, struct stat *stat, struct fuse_file_info *fi)
{
    (void)fi;
    return sftfs_endp_getattr(fuse_get_context()->private_data, path, stat);
}
