#pragma once
#ifdef __APPLE__

#include <d3d8.h>

#ifndef PALETTEENTRY_DEFINED
#define PALETTEENTRY_DEFINED
typedef struct { BYTE peRed; BYTE peGreen; BYTE peBlue; BYTE peFlags; } PALETTEENTRY;
#endif
#define D3DX_FILTER_NONE     0x00000001
#define D3DX_FILTER_POINT    0x00000002
#define D3DX_FILTER_LINEAR   0x00000003
#define D3DX_FILTER_TRIANGLE 0x00000004
#define D3DX_FILTER_BOX      0x00000005
#define D3DX_DEFAULT         0xFFFFFFFF

typedef struct _D3DXIMAGE_INFO {
    UINT Width;
    UINT Height;
    UINT Depth;
    UINT MipLevels;
    D3DFORMAT Format;
    DWORD ResourceType;
    DWORD ImageFileFormat;
} D3DXIMAGE_INFO;

inline HRESULT D3DXCreateTexture(
    IDirect3DDevice8 *pDevice,
    UINT Width, UINT Height, UINT MipLevels, DWORD Usage,
    D3DFORMAT Format, D3DPOOL Pool,
    IDirect3DTexture8 **ppTexture)
{
    return pDevice->CreateTexture(Width, Height, MipLevels, Usage, Format, Pool, ppTexture);
}

inline HRESULT D3DXCreateCubeTexture(
    IDirect3DDevice8 *pDevice,
    UINT Size, UINT MipLevels, DWORD Usage,
    D3DFORMAT Format, D3DPOOL Pool,
    IDirect3DCubeTexture8 **ppTexture)
{
    return pDevice->CreateCubeTexture(Size, MipLevels, Usage, Format, Pool, ppTexture);
}

inline HRESULT D3DXCreateTextureFromFileExA(
    IDirect3DDevice8 *pDevice,
    const char *pSrcFile,
    UINT Width, UINT Height, UINT MipLevels, DWORD Usage,
    D3DFORMAT Format, D3DPOOL Pool,
    DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey,
    D3DXIMAGE_INFO *pSrcInfo, PALETTEENTRY *pPalette,
    IDirect3DTexture8 **ppTexture)
{
    (void)pSrcFile; (void)Filter; (void)MipFilter;
    (void)ColorKey; (void)pSrcInfo; (void)pPalette;
    UINT w = (Width == D3DX_DEFAULT) ? 256 : Width;
    UINT h = (Height == D3DX_DEFAULT) ? 256 : Height;
    if (Format == D3DFMT_UNKNOWN) Format = D3DFMT_A8R8G8B8;
    return pDevice->CreateTexture(w, h, MipLevels, Usage, Format, Pool, ppTexture);
}

HRESULT WINAPI D3DXLoadSurfaceFromSurface(
    IDirect3DSurface8* pDestSurface, const void* pDestPalette, const RECT* pDestRect,
    IDirect3DSurface8* pSrcSurface, const void* pSrcPalette, const RECT* pSrcRect,
    DWORD Filter, DWORD ColorKey);

inline HRESULT D3DXLoadSurfaceFromMemory(
    IDirect3DSurface8*, const PALETTEENTRY*, const RECT*,
    const void*, D3DFORMAT, UINT, const PALETTEENTRY*,
    const RECT*, DWORD, D3DCOLOR)
{ return D3D_OK; }

HRESULT WINAPI D3DXFilterTexture(
    IDirect3DTexture8* pTexture, const void* pPalette, UINT SrcLevel, DWORD Filter);

#endif
