#pragma once

#ifdef __APPLE__

#include <windows.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>

#ifndef __forceinline
#define __forceinline inline __attribute__((always_inline))
#endif

#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif

#ifndef _MAX_FNAME
#define _MAX_FNAME 256
#endif

#ifndef _MAX_EXT
#define _MAX_EXT 256
#endif

#ifndef _MAX_DIR
#define _MAX_DIR 256
#endif

#ifndef _MAX_DRIVE
#define _MAX_DRIVE 3
#endif

#endif // __APPLE__
