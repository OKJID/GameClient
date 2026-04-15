/**
 * MetalBridgeMappings.h — DX8 → Metal state mapping functions
 *
 * Extracted from MetalDevice8.mm for testability.
 * These functions map D3D8 enums to Metal enums.
 * When METAL_BRIDGE_TEST_MODE is defined, Metal types are replaced with
 * integer constants so the tests can compile without Metal framework.
 */
#pragma once

#include <d3d8.h>
#include <cstdint>

// ─────────────────────────────────────────────────────────────────
// When compiling for tests, we don't have Metal headers available.
// Define the Metal enum values as plain integers so our mapping
// functions return comparable numeric values.
// ─────────────────────────────────────────────────────────────────
#ifdef METAL_BRIDGE_TEST_MODE

// MTLBlendFactor values (from Metal API)
enum {
  MTLBlendFactorZero = 0,
  MTLBlendFactorOne = 1,
  MTLBlendFactorSourceColor = 2,
  MTLBlendFactorOneMinusSourceColor = 3,
  MTLBlendFactorSourceAlpha = 4,
  MTLBlendFactorOneMinusSourceAlpha = 5,
  MTLBlendFactorDestinationColor = 6,
  MTLBlendFactorOneMinusDestinationColor = 7,
  MTLBlendFactorDestinationAlpha = 8,
  MTLBlendFactorOneMinusDestinationAlpha = 9,
  MTLBlendFactorSourceAlphaSaturated = 10,
  MTLBlendFactorBlendColor = 11,
  MTLBlendFactorOneMinusBlendColor = 12,
  MTLBlendFactorBlendAlpha = 13,
  MTLBlendFactorOneMinusBlendAlpha = 14,
};
typedef uint32_t MTLBlendFactor;

// MTLCullMode values
enum {
  MTLCullModeNone = 0,
  MTLCullModeFront = 1,
  MTLCullModeBack = 2,
};
typedef uint32_t MTLCullMode;

// MTLCompareFunction values
enum {
  MTLCompareFunctionNever = 0,
  MTLCompareFunctionLess = 1,
  MTLCompareFunctionEqual = 2,
  MTLCompareFunctionLessEqual = 3,
  MTLCompareFunctionGreater = 4,
  MTLCompareFunctionNotEqual = 5,
  MTLCompareFunctionGreaterEqual = 6,
  MTLCompareFunctionAlways = 7,
};
typedef uint32_t MTLCompareFunction;

// MTLSamplerAddressMode values
enum {
  MTLSamplerAddressModeClampToEdge = 0,
  MTLSamplerAddressModeMirrorClampToEdge = 1,
  MTLSamplerAddressModeRepeat = 2,
  MTLSamplerAddressModeMirrorRepeat = 3,
  MTLSamplerAddressModeClampToZero = 4,
  MTLSamplerAddressModeClampToBorderColor = 5,
};
typedef uint32_t MTLSamplerAddressMode;

// MTLSamplerMinMagFilter values
enum {
  MTLSamplerMinMagFilterNearest = 0,
  MTLSamplerMinMagFilterLinear = 1,
};
typedef uint32_t MTLSamplerMinMagFilter;

// MTLSamplerMipFilter values
enum {
  MTLSamplerMipFilterNotMipmapped = 0,
  MTLSamplerMipFilterNearest = 1,
  MTLSamplerMipFilterLinear = 2,
};
typedef uint32_t MTLSamplerMipFilter;

// MTLStencilOperation values
enum {
  MTLStencilOperationKeep = 0,
  MTLStencilOperationZero = 1,
  MTLStencilOperationReplace = 2,
  MTLStencilOperationIncrementClamp = 3,
  MTLStencilOperationDecrementClamp = 4,
  MTLStencilOperationInvert = 5,
  MTLStencilOperationIncrementWrap = 6,
  MTLStencilOperationDecrementWrap = 7,
};
typedef uint32_t MTLStencilOperation;

// MTLColorWriteMask values  
enum {
  MTLColorWriteMaskNone  = 0,
  MTLColorWriteMaskRed   = 0x8,
  MTLColorWriteMaskGreen = 0x4,
  MTLColorWriteMaskBlue  = 0x2,
  MTLColorWriteMaskAlpha = 0x1,
  MTLColorWriteMaskAll   = 0xF,
};
typedef uint32_t MTLColorWriteMask;

