#pragma once

#include "config.h"
#include "endp/defs.h"

#include "endp/none.h"

#ifdef SFTFS_SFTP 
#include "endp/sftp.h"
#endif

#ifdef SFTFS_CACHED
#include "cached.h"
#endif
