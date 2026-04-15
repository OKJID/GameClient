/*
**  d3d8_compat.h — Proxy to Platform/MacOS/Include/d3d8.h
**  All D3D8 type definitions now live in the macOS shadow header.
*/
#pragma once
#ifdef __APPLE__
#include <d3d8.h>
#endif
