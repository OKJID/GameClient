/*
**  win32types_compat.h — Proxy to Platform/MacOS/Include/windows.h
**  All Win32 type definitions now live in the macOS shadow header.
*/
#pragma once
#ifdef __APPLE__
#include <windows.h>
#endif
