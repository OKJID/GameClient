/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifdef __APPLE__

#include <stdint.h>

// ── Basic Win32 types (guarded to coexist with win32types_compat.h) ────

#ifndef _D3D8_COMPAT_BASIC_TYPES_
#define _D3D8_COMPAT_BASIC_TYPES_

#ifndef BOOL
typedef int BOOL;
#endif
#ifndef BYTE
typedef unsigned char BYTE;
#endif
#ifndef WORD
typedef unsigned short WORD;
#endif
#ifndef DWORD
typedef unsigned int DWORD;
#endif
#ifndef UINT
typedef unsigned int UINT;
#endif
#ifndef INT
typedef int32_t INT;
#endif
#ifndef LONG
typedef int32_t LONG;
#endif
#ifndef ULONG
typedef uint32_t ULONG;
#endif
#ifndef FLOAT
typedef float FLOAT;
#endif

typedef uintptr_t UINT_PTR;
typedef intptr_t INT_PTR;

#ifndef LPVOID
typedef void *LPVOID;
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif // _D3D8_COMPAT_BASIC_TYPES_

// ── Handle types (guarded) ─────────────────────────────────────────────

#ifndef HWND
typedef void *HWND;
#endif
#ifndef HMONITOR
typedef void *HMONITOR;
#endif

// ── HRESULT ────────────────────────────────────────────────────────────

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef LONG HRESULT;
#endif

#ifndef S_OK
#define S_OK ((HRESULT)0L)
#endif
#ifndef S_FALSE
#define S_FALSE ((HRESULT)1L)
#endif
#ifndef E_FAIL
#define E_FAIL ((HRESULT)0x80004005L)
#endif
#ifndef E_NOTIMPL
#define E_NOTIMPL ((HRESULT)0x80004001L)
#endif
#ifndef E_NOINTERFACE
#define E_NOINTERFACE ((HRESULT)0x80004002L)
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif
#ifndef FAILED
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#endif

#ifndef HRESULT_FROM_WIN32
#define HRESULT_FROM_WIN32(x) ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) : ((HRESULT)(((x) & 0x0000FFFF) | 0x80070000)))
#endif

#ifndef IS_ERROR
#define IS_ERROR(Status) (((unsigned long)(Status)) >> 31 == 1)
#endif

// ── Calling conventions (no-ops on macOS) ──────────────────────────────

#ifndef WINAPI
#define WINAPI
#endif
#ifndef CONST
#define CONST const
#endif

// ── D3D constants ──────────────────────────────────────────────────────

#define D3D_OK 0L
#define D3D_SDK_VERSION 220

// ── D3D error codes ────────────────────────────────────────────────────

#define D3DERR_CONFLICTINGTEXTUREFILTER ((HRESULT)0x8876087EL)
#define D3DERR_CONFLICTINGTEXTUREPALETTE ((HRESULT)0x8876087FL)
#define D3DERR_DEVICELOST ((HRESULT)0x88760868L)
#define D3DERR_DEVICENOTRESET ((HRESULT)0x88760869L)
#define D3DERR_NOTFOUND ((HRESULT)0x88760866L)
#define D3DERR_MOREDATA ((HRESULT)0x88760867L)
#define D3DERR_DRIVERINTERNALERROR ((HRESULT)0x8876086cL)
#define D3DERR_OUTOFVIDEOMEMORY ((HRESULT)0x88760864L)
#define D3DERR_NOTAVAILABLE ((HRESULT)0x8876086aL)
#define D3DERR_TOOMANYOPERATIONS ((HRESULT)0x88760871L)
#define D3DERR_UNSUPPORTEDALPHAARG ((HRESULT)0x88760872L)
#define D3DERR_UNSUPPORTEDALPHAOPERATION ((HRESULT)0x88760873L)
#define D3DERR_UNSUPPORTEDCOLORARG ((HRESULT)0x88760874L)
#define D3DERR_UNSUPPORTEDCOLOROPERATION ((HRESULT)0x88760875L)
#define D3DERR_UNSUPPORTEDFACTORVALUE ((HRESULT)0x88760876L)
#define D3DERR_UNSUPPORTEDTEXTUREFILTER ((HRESULT)0x88760877L)
#define D3DERR_WRONGTEXTUREFORMAT ((HRESULT)0x88760878L)

// ── D3D lock / usage / clear flags ─────────────────────────────────────

#define D3DLOCK_READONLY 0x00000010L
#define D3DLOCK_DISCARD 0x00002000L
#define D3DLOCK_NOOVERWRITE 0x00001000L
#define D3DLOCK_NOSYSLOCK 0x00000800L
#define D3DLOCK_NO_DIRTY_UPDATE 0x00000001L

#define D3DUSAGE_RENDERTARGET 0x00000001L
#define D3DUSAGE_DEPTHSTENCIL 0x00000002L
#define D3DUSAGE_DYNAMIC 0x00000200L
#define D3DUSAGE_WRITEONLY 0x00000008L
#define D3DUSAGE_SOFTWAREPROCESSING 0x00000010L
#define D3DUSAGE_DONOTCLIP 0x00000020L
#define D3DUSAGE_POINTS 0x00000040L
#define D3DUSAGE_RTPATCHES 0x00000080L
#define D3DUSAGE_NPATCHES 0x00000100L

#define D3DADAPTER_DEFAULT 0
#define D3DCLEAR_TARGET 0x00000001
#define D3DCLEAR_ZBUFFER 0x00000002
#define D3DCLEAR_STENCIL 0x00000004

#define D3DCREATE_FPU_PRESERVE 0x00000002
#define D3DCREATE_HARDWARE_VERTEXPROCESSING 0x00000040
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING 0x00000020
#define D3DCREATE_MIXED_VERTEXPROCESSING 0x00000080

#define D3DPRESENT_INTERVAL_DEFAULT 0x00000000
#define D3DPRESENT_INTERVAL_ONE 0x00000001
#define D3DPRESENT_INTERVAL_TWO 0x00000002
#define D3DPRESENT_INTERVAL_THREE 0x00000004
#define D3DPRESENT_INTERVAL_FOUR 0x00000008
#define D3DPRESENT_INTERVAL_IMMEDIATE 0x80000000
#define D3DPRESENT_RATE_DEFAULT 0

#define D3DSGR_NO_CALIBRATION 0x00000000
#define D3DSGR_CALIBRATE 0x00000001

#define D3DENUM_NO_WHQL_LEVEL 0x00000002L

#ifndef D3DCURSOR_IMMEDIATE_UPDATE
#define D3DCURSOR_IMMEDIATE_UPDATE 0x00000001
#endif

// ── FVF defines ────────────────────────────────────────────────────────

#define D3DFVF_RESERVED0 0x001
#define D3DFVF_XYZ 0x002
#define D3DFVF_XYZRHW 0x004
#define D3DFVF_XYZB1 0x006
#define D3DFVF_XYZB2 0x008
#define D3DFVF_XYZB3 0x00a
#define D3DFVF_XYZB4 0x00c
#define D3DFVF_XYZB5 0x00e
#define D3DFVF_NORMAL 0x010
#define D3DFVF_PSIZE 0x020
#define D3DFVF_DIFFUSE 0x040
#define D3DFVF_SPECULAR 0x080
#define D3DFVF_TEX0 0x000
#define D3DFVF_TEX1 0x100
#define D3DFVF_TEX2 0x200
#define D3DFVF_TEX3 0x300
#define D3DFVF_TEX4 0x400
#define D3DFVF_TEX5 0x500
#define D3DFVF_TEX6 0x600
#define D3DFVF_TEX7 0x700
#define D3DFVF_TEX8 0x800

