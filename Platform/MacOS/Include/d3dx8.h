#pragma once
#ifdef __APPLE__
#include <d3dx8math.h>

#ifndef D3DX_PI
#define D3DX_PI 3.14159265358979323846f
#endif

#define D3DX_FILTER_NONE     0x00000001
#define D3DX_FILTER_POINT    0x00000002
#define D3DX_FILTER_LINEAR   0x00000003
#define D3DX_FILTER_BOX      0x00000005
#define D3DX_FILTER_TRIANGLE 0x00000004

inline HRESULT D3DXLoadSurfaceFromSurface(
    IDirect3DSurface8*, const void*, const RECT*,
    IDirect3DSurface8*, const void*, const RECT*, DWORD, DWORD) { return 0; }

inline HRESULT D3DXLoadSurfaceFromMemory(
    IDirect3DSurface8*, const void*, const RECT*,
    const void*, D3DFORMAT, UINT, const void*, const RECT*,
    DWORD, DWORD) { return 0; }

typedef void* LPD3DXBUFFER;

#endif
