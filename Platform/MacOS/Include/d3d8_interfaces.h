/*
**  d3d8_interfaces.h — D3D8 data structs + COM interfaces
**  Split: structs here, COM in d3d8_com.h
*/
#pragma once
#ifdef __APPLE__

#ifndef FLOAT
typedef float FLOAT;
#endif
typedef uintptr_t UINT_PTR;
typedef intptr_t INT_PTR;

typedef uint32_t D3DCOLOR;

typedef struct _D3DCOLORVALUE { float r, g, b, a; } D3DCOLORVALUE;

typedef struct _D3DMATRIX {
  union {
    struct { float _11,_12,_13,_14, _21,_22,_23,_24, _31,_32,_33,_34, _41,_42,_43,_44; };
    float m[4][4];
  };
} D3DMATRIX;

typedef struct _D3DVECTOR { float x, y, z; } D3DVECTOR;
typedef struct _D3DLOCKED_RECT { INT Pitch; void *pBits; } D3DLOCKED_RECT;
typedef struct _D3DLOCKED_BOX { int RowPitch; int SlicePitch; void *pBits; } D3DLOCKED_BOX;
typedef struct _D3DRECT { long x1, y1, x2, y2; } D3DRECT;

typedef struct _D3DVIEWPORT8 {
  DWORD X, Y, Width, Height; float MinZ, MaxZ;
} D3DVIEWPORT8;

typedef struct _D3DMATERIAL8 {
  D3DCOLORVALUE Diffuse, Ambient, Specular, Emissive; float Power;
} D3DMATERIAL8;

typedef struct _D3DLIGHT8 {
  D3DLIGHTTYPE Type;
  D3DCOLORVALUE Diffuse, Ambient, Specular;
  D3DVECTOR Position, Direction;
  float Range, Falloff, Attenuation0, Attenuation1, Attenuation2, Theta, Phi;
} D3DLIGHT8;

typedef struct _D3DGAMMARAMP { WORD red[256]; WORD green[256]; WORD blue[256]; } D3DGAMMARAMP;
typedef struct _D3DDISPLAYMODE { UINT Width, Height, RefreshRate; D3DFORMAT Format; } D3DDISPLAYMODE;

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID { unsigned long Data1; unsigned short Data2, Data3; unsigned char Data4[8]; } GUID;
#endif

#ifndef _LARGE_INTEGER_DEFINED
#define _LARGE_INTEGER_DEFINED
typedef union _LARGE_INTEGER { struct { DWORD LowPart; LONG HighPart; }; long long QuadPart; } LARGE_INTEGER;
#endif

typedef struct _D3DADAPTER_IDENTIFIER8 {
  char Driver[512]; char Description[512];
  LARGE_INTEGER DriverVersion;
  DWORD VendorId, DeviceId, SubSysId, Revision;
  GUID DeviceIdentifier; DWORD WHQLLevel;
} D3DADAPTER_IDENTIFIER8;

typedef struct _D3DPRESENT_PARAMETERS {
  UINT BackBufferWidth, BackBufferHeight;
  D3DFORMAT BackBufferFormat; UINT BackBufferCount;
  D3DMULTISAMPLE_TYPE MultiSampleType; D3DSWAPEFFECT SwapEffect;
  HWND hDeviceWindow; BOOL Windowed, EnableAutoDepthStencil;
  D3DFORMAT AutoDepthStencilFormat; DWORD Flags;
  UINT FullScreen_RefreshRateInHz, FullScreen_PresentationInterval;
} D3DPRESENT_PARAMETERS;

typedef struct _D3DCAPS8 {
  DWORD DeviceType; UINT AdapterOrdinal;
  DWORD Caps, Caps2, Caps3, PresentationIntervals, CursorCaps, DevCaps;
  DWORD PrimitiveMiscCaps, RasterCaps, ZCmpCaps, SrcBlendCaps, DestBlendCaps;
  DWORD AlphaCmpCaps, ShadeCaps, TextureCaps, TextureFilterCaps;
  DWORD CubeTextureFilterCaps, VolumeTextureFilterCaps;
  DWORD TextureAddressCaps, VolumeTextureAddressCaps, LineCaps;
  DWORD MaxTextureWidth, MaxTextureHeight, MaxVolumeExtent;
  DWORD MaxTextureRepeat, MaxTextureAspectRatio, MaxAnisotropy;
  float MaxVertexW;
  float GuardBandLeft, GuardBandTop, GuardBandRight, GuardBandBottom, ExtentsAdjust;
  DWORD StencilCaps, FVFCaps, TextureOpCaps;
  DWORD MaxTextureBlendStages, MaxSimultaneousTextures;
  DWORD VertexProcessingCaps, MaxActiveLights, MaxUserClipPlanes;
  DWORD MaxVertexBlendMatrices, MaxVertexBlendMatrixIndex;
  float MaxPointSize;
  DWORD MaxPrimitiveCount, MaxVertexIndex, MaxStreams, MaxStreamStride;
  DWORD VertexShaderVersion, MaxVertexShaderConst, PixelShaderVersion;
  float MaxPixelShaderValue;
} D3DCAPS8;

typedef struct _D3DSURFACE_DESC {
  D3DFORMAT Format; D3DRESOURCETYPE Type; DWORD Usage; D3DPOOL Pool;
  UINT Size; D3DMULTISAMPLE_TYPE MultiSampleType; UINT Width, Height;
} D3DSURFACE_DESC;

typedef struct _D3DVOLUME_DESC {
  D3DFORMAT Format; D3DRESOURCETYPE Type; DWORD Usage; D3DPOOL Pool;
  UINT Width, Height, Depth;
} D3DVOLUME_DESC;

typedef struct _D3DINDEXBUFFER_DESC {
  D3DFORMAT Format; D3DRESOURCETYPE Type; DWORD Usage; D3DPOOL Pool; UINT Size;
} D3DINDEXBUFFER_DESC;

typedef struct _D3DVERTEXBUFFER_DESC {
  D3DFORMAT Format; D3DRESOURCETYPE Type; DWORD Usage; D3DPOOL Pool; UINT Size; DWORD FVF;
} D3DVERTEXBUFFER_DESC;

#include "d3d8_com.h"

#endif // __APPLE__