// MTLPixelFormat values (subset used in tests)
enum {
  MTLPixelFormatBGRA8Unorm = 80,
  MTLPixelFormatRGBA8Unorm = 70,
  MTLPixelFormatR8Unorm = 10,
  MTLPixelFormatA8Unorm = 1,
  MTLPixelFormatRG8Unorm = 30,
  MTLPixelFormatRG8Snorm = 32,
  MTLPixelFormatBC1_RGBA = 130,
  MTLPixelFormatBC2_RGBA = 132,
  MTLPixelFormatBC3_RGBA = 134,
};
typedef uint32_t MTLPixelFormat;

// MTLVertexFormat values (subset used in tests)
enum {
  MTLVertexFormatFloat2 = 29,
  MTLVertexFormatFloat3 = 30,
  MTLVertexFormatFloat4 = 31,
  MTLVertexFormatUChar4Normalized_BGRA = 42,
};
typedef uint32_t MTLVertexFormat;

#else
// In production code, include the real Metal header
#include <Metal/Metal.h>
#endif // METAL_BRIDGE_TEST_MODE


// ═══════════════════════════════════════════════════════════════
//  D3DBLEND → MTLBlendFactor
// ═══════════════════════════════════════════════════════════════
inline MTLBlendFactor MapD3DBlendToMTL(DWORD blend) {
  switch (blend) {
  case D3DBLEND_ZERO:
    return MTLBlendFactorZero;
  case D3DBLEND_ONE:
    return MTLBlendFactorOne;
  case D3DBLEND_SRCCOLOR:
    return MTLBlendFactorSourceColor;
  case D3DBLEND_INVSRCCOLOR:
    return MTLBlendFactorOneMinusSourceColor;
  case D3DBLEND_SRCALPHA:
    return MTLBlendFactorSourceAlpha;
  case D3DBLEND_INVSRCALPHA:
    return MTLBlendFactorOneMinusSourceAlpha;
  case D3DBLEND_DESTALPHA:
    return MTLBlendFactorDestinationAlpha;
  case D3DBLEND_INVDESTALPHA:
    return MTLBlendFactorOneMinusDestinationAlpha;
  case D3DBLEND_DESTCOLOR:
    return MTLBlendFactorDestinationColor;
  case D3DBLEND_INVDESTCOLOR:
    return MTLBlendFactorOneMinusDestinationColor;
  case D3DBLEND_SRCALPHASAT:
    return MTLBlendFactorSourceAlphaSaturated;
  default:
    return MTLBlendFactorOne;
  }
}

// ═══════════════════════════════════════════════════════════════
//  D3DCULL → MTLCullMode
//  DX8 uses CW/CCW winding opposite to Metal
// ═══════════════════════════════════════════════════════════════
inline MTLCullMode MapD3DCullToMTL(DWORD cull) {
  switch (cull) {
  case D3DCULL_NONE:
    return MTLCullModeNone;
  case D3DCULL_CW:
    return MTLCullModeFront; // DX8 CW = Metal Front
  case D3DCULL_CCW:
    return MTLCullModeBack;
  default:
    return MTLCullModeBack; // DX8 default is CCW
  }
}

// ═══════════════════════════════════════════════════════════════
//  D3DCMP → MTLCompareFunction
// ═══════════════════════════════════════════════════════════════
inline MTLCompareFunction MapD3DCmpToMTL(DWORD d3dCmp) {
  switch (d3dCmp) {
  case D3DCMP_NEVER:
    return MTLCompareFunctionNever;
  case D3DCMP_LESS:
    return MTLCompareFunctionLess;
  case D3DCMP_EQUAL:
    return MTLCompareFunctionEqual;
  case D3DCMP_LESSEQUAL:
    return MTLCompareFunctionLessEqual;
  case D3DCMP_GREATER:
    return MTLCompareFunctionGreater;
  case D3DCMP_NOTEQUAL:
    return MTLCompareFunctionNotEqual;
  case D3DCMP_GREATEREQUAL:
    return MTLCompareFunctionGreaterEqual;
  case D3DCMP_ALWAYS:
    return MTLCompareFunctionAlways;
  default:
    return MTLCompareFunctionLessEqual; // DX8 default
  }
}

// ═══════════════════════════════════════════════════════════════
//  D3DSTENCILOP → MTLStencilOperation
// ═══════════════════════════════════════════════════════════════
inline MTLStencilOperation MapD3DStencilOpToMTL(DWORD op) {
  switch (op) {
  case D3DSTENCILOP_KEEP:
    return MTLStencilOperationKeep;
  case D3DSTENCILOP_ZERO:
    return MTLStencilOperationZero;
  case D3DSTENCILOP_REPLACE:
    return MTLStencilOperationReplace;
  case D3DSTENCILOP_INCRSAT:
    return MTLStencilOperationIncrementClamp;
  case D3DSTENCILOP_DECRSAT:
    return MTLStencilOperationDecrementClamp;
  case D3DSTENCILOP_INVERT:
    return MTLStencilOperationInvert;
  case D3DSTENCILOP_INCR:
    return MTLStencilOperationIncrementWrap;
  case D3DSTENCILOP_DECR:
    return MTLStencilOperationDecrementWrap;
  default:
    return MTLStencilOperationKeep;
  }
}