#define D3DFVF_TEXCOUNT_MASK 0x00000F00
#define D3DFVF_TEXCOUNT_SHIFT 8

#define D3DFVF_TEXTUREFORMAT2 0x0
#define D3DFVF_TEXTUREFORMAT1 0x3
#define D3DFVF_TEXTUREFORMAT3 0x1
#define D3DFVF_TEXTUREFORMAT4 0x2

#define D3DFVF_TEXCOORDSIZE3(Index) (D3DFVF_TEXTUREFORMAT3 << (Index * 2 + 16))
#define D3DFVF_TEXCOORDSIZE2(Index) (D3DFVF_TEXTUREFORMAT2 << (Index * 2 + 16))
#define D3DFVF_TEXCOORDSIZE4(Index) (D3DFVF_TEXTUREFORMAT4 << (Index * 2 + 16))
#define D3DFVF_TEXCOORDSIZE1(Index) (D3DFVF_TEXTUREFORMAT1 << (Index * 2 + 16))

#define D3DFVF_LASTBETA_UBYTE4 0x1000
#define D3DDP_MAXTEXCOORD 8

// ── Vertex shader declaration macros ───────────────────────────────────

#define D3DVSD_END() 0xFFFFFFFF
#define D3DVSD_STREAM(s) (0x80000000 | (s))
#define D3DVSD_REG(r, t) ((r) | ((t) << 16))

// ── Texture argument defines ───────────────────────────────────────────

#define D3DTA_DIFFUSE 0x00000000
#define D3DTA_CURRENT 0x00000001
#define D3DTA_TEXTURE 0x00000002
#define D3DTA_TFACTOR 0x00000003
#define D3DTA_SPECULAR 0x00000004
#define D3DTA_TEMP 0x00000005
#define D3DTA_COMPLEMENT 0x00000010
#define D3DTA_ALPHAREPLICATE 0x00000020
#define D3DTA_SELECTMASK 0x0000000f

// ── Texture coordinate index flags ─────────────────────────────────────

#define D3DTSS_TCI_PASSTHRU 0x00000000
#define D3DTSS_TCI_CAMERASPACEPOSITION 0x00010000
#define D3DTSS_TCI_CAMERASPACENORMAL 0x00020000
#define D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR 0x00030000

// ── Color write enable flags ───────────────────────────────────────────

#define D3DCOLORWRITEENABLE_RED (1L << 0)
#define D3DCOLORWRITEENABLE_GREEN (1L << 1)
#define D3DCOLORWRITEENABLE_BLUE (1L << 2)
#define D3DCOLORWRITEENABLE_ALPHA (1L << 3)

// ── Wrap flags ─────────────────────────────────────────────────────────

#define D3DWRAP_U 0x00000001
#define D3DWRAP_V 0x00000002
#define D3DWRAP_W 0x00000004

// ── Fog constants ──────────────────────────────────────────────────────

#define D3DFOG_NONE 0
#define D3DFOG_EXP 1
#define D3DFOG_EXP2 2
#define D3DFOG_LINEAR 3

// ── Material color source ──────────────────────────────────────────────

#define D3DMCS_MATERIAL 0
#define D3DMCS_COLOR1 1
#define D3DMCS_COLOR2 2

// ── Capabilities flags ─────────────────────────────────────────────────

#define D3DDEVCAPS_HWTRANSFORMANDLIGHT 0x00010000L
#define D3DDEVCAPS_NPATCHES 0x01000000L
#define D3DCAPS2_FULLSCREENGAMMA 0x00020000L

#define D3DPRASTERCAPS_ZBIAS 0x00004000L
#define D3DPRASTERCAPS_FOGRANGE 0x00010000
#define D3DPRASTERCAPS_FOGTABLE 0x00000100L
#define D3DPRASTERCAPS_FOGVERTEX 0x00000080L
#define D3DPRASTERCAPS_MIPMAPLODBIAS 0x00002000L
#define D3DPRASTERCAPS_ZTEST 0x00000010L
#define D3DPRASTERCAPS_ANISOTROPY 0x00020000L

#define D3DPMISCCAPS_COLORWRITEENABLE 0x00000080L
#define D3DPMISCCAPS_CULLNONE 0x00000010L
#define D3DPMISCCAPS_CULLCW 0x00000020L
#define D3DPMISCCAPS_CULLCCW 0x00000040L
#define D3DPMISCCAPS_BLENDOP 0x00000800L
#define D3DPMISCCAPS_MASKZ 0x00000002L

#define D3DPTEXTURECAPS_PERSPECTIVE 0x00000001L
#define D3DPTEXTURECAPS_ALPHA 0x00000004L
#define D3DPTEXTURECAPS_PROJECTED 0x00000400L
#define D3DPTEXTURECAPS_CUBEMAP 0x00000800L
#define D3DPTEXTURECAPS_MIPMAP 0x00004000L
#define D3DPTEXTURECAPS_MIPCUBEMAP 0x00010000L

#define D3DPTADDRESSCAPS_WRAP 0x00000001L
#define D3DPTADDRESSCAPS_MIRROR 0x00000002L
#define D3DPTADDRESSCAPS_CLAMP 0x00000004L
#define D3DPTADDRESSCAPS_BORDER 0x00000008L
#define D3DPTADDRESSCAPS_MIRRORONCE 0x00000010L

#define D3DPTFILTERCAPS_MINFPOINT 0x00000100L
#define D3DPTFILTERCAPS_MINFLINEAR 0x00000200L
#define D3DPTFILTERCAPS_MINFANISOTROPIC 0x00000400L
#define D3DPTFILTERCAPS_MIPFPOINT 0x00010000L
#define D3DPTFILTERCAPS_MIPFLINEAR 0x00020000L
#define D3DPTFILTERCAPS_MAGFPOINT 0x01000000L
#define D3DPTFILTERCAPS_MAGFLINEAR 0x02000000L
#define D3DPTFILTERCAPS_MAGFANISOTROPIC 0x04000000L

// ── Texture op capabilities ────────────────────────────────────────────

