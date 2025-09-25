#include "core.h"
#include "endp.h"

static inline sftfs_endp get_endp()
{
    return fuse_get_context()->private_data;
}

int sftfs_getattr(const char *path, struct stat *stat, struct fuse_file_info *fi)
{
    (void)fi;
    return sftfs_endp_getattr(get_endp(), path, stat);
}

int sftfs_readlink(const char *path, char *buf, size_t bufsiz)
{
    assert(bufsiz > 0);
    return sftfs_endp_readlink(get_endp(), path, buf, bufsiz);
}
