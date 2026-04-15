#pragma once
#ifdef __APPLE__
#include <unistd.h>
#include <signal.h>

#ifndef _P_NOWAIT
#define _P_NOWAIT 1
#endif

inline int _getpid() { return getpid(); }
#endif