#define D3DTEXOPCAPS_DISABLE 0x00000001
#define D3DTEXOPCAPS_SELECTARG1 0x00000002
#define D3DTEXOPCAPS_SELECTARG2 0x00000004
#define D3DTEXOPCAPS_MODULATE 0x00000008
#define D3DTEXOPCAPS_MODULATE2X 0x00000010
#define D3DTEXOPCAPS_MODULATE4X 0x00000020
#define D3DTEXOPCAPS_ADD 0x00000040
#define D3DTEXOPCAPS_ADDSIGNED 0x00000080
#define D3DTEXOPCAPS_ADDSIGNED2X 0x00000100
#define D3DTEXOPCAPS_SUBTRACT 0x00000200
#define D3DTEXOPCAPS_ADDSMOOTH 0x00000400
#define D3DTEXOPCAPS_BLENDDIFFUSEALPHA 0x00000800
#define D3DTEXOPCAPS_BLENDTEXTUREALPHA 0x00001000
#define D3DTEXOPCAPS_BLENDFACTORALPHA 0x00002000
#define D3DTEXOPCAPS_BLENDTEXTUREALPHAPM 0x00004000
#define D3DTEXOPCAPS_BLENDCURRENTALPHA 0x00008000
#define D3DTEXOPCAPS_PREMODULATE 0x00010000
#define D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR 0x00020000
#define D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA 0x00040000
#define D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR 0x00080000
#define D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA 0x00100000
#define D3DTEXOPCAPS_BUMPENVMAP 0x00200000
#define D3DTEXOPCAPS_BUMPENVMAPLUMINANCE 0x00400000
#define D3DTEXOPCAPS_DOTPRODUCT3 0x00800000
#define D3DTEXOPCAPS_MULTIPLYADD 0x01000000
#define D3DTEXOPCAPS_LERP 0x02000000

// ── D3DFMT index formats ──────────────────────────────────────────────

#define D3DFMT_INDEX16 ((D3DFORMAT)101)
#define D3DFMT_INDEX32 ((D3DFORMAT)102)

// ============================================================================
// D3D8 Enumerations
// ============================================================================

typedef enum _D3DXIMAGE_FILEFORMAT {
  D3DXIFF_BMP = 0,
  D3DXIFF_JPG = 1,
  D3DXIFF_TGA = 2,
  D3DXIFF_PNG = 3,
  D3DXIFF_DDS = 4,
  D3DXIFF_PPM = 5,
  D3DXIFF_DIB = 6,
  D3DXIFF_HDR = 7,
  D3DXIFF_PFM = 8,
  D3DXIFF_FORCE_DWORD = 0x7fffffff
} D3DXIMAGE_FILEFORMAT;

typedef D3DXIMAGE_FILEFORMAT D3DIMAGE_FILEFORMAT;

typedef enum _D3DMULTISAMPLE_TYPE {
  D3DMULTISAMPLE_NONE = 0,
  D3DMULTISAMPLE_2_SAMPLES = 2,
  D3DMULTISAMPLE_3_SAMPLES = 3,
  D3DMULTISAMPLE_4_SAMPLES = 4,
  D3DMULTISAMPLE_5_SAMPLES = 5,
  D3DMULTISAMPLE_6_SAMPLES = 6,
  D3DMULTISAMPLE_7_SAMPLES = 7,
  D3DMULTISAMPLE_8_SAMPLES = 8,
  D3DMULTISAMPLE_9_SAMPLES = 9,
  D3DMULTISAMPLE_10_SAMPLES = 10,
  D3DMULTISAMPLE_11_SAMPLES = 11,
  D3DMULTISAMPLE_12_SAMPLES = 12,
  D3DMULTISAMPLE_13_SAMPLES = 13,
  D3DMULTISAMPLE_14_SAMPLES = 14,
  D3DMULTISAMPLE_15_SAMPLES = 15,
  D3DMULTISAMPLE_16_SAMPLES = 16,
  D3DMULTISAMPLE_FORCE_DWORD = 0xffffffff
} D3DMULTISAMPLE_TYPE;

typedef enum _D3DDEVTYPE {
  D3DDEVTYPE_HAL = 1,
  D3DDEVTYPE_REF = 2,
  D3DDEVTYPE_SW = 3,
  D3DDEVTYPE_FORCE_DWORD = 0xffffffff
} D3DDEVTYPE;

typedef enum _D3DPOOL {
  D3DPOOL_DEFAULT = 0,
  D3DPOOL_MANAGED = 1,
  D3DPOOL_SYSTEMMEM = 2,
  D3DPOOL_SCRATCH = 3,
  D3DPOOL_FORCE_DWORD = 0x7fffffff
} D3DPOOL;

typedef enum _D3DFORMAT {
  D3DFMT_UNKNOWN = 0,
  D3DFMT_R8G8B8 = 20,
  D3DFMT_A8R8G8B8 = 21,
  D3DFMT_X8R8G8B8 = 22,
  D3DFMT_R5G6B5 = 23,
  D3DFMT_X1R5G5B5 = 24,
  D3DFMT_A1R5G5B5 = 25,
  D3DFMT_A4R4G4B4 = 26,
  D3DFMT_R3G3B2 = 27,
  D3DFMT_A8 = 28,
  D3DFMT_A8R3G3B2 = 29,
  D3DFMT_X4R4G4B4 = 30,
  D3DFMT_D16_LOCKABLE = 70,
  D3DFMT_D32 = 71,
  D3DFMT_D15S1 = 73,
  D3DFMT_D24S8 = 75,
  D3DFMT_D24X4S4 = 79,
  D3DFMT_D24X8 = 77,
  D3DFMT_D16 = 80,
  D3DFMT_DXT1 = 0x31545844,
  D3DFMT_DXT2 = 0x32545844,
  D3DFMT_DXT3 = 0x33545844,
  D3DFMT_DXT4 = 0x34545844,
  D3DFMT_DXT5 = 0x35545844,
  D3DFMT_P8 = 41,
  D3DFMT_A8P8 = 40,
  D3DFMT_L8 = 50,
  D3DFMT_A8L8 = 51,
  D3DFMT_A4L4 = 52,
  D3DFMT_V8U8 = 60,
  D3DFMT_L6V5U5 = 61,
  D3DFMT_X8L8V8U8 = 62,
  D3DFMT_Q8W8V8U8 = 63,
  D3DFMT_V16U16 = 64,
  D3DFMT_W11V11U10 = 65,
  D3DFMT_UYVY = 0x59565955,
  D3DFMT_YUY2 = 0x32595559,
} D3DFORMAT;

typedef enum _D3DSWAPEFFECT {
  D3DSWAPEFFECT_DISCARD = 1,
  D3DSWAPEFFECT_FLIP = 2,
  D3DSWAPEFFECT_COPY = 3,
  D3DSWAPEFFECT_COPY_VSYNC = 4,
  D3DSWAPEFFECT_FORCE_DWORD = 0xffffffff
} D3DSWAPEFFECT;

typedef enum _D3DRESOURCETYPE {
  D3DRTYPE_SURFACE = 1,
  D3DRTYPE_VOLUME = 2,
  D3DRTYPE_TEXTURE = 3,
  D3DRTYPE_VOLUMETEXTURE = 4,
  D3DRTYPE_CUBETEXTURE = 5,
  D3DRTYPE_VERTEXBUFFER = 6,
  D3DRTYPE_INDEXBUFFER = 7,
  D3DRTYPE_FORCE_DWORD = 0x7fffffff
} D3DRESOURCETYPE;

typedef enum _D3DCUBEMAP_FACES {
  D3DCUBEMAP_FACE_POSITIVE_X = 0,
  D3DCUBEMAP_FACE_NEGATIVE_X = 1,
  D3DCUBEMAP_FACE_POSITIVE_Y = 2,
  D3DCUBEMAP_FACE_NEGATIVE_Y = 3,
  D3DCUBEMAP_FACE_POSITIVE_Z = 4,
  D3DCUBEMAP_FACE_NEGATIVE_Z = 5,
  D3DCUBEMAP_FACE_FORCE_DWORD = 0xffffffff
} D3DCUBEMAP_FACES;

