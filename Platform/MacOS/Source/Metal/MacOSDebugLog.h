#pragma once
// MacOSDebugLog.h — Debug logging utilities for Metal backend

#include <cstdio>
#include <cstdarg>

// #define METAL_DEBUG_LOG

namespace MacOSDebug {
	inline void Log(const char* fmt, ...) {
		va_list args;
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
		fprintf(stderr, "\n");
	}
}

#ifdef METAL_DEBUG_LOG
#define DLOG_RFLOW(level, fmt, ...) MacOSDebug::Log("[RFLOW:%d] " fmt, level, ##__VA_ARGS__)
#else
#define DLOG_RFLOW(level, fmt, ...) ((void)0)
#endif
