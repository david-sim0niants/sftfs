#include "core.h"
#include "logging.h"

#define FUSE_USE_VERSION 31
#include <fuse.h>

// static void sftfs_init(void)
// {
//     handle.session = ssh_new();
//     if (handle.session == NULL)
//         sftfs_error("Failed creating SSH session\n");
// }

// static void sftfs_destroy(void *data)
// {
//     struct sftfs_handle *handle = (struct sftfs_handle *)(data);
//     ssh_free(handle->session);
// }

static struct fuse_operations ops = {
    .getattr = sftfs_getattr,
    // .destroy = sftfs_destroy,
};

// struct sftfs_config {
// }
//
int main(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    // sftfs_init();

    return fuse_main(argc, argv, &ops, NULL);
}