typedef enum _D3DPRIMITIVETYPE {
  D3DPT_POINTLIST = 1,
  D3DPT_LINELIST = 2,
  D3DPT_LINESTRIP = 3,
  D3DPT_TRIANGLELIST = 4,
  D3DPT_TRIANGLESTRIP = 5,
  D3DPT_TRIANGLEFAN = 6,
  D3DPT_FORCE_DWORD = 0x7fffffff
} D3DPRIMITIVETYPE;

typedef enum _D3DBACKBUFFER_TYPE {
  D3DBACKBUFFER_TYPE_MONO = 0,
  D3DBACKBUFFER_TYPE_LEFT = 1,
  D3DBACKBUFFER_TYPE_RIGHT = 2,
  D3DBACKBUFFER_TYPE_FORCE_DWORD = 0x7fffffff
} D3DBACKBUFFER_TYPE;

typedef enum _D3DRENDERSTATETYPE {
  D3DRS_ZENABLE = 7,
  D3DRS_FILLMODE = 8,
  D3DRS_SHADEMODE = 9,
  D3DRS_ZWRITEENABLE = 14,
  D3DRS_ALPHATESTENABLE = 15,
  D3DRS_LASTPIXEL = 16,
  D3DRS_SRCBLEND = 19,
  D3DRS_DESTBLEND = 20,
  D3DRS_CULLMODE = 22,
  D3DRS_ZFUNC = 23,
  D3DRS_ALPHAREF = 24,
  D3DRS_ALPHAFUNC = 25,
  D3DRS_DITHERENABLE = 26,
  D3DRS_ALPHABLENDENABLE = 27,
  D3DRS_FOGENABLE = 28,
  D3DRS_SPECULARENABLE = 29,
  D3DRS_FOGCOLOR = 34,
  D3DRS_FOGTABLEMODE = 35,
  D3DRS_FOGSTART = 36,
  D3DRS_FOGEND = 37,
  D3DRS_FOGDENSITY = 38,
  D3DRS_EDGEANTIALIAS = 40,
  D3DRS_ZBIAS = 47,
  D3DRS_RANGEFOGENABLE = 48,
  D3DRS_STENCILENABLE = 52,
  D3DRS_STENCILFAIL = 53,
  D3DRS_STENCILZFAIL = 54,
  D3DRS_STENCILPASS = 55,
  D3DRS_STENCILFUNC = 56,
  D3DRS_STENCILREF = 57,
  D3DRS_STENCILMASK = 58,
  D3DRS_STENCILWRITEMASK = 59,
  D3DRS_TEXTUREFACTOR = 60,
  D3DRS_WRAP0 = 128,
  D3DRS_WRAP1 = 129,
  D3DRS_WRAP2 = 130,
  D3DRS_WRAP3 = 131,
  D3DRS_WRAP4 = 132,
  D3DRS_WRAP5 = 133,
  D3DRS_WRAP6 = 134,
  D3DRS_WRAP7 = 135,
  D3DRS_CLIPPING = 136,
  D3DRS_LIGHTING = 137,
  D3DRS_AMBIENT = 139,
  D3DRS_FOGVERTEXMODE = 140,
  D3DRS_COLORVERTEX = 141,
  D3DRS_LOCALVIEWER = 142,
  D3DRS_NORMALIZENORMALS = 143,
  D3DRS_DIFFUSEMATERIALSOURCE = 145,
  D3DRS_SPECULARMATERIALSOURCE = 146,
  D3DRS_AMBIENTMATERIALSOURCE = 147,
  D3DRS_EMISSIVEMATERIALSOURCE = 148,
  D3DRS_VERTEXBLEND = 151,
  D3DRS_CLIPPLANEENABLE = 152,
  D3DRS_SOFTWAREVERTEXPROCESSING = 153,
  D3DRS_POINTSIZE = 154,
  D3DRS_POINTSIZE_MIN = 155,
  D3DRS_POINTSPRITEENABLE = 156,
  D3DRS_POINTSCALEENABLE = 157,
  D3DRS_POINTSCALE_A = 158,
  D3DRS_POINTSCALE_B = 159,
  D3DRS_POINTSCALE_C = 160,
  D3DRS_LINEPATTERN = 10,
  D3DRS_ZVISIBLE = 30,
  D3DRS_MULTISAMPLEANTIALIAS = 161,
  D3DRS_MULTISAMPLEMASK = 162,
  D3DRS_PATCHEDGESTYLE = 163,
  D3DRS_PATCHSEGMENTS = 164,
  D3DRS_DEBUGMONITORTOKEN = 165,
  D3DRS_POINTSIZE_MAX = 166,
  D3DRS_INDEXEDVERTEXBLENDENABLE = 167,
  D3DRS_COLORWRITEENABLE = 168,
  D3DRS_TWEENFACTOR = 170,
  D3DRS_BLENDOP = 171,
  D3DRS_POSITIONORDER = 172,
  D3DRS_NORMALORDER = 173,
  D3DRS_FORCE_DWORD = 0x7fffffff
} D3DRENDERSTATETYPE;

typedef enum _D3DTEXTURESTAGESTATETYPE {
  D3DTSS_COLOROP = 1,
  D3DTSS_COLORARG1 = 2,
  D3DTSS_COLORARG2 = 3,
  D3DTSS_ALPHAOP = 4,
  D3DTSS_ALPHAARG1 = 5,
  D3DTSS_ALPHAARG2 = 6,
  D3DTSS_BUMPENVMAT00 = 7,
  D3DTSS_BUMPENVMAT01 = 8,
  D3DTSS_BUMPENVMAT10 = 9,
  D3DTSS_BUMPENVMAT11 = 10,
  D3DTSS_TEXCOORDINDEX = 11,
  D3DTSS_ADDRESSU = 13,
  D3DTSS_ADDRESSV = 14,
  D3DTSS_BORDERCOLOR = 15,
  D3DTSS_MAGFILTER = 16,
  D3DTSS_MINFILTER = 17,
  D3DTSS_MIPFILTER = 18,
  D3DTSS_MIPMAPLODBIAS = 19,
  D3DTSS_MAXMIPLEVEL = 20,
  D3DTSS_MAXANISOTROPY = 21,
  D3DTSS_BUMPENVLSCALE = 22,
  D3DTSS_BUMPENVLOFFSET = 23,
  D3DTSS_TEXTURETRANSFORMFLAGS = 24,
  D3DTSS_ADDRESSW = 25,
  D3DTSS_COLORARG0 = 26,
  D3DTSS_ALPHAARG0 = 27,
  D3DTSS_RESULTARG = 28,
} D3DTEXTURESTAGESTATETYPE;

typedef enum _D3DTRANSFORMSTATETYPE {
  D3DTS_VIEW = 2,
  D3DTS_PROJECTION = 3,
  D3DTS_TEXTURE0 = 16,
  D3DTS_TEXTURE1 = 17,
  D3DTS_TEXTURE2 = 18,
  D3DTS_TEXTURE3 = 19,
  D3DTS_TEXTURE4 = 20,
  D3DTS_TEXTURE5 = 21,
  D3DTS_TEXTURE6 = 22,
  D3DTS_TEXTURE7 = 23,
  D3DTS_WORLD = 256,
} D3DTRANSFORMSTATETYPE;

