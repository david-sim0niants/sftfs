#pragma once

#include "config.h"
#include "defs.h"

#include "none.h"

#ifdef SFTFS_SFTP 
#include "sftp.h"
#endif

#ifdef SFTFS_CACHED
#include "cached.h"
#endif