// ═══════════════════════════════════════════════════════════════
//  D3DTEXTUREADDRESS → MTLSamplerAddressMode
// ═══════════════════════════════════════════════════════════════
inline MTLSamplerAddressMode MapD3DAddressToMTL(DWORD addr) {
  switch (addr) {
  case D3DTADDRESS_WRAP:
    return MTLSamplerAddressModeRepeat;
  case D3DTADDRESS_CLAMP:
    return MTLSamplerAddressModeClampToEdge;
  case D3DTADDRESS_MIRROR:
    return MTLSamplerAddressModeMirrorRepeat;
  case D3DTADDRESS_BORDER:
    return MTLSamplerAddressModeClampToZero;
  default:
    return MTLSamplerAddressModeRepeat;
  }
}

// ═══════════════════════════════════════════════════════════════
//  D3DTEXF → MTLSamplerMinMagFilter
// ═══════════════════════════════════════════════════════════════
inline MTLSamplerMinMagFilter MapD3DFilterToMTL(DWORD filter) {
  switch (filter) {
  case D3DTEXF_POINT:
    return MTLSamplerMinMagFilterNearest;
  case D3DTEXF_LINEAR:
  case D3DTEXF_ANISOTROPIC:
  case D3DTEXF_FLATCUBIC:
  case D3DTEXF_GAUSSIANCUBIC:
    return MTLSamplerMinMagFilterLinear;
  default:
    return MTLSamplerMinMagFilterLinear;
  }
}

// ═══════════════════════════════════════════════════════════════
//  D3DTEXF → MTLSamplerMipFilter
// ═══════════════════════════════════════════════════════════════
inline MTLSamplerMipFilter MapD3DMipFilterToMTL(DWORD filter) {
  switch (filter) {
  case D3DTEXF_NONE:
    return MTLSamplerMipFilterNotMipmapped;
  case D3DTEXF_POINT:
    return MTLSamplerMipFilterNearest;
  case D3DTEXF_LINEAR:
    return MTLSamplerMipFilterLinear;
  default:
    return MTLSamplerMipFilterNotMipmapped;
  }
}

// ═══════════════════════════════════════════════════════════════
//  D3DFORMAT → MTLPixelFormat
// ═══════════════════════════════════════════════════════════════
inline MTLPixelFormat MetalFormatFromD3D(D3DFORMAT fmt) {
  switch (fmt) {
  case D3DFMT_A8R8G8B8:
  case D3DFMT_X8R8G8B8:
  case D3DFMT_R8G8B8:     // 24-bit → promoted to 32-bit BGRA in UnlockRect
    return MTLPixelFormatBGRA8Unorm;

  // 16-bit formats → BGRA8Unorm (CPU conversion in UnlockRect)
  case D3DFMT_R5G6B5:
  case D3DFMT_X1R5G5B5:
  case D3DFMT_A1R5G5B5:
  case D3DFMT_A4R4G4B4:
    return MTLPixelFormatBGRA8Unorm;

  case D3DFMT_V8U8:
  case D3DFMT_L6V5U5:
    return MTLPixelFormatRG8Snorm;

  case D3DFMT_L8:
  case D3DFMT_P8:
    return MTLPixelFormatR8Unorm;

  case D3DFMT_A8:
    return MTLPixelFormatA8Unorm;

  case D3DFMT_A8L8:
  case D3DFMT_A8P8:
    return MTLPixelFormatRG8Unorm;

  case D3DFMT_A4L4:
    return MTLPixelFormatRG8Unorm; // CPU conversion: 4+4 bits → 8+8 bits

  // macOS Metal supports BC compression
  case D3DFMT_DXT1:
    return MTLPixelFormatBC1_RGBA;
  case D3DFMT_DXT2: // premultiplied alpha DXT3
  case D3DFMT_DXT3:
    return MTLPixelFormatBC2_RGBA;
  case D3DFMT_DXT4: // premultiplied alpha DXT5
  case D3DFMT_DXT5:
    return MTLPixelFormatBC3_RGBA;

  default:
    return MTLPixelFormatBGRA8Unorm;
  }
}