typedef enum _D3DFILLMODE {
  D3DFILL_POINT = 1,
  D3DFILL_WIREFRAME = 2,
  D3DFILL_SOLID = 3,
} D3DFILLMODE;

typedef enum _D3DSHADEMODE {
  D3DSHADE_FLAT = 1,
  D3DSHADE_GOURAUD = 2,
  D3DSHADE_PHONG = 3,
} D3DSHADEMODE;

typedef enum _D3DBLEND {
  D3DBLEND_ZERO = 1,
  D3DBLEND_ONE = 2,
  D3DBLEND_SRCCOLOR = 3,
  D3DBLEND_INVSRCCOLOR = 4,
  D3DBLEND_SRCALPHA = 5,
  D3DBLEND_INVSRCALPHA = 6,
  D3DBLEND_DESTALPHA = 7,
  D3DBLEND_INVDESTALPHA = 8,
  D3DBLEND_DESTCOLOR = 9,
  D3DBLEND_INVDESTCOLOR = 10,
  D3DBLEND_SRCALPHASAT = 11,
  D3DBLEND_BOTHSRCALPHA = 12,
  D3DBLEND_BOTHINVSRCALPHA = 13,
} D3DBLEND;

typedef enum _D3DCULL {
  D3DCULL_NONE = 1,
  D3DCULL_CW = 2,
  D3DCULL_CCW = 3,
} D3DCULL;

typedef enum _D3DCMPFUNC {
  D3DCMP_NEVER = 1,
  D3DCMP_LESS = 2,
  D3DCMP_EQUAL = 3,
  D3DCMP_LESSEQUAL = 4,
  D3DCMP_GREATER = 5,
  D3DCMP_NOTEQUAL = 6,
  D3DCMP_GREATEREQUAL = 7,
  D3DCMP_ALWAYS = 8,
  D3DCMP_FORCE_DWORD = 0x7fffffff
} D3DCMPFUNC;

typedef enum _D3DSTENCILOP {
  D3DSTENCILOP_KEEP = 1,
  D3DSTENCILOP_ZERO = 2,
  D3DSTENCILOP_REPLACE = 3,
  D3DSTENCILOP_INCRSAT = 4,
  D3DSTENCILOP_DECRSAT = 5,
  D3DSTENCILOP_INVERT = 6,
  D3DSTENCILOP_INCR = 7,
  D3DSTENCILOP_DECR = 8,
  D3DSTENCILOP_FORCE_DWORD = 0x7fffffff
} D3DSTENCILOP;

typedef enum _D3DBLENDOP {
  D3DBLENDOP_ADD = 1,
  D3DBLENDOP_SUBTRACT = 2,
  D3DBLENDOP_REVSUBTRACT = 3,
  D3DBLENDOP_MIN = 4,
  D3DBLENDOP_MAX = 5,
  D3DBLENDOP_FORCE_DWORD = 0x7fffffff
} D3DBLENDOP;

typedef enum _D3DTEXTUREOP {
  D3DTOP_DISABLE = 1,
  D3DTOP_SELECTARG1 = 2,
  D3DTOP_SELECTARG2 = 3,
  D3DTOP_MODULATE = 4,
  D3DTOP_MODULATE2X = 5,
  D3DTOP_MODULATE4X = 6,
  D3DTOP_ADD = 7,
  D3DTOP_ADDSIGNED = 8,
  D3DTOP_ADDSIGNED2X = 9,
  D3DTOP_SUBTRACT = 10,
  D3DTOP_ADDSMOOTH = 11,
  D3DTOP_BLENDDIFFUSEALPHA = 12,
  D3DTOP_BLENDTEXTUREALPHA = 13,
  D3DTOP_BLENDFACTORALPHA = 14,
  D3DTOP_BLENDTEXTUREALPHAPM = 15,
  D3DTOP_BLENDCURRENTALPHA = 16,
  D3DTOP_PREMODULATE = 17,
  D3DTOP_MODULATEALPHA_ADDCOLOR = 18,
  D3DTOP_MODULATECOLOR_ADDALPHA = 19,
  D3DTOP_MODULATEINVALPHA_ADDCOLOR = 20,
  D3DTOP_MODULATEINVCOLOR_ADDALPHA = 21,
  D3DTOP_BUMPENVMAP = 22,
  D3DTOP_BUMPENVMAPLUMINANCE = 23,
  D3DTOP_DOTPRODUCT3 = 24,
  D3DTOP_MULTIPLYADD = 25,
  D3DTOP_LERP = 26,
  D3DTOP_FORCE_DWORD = 0x7fffffff
} D3DTEXTUREOP;

typedef enum _D3DTEXTURETRANSFORMFLAGS {
  D3DTTFF_DISABLE = 0,
  D3DTTFF_COUNT1 = 1,
  D3DTTFF_COUNT2 = 2,
  D3DTTFF_COUNT3 = 3,
  D3DTTFF_COUNT4 = 4,
  D3DTTFF_PROJECTED = 256,
  D3DTTFF_FORCE_DWORD = 0x7fffffff
} D3DTEXTURETRANSFORMFLAGS;

typedef enum _D3DZBUFFERTYPE {
  D3DZB_FALSE = 0,
  D3DZB_TRUE = 1,
  D3DZB_USEW = 2,
  D3DZB_FORCE_DWORD = 0x7fffffff
} D3DZBUFFERTYPE;

typedef enum _D3DTEXTUREADDRESS {
  D3DTADDRESS_WRAP = 1,
  D3DTADDRESS_MIRROR = 2,
  D3DTADDRESS_CLAMP = 3,
  D3DTADDRESS_BORDER = 4,
  D3DTADDRESS_MIRRORONCE = 5,
  D3DTADDRESS_FORCE_DWORD = 0x7fffffff
} D3DTEXTUREADDRESS;

typedef enum _D3DTEXTUREFILTERTYPE {
  D3DTEXF_NONE = 0,
  D3DTEXF_POINT = 1,
  D3DTEXF_LINEAR = 2,
  D3DTEXF_ANISOTROPIC = 3,
  D3DTEXF_FLATCUBIC = 4,
  D3DTEXF_GAUSSIANCUBIC = 5,
  D3DTEXF_FORCE_DWORD = 0x7fffffff
} D3DTEXTUREFILTERTYPE;

typedef enum _D3DLIGHTTYPE {
  D3DLIGHT_POINT = 1,
  D3DLIGHT_SPOT = 2,
  D3DLIGHT_DIRECTIONAL = 3,
  D3DLIGHT_FORCE_DWORD = 0x7fffffff
} D3DLIGHTTYPE;

typedef enum _D3DORDER {
  D3DORDER_LINEAR = 1,
  D3DORDER_CUBIC = 2,
  D3DORDER_FORCE_DWORD = 0x7fffffff
} D3DORDER;

typedef enum _D3DVERTEXBLENDFLAGS {
  D3DVBF_DISABLE = 0,
  D3DVBF_1WEIGHTS = 1,
  D3DVBF_2WEIGHTS = 2,
  D3DVBF_3WEIGHTS = 3,
  D3DVBF_TWEENING = 255,
  D3DVBF_0WEIGHTS = 256,
  D3DVBF_FORCE_DWORD = 0x7fffffff
} D3DVERTEXBLENDFLAGS;

typedef enum _D3DPATCHEDGESTYLE {
  D3DPATCHEDGE_DISCRETE = 0,
  D3DPATCHEDGE_CONTINUOUS = 1,
  D3DPATCHEDGE_FORCE_DWORD = 0x7fffffff
} D3DPATCHEDGESTYLE;

typedef enum _D3DDEBUGMONITORTOKENS {
  D3DDMT_ENABLE = 0,
  D3DDMT_DISABLE = 1,
  D3DDMT_FORCE_DWORD = 0x7fffffff
} D3DDEBUGMONITORTOKENS;

typedef enum _D3DVSDT_TYPE {
  D3DVSDT_FLOAT1 = 0,
  D3DVSDT_FLOAT2 = 1,
  D3DVSDT_FLOAT3 = 2,
  D3DVSDT_FLOAT4 = 3,
  D3DVSDT_D3DCOLOR = 4,
  D3DVSDT_UBYTE4 = 5,
  D3DVSDT_SHORT2 = 6,
  D3DVSDT_SHORT4 = 7,
} D3DVSDT_TYPE;

// ============================================================================
// D3D8 Data Structs
// ============================================================================

typedef uint32_t D3DCOLOR;

typedef struct _D3DCOLORVALUE {
  float r;
  float g;
  float b;
  float a;
} D3DCOLORVALUE;

typedef struct _D3DMATRIX {
  union {
    struct {
      float _11, _12, _13, _14;
      float _21, _22, _23, _24;
      float _31, _32, _33, _34;
      float _41, _42, _43, _44;
    };
    float m[4][4];
  };
} D3DMATRIX;

typedef struct _D3DVECTOR {
  float x;
  float y;
  float z;
} D3DVECTOR;

typedef struct _D3DLOCKED_RECT {
  INT Pitch;
  void *pBits;
} D3DLOCKED_RECT;

typedef struct _D3DLOCKED_BOX {
  int RowPitch;
  int SlicePitch;
  void *pBits;
} D3DLOCKED_BOX;

typedef struct _D3DRECT {
  long x1;
  long y1;
  long x2;
  long y2;
} D3DRECT;

typedef struct _D3DVIEWPORT8 {
  DWORD X;
  DWORD Y;
  DWORD Width;
  DWORD Height;
  float MinZ;
  float MaxZ;
} D3DVIEWPORT8;

typedef struct _D3DMATERIAL8 {
  D3DCOLORVALUE Diffuse;
  D3DCOLORVALUE Ambient;
  D3DCOLORVALUE Specular;
  D3DCOLORVALUE Emissive;
  float Power;
} D3DMATERIAL8;

typedef struct _D3DLIGHT8 {
  D3DLIGHTTYPE Type;
  D3DCOLORVALUE Diffuse;
  D3DCOLORVALUE Ambient;
  D3DCOLORVALUE Specular;
  D3DVECTOR Position;
  D3DVECTOR Direction;
  float Range;
  float Falloff;
  float Attenuation0;
  float Attenuation1;
  float Attenuation2;
  float Theta;
  float Phi;
} D3DLIGHT8;

typedef struct _D3DGAMMARAMP {
  WORD red[256];
  WORD green[256];
  WORD blue[256];
} D3DGAMMARAMP;

typedef struct _D3DDISPLAYMODE {
  UINT Width;
  UINT Height;
  UINT RefreshRate;
  D3DFORMAT Format;
} D3DDISPLAYMODE;

// GUID stub for adapter identifier (no full COM needed)
#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID {
  unsigned long Data1;
  unsigned short Data2;
  unsigned short Data3;
  unsigned char Data4[8];
} GUID;
#endif

#ifndef _LARGE_INTEGER_DEFINED
#define _LARGE_INTEGER_DEFINED
typedef union _LARGE_INTEGER {
  struct {
    DWORD LowPart;
    LONG HighPart;
  };
  long long QuadPart;
} LARGE_INTEGER;
#endif

typedef struct _D3DADAPTER_IDENTIFIER8 {
  char Driver[512];
  char Description[512];
  LARGE_INTEGER DriverVersion;
  DWORD VendorId;
  DWORD DeviceId;
  DWORD SubSysId;
  DWORD Revision;
  GUID DeviceIdentifier;
  DWORD WHQLLevel;
} D3DADAPTER_IDENTIFIER8;

typedef struct _D3DPRESENT_PARAMETERS {
  UINT BackBufferWidth;
  UINT BackBufferHeight;
  D3DFORMAT BackBufferFormat;
  UINT BackBufferCount;
  D3DMULTISAMPLE_TYPE MultiSampleType;
  D3DSWAPEFFECT SwapEffect;
  HWND hDeviceWindow;
  BOOL Windowed;
  BOOL EnableAutoDepthStencil;
  D3DFORMAT AutoDepthStencilFormat;
  DWORD Flags;
  UINT FullScreen_RefreshRateInHz;
  UINT FullScreen_PresentationInterval;
} D3DPRESENT_PARAMETERS;

typedef struct _D3DCAPS8 {
  DWORD DeviceType;
  UINT AdapterOrdinal;
  DWORD Caps;
  DWORD Caps2;
  DWORD Caps3;
  DWORD PresentationIntervals;
  DWORD CursorCaps;
  DWORD DevCaps;
  DWORD PrimitiveMiscCaps;
  DWORD RasterCaps;
  DWORD ZCmpCaps;
  DWORD SrcBlendCaps;
  DWORD DestBlendCaps;
  DWORD AlphaCmpCaps;
  DWORD ShadeCaps;
  DWORD TextureCaps;
  DWORD TextureFilterCaps;
  DWORD CubeTextureFilterCaps;
  DWORD VolumeTextureFilterCaps;
  DWORD TextureAddressCaps;
  DWORD VolumeTextureAddressCaps;
  DWORD LineCaps;
  DWORD MaxTextureWidth, MaxTextureHeight;
  DWORD MaxVolumeExtent;
  DWORD MaxTextureRepeat;
  DWORD MaxTextureAspectRatio;
  DWORD MaxAnisotropy;
  float MaxVertexW;
  float GuardBandLeft;
  float GuardBandTop;
  float GuardBandRight;
  float GuardBandBottom;
  float ExtentsAdjust;
  DWORD StencilCaps;
  DWORD FVFCaps;
  DWORD TextureOpCaps;
  DWORD MaxTextureBlendStages;
  DWORD MaxSimultaneousTextures;
  DWORD VertexProcessingCaps;
  DWORD MaxActiveLights;
  DWORD MaxUserClipPlanes;
  DWORD MaxVertexBlendMatrices;
  DWORD MaxVertexBlendMatrixIndex;
  float MaxPointSize;
  DWORD MaxPrimitiveCount;
  DWORD MaxVertexIndex;
  DWORD MaxStreams;
  DWORD MaxStreamStride;
  DWORD VertexShaderVersion;
  DWORD MaxVertexShaderConst;
  DWORD PixelShaderVersion;
  float MaxPixelShaderValue;
} D3DCAPS8;