// ═══════════════════════════════════════════════════════════════
//  D3DRS_COLORWRITEENABLE → MTLColorWriteMask
// ═══════════════════════════════════════════════════════════════
inline MTLColorWriteMask MapD3DColorWriteToMTL(DWORD cwMask) {
  if (cwMask == 0) cwMask = 0xF; // default: write all
  MTLColorWriteMask mtlMask = MTLColorWriteMaskNone;
  if (cwMask & 1) mtlMask |= MTLColorWriteMaskRed;
  if (cwMask & 2) mtlMask |= MTLColorWriteMaskGreen;
  if (cwMask & 4) mtlMask |= MTLColorWriteMaskBlue;
  if (cwMask & 8) mtlMask |= MTLColorWriteMaskAlpha;
  return mtlMask;
}

// ═══════════════════════════════════════════════════════════════
//  FVF Vertex Layout Calculator (CPU-only, no Metal objects)
//  Returns per-attribute info without creating MTLVertexDescriptor
// ═══════════════════════════════════════════════════════════════

struct FVFAttributeInfo {
  uint32_t format;    // MTLVertexFormat enum value
  uint32_t offset;    // byte offset within vertex
  uint32_t bufIndex;  // buffer index (0=vertex, 30=defaults)
  bool present;       // true if FVF provides this attribute
};

struct FVFLayoutResult {
  FVFAttributeInfo position;     // attr[0]
  FVFAttributeInfo diffuse;      // attr[1]
  FVFAttributeInfo texCoord0;    // attr[2]
  FVFAttributeInfo normal;       // attr[3]
  FVFAttributeInfo specular;     // attr[4]
  FVFAttributeInfo texCoord1;    // attr[5]
  uint32_t computedStride;       // tightly-packed stride (sum of attrs)
};

inline FVFLayoutResult ComputeFVFLayout(DWORD fvf) {
  FVFLayoutResult r = {};
  uint32_t offset = 0;

  // Position
  DWORD posType = fvf & 0x400E; // D3DFVF_POSITION_MASK
  if (posType == D3DFVF_XYZRHW) {
    r.position = {MTLVertexFormatFloat4, offset, 0, true};
    offset += 16;
  } else if (posType != 0) {
    r.position = {MTLVertexFormatFloat3, offset, 0, true};
    offset += 12; // base XYZ
    
    // Add size of blend weights (padding)
    if (posType == D3DFVF_XYZB1) offset += 4;
    else if (posType == D3DFVF_XYZB2) offset += 8;
    else if (posType == D3DFVF_XYZB3) offset += 12;
    else if (posType == D3DFVF_XYZB4) offset += 16;
    else if (posType == D3DFVF_XYZB5) offset += 20;
  }

  // Normal (must come after position in DX8 FVF order)
  if (fvf & D3DFVF_NORMAL) {
    r.normal = {MTLVertexFormatFloat3, offset, 0, true};
    offset += 12;
  }

  // Diffuse
  if (fvf & D3DFVF_DIFFUSE) {
    r.diffuse = {MTLVertexFormatUChar4Normalized_BGRA, offset, 0, true};
    offset += 4;
  }

  // Specular
  if (fvf & 0x080) { // D3DFVF_SPECULAR
    r.specular = {MTLVertexFormatUChar4Normalized_BGRA, offset, 0, true};
    offset += 4;
  }

  // Texture coords
  UINT texCount = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
  if (texCount >= 1) {
    r.texCoord0 = {MTLVertexFormatFloat2, offset, 0, true};
    offset += 8;
  }
  if (texCount >= 2) {
    r.texCoord1 = {MTLVertexFormatFloat2, offset, 0, true};
    offset += 8;
  }

  r.computedStride = offset;

  // Defaults for missing attributes (buffer 30)
  if (!r.position.present) {
    r.position = {MTLVertexFormatFloat3, 8, 30, false};
  }
  if (!r.diffuse.present) {
    r.diffuse = {MTLVertexFormatUChar4Normalized_BGRA, 0, 30, false};
  }
  if (!r.texCoord0.present) {
    r.texCoord0 = {MTLVertexFormatFloat2, 8, 30, false};
  }
  if (!r.normal.present) {
    r.normal = {MTLVertexFormatFloat3, 8, 30, false};
  }
  if (!r.specular.present) {
    r.specular = {MTLVertexFormatUChar4Normalized_BGRA, 4, 30, false};
  }
  if (!r.texCoord1.present) {
    r.texCoord1 = {MTLVertexFormatFloat2, 8, 30, false};
  }

  return r;
}