typedef struct _D3DSURFACE_DESC {
  D3DFORMAT Format;
  D3DRESOURCETYPE Type;
  DWORD Usage;
  D3DPOOL Pool;
  UINT Size;
  D3DMULTISAMPLE_TYPE MultiSampleType;
  UINT Width;
  UINT Height;
} D3DSURFACE_DESC;

typedef struct _D3DVOLUME_DESC {
  D3DFORMAT Format;
  D3DRESOURCETYPE Type;
  DWORD Usage;
  D3DPOOL Pool;
  UINT Width;
  UINT Height;
  UINT Depth;
} D3DVOLUME_DESC;

typedef struct _D3DINDEXBUFFER_DESC {
  D3DFORMAT Format;
  D3DRESOURCETYPE Type;
  DWORD Usage;
  D3DPOOL Pool;
  UINT Size;
} D3DINDEXBUFFER_DESC;

typedef struct _D3DVERTEXBUFFER_DESC {
  D3DFORMAT Format;
  D3DRESOURCETYPE Type;
  DWORD Usage;
  D3DPOOL Pool;
  UINT Size;
  DWORD FVF;
} D3DVERTEXBUFFER_DESC;

// ============================================================================
// COM Interfaces — abstract base classes for DX8 type compatibility.
// No dependency on objbase.h/windows.h. Uses minimal virtual interfaces.
// ============================================================================

// Minimal RECT for surface lock methods (guarded for win32types_compat.h)
#ifndef _RECT_DEFINED
#define _RECT_DEFINED
typedef struct tagRECT {
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
} RECT;
#endif

// Forward declarations
struct IDirect3D8;
struct IDirect3DDevice8;
struct IDirect3DResource8;
struct IDirect3DBaseTexture8;
struct IDirect3DTexture8;
struct IDirect3DCubeTexture8;
struct IDirect3DVolumeTexture8;
struct IDirect3DSurface8;
struct IDirect3DVolume8;
struct IDirect3DVertexBuffer8;
struct IDirect3DIndexBuffer8;
struct IDirect3DSwapChain8;

struct IDirect3DResource8 {
  virtual ~IDirect3DResource8() = default;
  virtual D3DRESOURCETYPE GetType() = 0;
};

struct IDirect3DVertexBuffer8 : public IDirect3DResource8 {
  virtual HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, BYTE **ppbData, DWORD Flags) = 0;
  virtual HRESULT Unlock() = 0;
  virtual HRESULT GetDesc(D3DVERTEXBUFFER_DESC *pDesc) = 0;
};

struct IDirect3DIndexBuffer8 : public IDirect3DResource8 {
  virtual HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, BYTE **ppbData, DWORD Flags) = 0;
  virtual HRESULT Unlock() = 0;
  virtual HRESULT GetDesc(D3DINDEXBUFFER_DESC *pDesc) = 0;
};

struct IDirect3DSurface8 {
  virtual ~IDirect3DSurface8() = default;
  virtual HRESULT GetDesc(D3DSURFACE_DESC *pDesc) = 0;
  virtual HRESULT LockRect(D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) = 0;
  virtual HRESULT UnlockRect() = 0;
};

struct IDirect3DBaseTexture8 : public IDirect3DResource8 {
  virtual DWORD SetLOD(DWORD LODNew) = 0;
  virtual DWORD GetLOD() = 0;
  virtual DWORD GetLevelCount() = 0;
};

struct IDirect3DTexture8 : public IDirect3DBaseTexture8 {
  virtual HRESULT GetLevelDesc(UINT Level, D3DSURFACE_DESC *pDesc) = 0;
  virtual HRESULT GetSurfaceLevel(UINT Level, IDirect3DSurface8 **ppSurfaceLevel) = 0;
  virtual HRESULT LockRect(UINT Level, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) = 0;
  virtual HRESULT UnlockRect(UINT Level) = 0;
  virtual HRESULT AddDirtyRect(const RECT *pDirtyRect) = 0;
};

struct IDirect3DVolumeTexture8 : public IDirect3DBaseTexture8 {
  virtual HRESULT GetLevelDesc(UINT Level, D3DVOLUME_DESC *pDesc) = 0;
  virtual HRESULT LockBox(UINT Level, D3DLOCKED_BOX *pLockedVolume, const void *pBox, DWORD Flags) = 0;
  virtual HRESULT UnlockBox(UINT Level) = 0;
};

struct IDirect3DCubeTexture8 : public IDirect3DBaseTexture8 {
  virtual HRESULT GetLevelDesc(UINT Level, D3DSURFACE_DESC *pDesc) = 0;
  virtual HRESULT LockRect(D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) = 0;
  virtual HRESULT UnlockRect(D3DCUBEMAP_FACES FaceType, UINT Level) = 0;
};

struct IDirect3DVolume8 {
  virtual ~IDirect3DVolume8() = default;
};

struct IDirect3DSwapChain8 {
  virtual ~IDirect3DSwapChain8() = default;
  virtual HRESULT Present(const void *s, const void *d, HWND w, const void *r) = 0;
  virtual HRESULT GetBackBuffer(UINT i, D3DBACKBUFFER_TYPE t, IDirect3DSurface8 **b) = 0;
};

struct IDirect3DDevice8 {
  virtual ~IDirect3DDevice8() = default;

  virtual HRESULT TestCooperativeLevel() = 0;
  virtual HRESULT SetVertexShader(DWORD v) = 0;
  virtual HRESULT DeleteVertexShader(DWORD v) = 0;
  virtual HRESULT SetPixelShader(DWORD v) = 0;
  virtual HRESULT DeletePixelShader(DWORD v) = 0;
  virtual HRESULT CreatePixelShader(const DWORD *pFunction, DWORD *pHandle) = 0;
  virtual HRESULT SetVertexShaderConstant(DWORD r, const void *d, DWORD c) = 0;
  virtual HRESULT SetPixelShaderConstant(DWORD r, const void *d, DWORD c) = 0;
  virtual HRESULT SetTransform(D3DTRANSFORMSTATETYPE t, const D3DMATRIX *m) = 0;
  virtual HRESULT GetTransform(D3DTRANSFORMSTATETYPE t, D3DMATRIX *m) = 0;
  virtual HRESULT LightEnable(DWORD i, BOOL b) = 0;
  virtual HRESULT SetTexture(DWORD s, IDirect3DBaseTexture8 *t) = 0;
  virtual HRESULT SetRenderState(D3DRENDERSTATETYPE s, DWORD v) = 0;
  virtual HRESULT GetRenderState(D3DRENDERSTATETYPE s, DWORD *v) = 0;
  virtual HRESULT SetTextureStageState(DWORD s, D3DTEXTURESTAGESTATETYPE t, DWORD v) = 0;
  virtual HRESULT GetTextureStageState(DWORD s, D3DTEXTURESTAGESTATETYPE t, DWORD *v) = 0;
  virtual HRESULT SetLight(DWORD i, const D3DLIGHT8 *l) = 0;
  virtual HRESULT SetViewport(const D3DVIEWPORT8 *v) = 0;
  virtual HRESULT Clear(DWORD c, const void *r, DWORD f, D3DCOLOR cl, float z, DWORD s) = 0;
  virtual HRESULT BeginScene() = 0;
  virtual HRESULT EndScene() = 0;
  virtual HRESULT Present(const void *s, const void *d, HWND w, const void *r) = 0;
  virtual HRESULT GetBackBuffer(UINT i, D3DBACKBUFFER_TYPE t, IDirect3DSurface8 **b) = 0;
  virtual HRESULT GetFrontBuffer(IDirect3DSurface8 *d) = 0;
  virtual HRESULT UpdateTexture(IDirect3DBaseTexture8 *s, IDirect3DBaseTexture8 *d) = 0;
  virtual HRESULT SetIndices(IDirect3DIndexBuffer8 *i, UINT b) = 0;
  virtual HRESULT DrawIndexedPrimitive(DWORD t, UINT m, UINT v, UINT s, UINT p) = 0;
  virtual HRESULT SetStreamSource(UINT s, IDirect3DVertexBuffer8 *v, UINT d) = 0;
  virtual HRESULT DrawPrimitive(DWORD t, UINT s, UINT p) = 0;
  virtual HRESULT CreateTexture(UINT w, UINT h, UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DTexture8 **t) = 0;
  virtual HRESULT CreateVolumeTexture(UINT w, UINT h, UINT d, UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DVolumeTexture8 **t) = 0;
  virtual HRESULT CreateImageSurface(UINT w, UINT h, D3DFORMAT f, IDirect3DSurface8 **s) = 0;
  virtual HRESULT CreateCubeTexture(UINT s, UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DCubeTexture8 **t) = 0;
  virtual HRESULT CreateVertexBuffer(UINT l, DWORD u, DWORD f, D3DPOOL p, IDirect3DVertexBuffer8 **v) = 0;
  virtual HRESULT CreateIndexBuffer(UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DIndexBuffer8 **i) = 0;
  virtual HRESULT GetRenderTarget(IDirect3DSurface8 **s) = 0;
  virtual HRESULT SetRenderTarget(IDirect3DSurface8 *s, IDirect3DSurface8 *d) = 0;
  virtual HRESULT GetDepthStencilSurface(IDirect3DSurface8 **s) = 0;
  virtual HRESULT SetDepthStencilSurface(IDirect3DSurface8 *s) = 0;
  virtual HRESULT CopyRects(IDirect3DSurface8 *s, const void *r, UINT c, IDirect3DSurface8 *d, const void *p) = 0;
  virtual HRESULT Reset(D3DPRESENT_PARAMETERS *p) = 0;
  virtual HRESULT GetDeviceCaps(D3DCAPS8 *c) = 0;
  virtual HRESULT GetAdapterIdentifier(UINT a, DWORD f, D3DADAPTER_IDENTIFIER8 *i) = 0;
  virtual HRESULT SetMaterial(const D3DMATERIAL8 *m) = 0;
  virtual HRESULT SetClipPlane(DWORD i, const float *p) = 0;
  virtual HRESULT ResourceManagerDiscardBytes(DWORD Bytes) = 0;
  virtual HRESULT ValidateDevice(DWORD *pPasses) = 0;
  virtual HRESULT GetDisplayMode(D3DDISPLAYMODE *pMode) = 0;
  virtual HRESULT CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS *pModel, IDirect3DSwapChain8 **pSwapChain) = 0;
  virtual UINT GetAvailableTextureMem() = 0;
  virtual HRESULT DrawPrimitiveUP(DWORD PrimitiveType, UINT PrimitiveCount, const void *pVertexStreamZeroData, UINT VertexStreamZeroStride) = 0;
  virtual HRESULT DrawIndexedPrimitiveUP(DWORD PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, const void *pIndexData, D3DFORMAT IndexDataFormat, const void *pVertexStreamZeroData, UINT VertexStreamZeroStride) = 0;
  virtual HRESULT CreateVertexShader(const DWORD *pDeclaration, const DWORD *pFunction, DWORD *pHandle, DWORD Flags) = 0;
  virtual HRESULT SetGammaRamp(DWORD Flags, const D3DGAMMARAMP *pRamp) = 0;
  virtual HRESULT GetGammaRamp(D3DGAMMARAMP *pRamp) = 0;
  virtual BOOL ShowCursor(BOOL bShow) = 0;
  virtual HRESULT SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8 *pCursorBitmap) = 0;
  virtual void SetCursorPosition(int X, int Y, DWORD Flags) = 0;
};

struct IDirect3D8 {
  virtual ~IDirect3D8() = default;

  virtual HRESULT RegisterSoftwareDevice(void *pInitializeFunction) = 0;
  virtual UINT GetAdapterCount() = 0;
  virtual HRESULT GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER8 *pIdentifier) = 0;
  virtual UINT GetAdapterModeCount(UINT Adapter) = 0;
  virtual HRESULT EnumAdapterModes(UINT Adapter, UINT Mode, D3DDISPLAYMODE *pMode) = 0;
  virtual HRESULT GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE *pMode) = 0;
  virtual HRESULT CheckDeviceType(UINT Adapter, DWORD CheckType, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed) = 0;
  virtual HRESULT CheckDeviceFormat(UINT Adapter, DWORD DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, DWORD RType, D3DFORMAT CheckFormat) = 0;
  virtual HRESULT CheckDeviceMultiSampleType(UINT Adapter, DWORD DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, DWORD MultiSampleType) = 0;
  virtual HRESULT CheckDepthStencilMatch(UINT Adapter, DWORD DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) = 0;
  virtual HRESULT GetDeviceCaps(UINT Adapter, DWORD DeviceType, D3DCAPS8 *pCaps) = 0;
  virtual HMONITOR GetAdapterMonitor(UINT Adapter) = 0;
  virtual HRESULT CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DDevice8 **ppReturnedDeviceInterface) = 0;
};

// ── D3DX buffer interface ──────────────────────────────────────────────

struct ID3DXBuffer {
  virtual ~ID3DXBuffer() = default;
  virtual void *GetBufferPointer() = 0;
  virtual DWORD GetBufferSize() = 0;
};
typedef struct ID3DXBuffer *LPD3DXBUFFER;

// ── Pointer typedefs ───────────────────────────────────────────────────

typedef struct IDirect3D8 *LPDIRECT3D8;
typedef struct IDirect3DDevice8 *LPDIRECT3DDEVICE8;
typedef struct IDirect3DResource8 *LPDIRECT3DRESOURCE8;
typedef struct IDirect3DBaseTexture8 *LPDIRECT3DBASETEXTURE8;
typedef struct IDirect3DTexture8 *LPDIRECT3DTEXTURE8;
typedef struct IDirect3DCubeTexture8 *LPDIRECT3DCUBETEXTURE8;
typedef struct IDirect3DVolumeTexture8 *LPDIRECT3DVOLUMETEXTURE8;
typedef struct IDirect3DSurface8 *LPDIRECT3DSURFACE8;
typedef struct IDirect3DVolume8 *LPDIRECT3DVOLUME8;
typedef struct IDirect3DVertexBuffer8 *LPDIRECT3DVERTEXBUFFER8;
typedef struct IDirect3DIndexBuffer8 *LPDIRECT3DINDEXBUFFER8;
typedef struct IDirect3DSwapChain8 *LPDIRECT3DSWAPCHAIN8;

#endif // __APPLE__
