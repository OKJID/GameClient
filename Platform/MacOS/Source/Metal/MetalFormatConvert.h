/**
 * MetalFormatConvert.h — Pure CPU format conversion functions
 *
 * Extracted from MetalTexture8.mm for testability.
 * These functions have NO Metal/GPU dependencies — they are pure data converters.
 */
#pragma once

#include <d3d8.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>

// ─────────────────────────────────────────────────────────────────
// Helper: Get Bytes Per Pixel or Block Size for a D3D format
// ─────────────────────────────────────────────────────────────────
inline UINT BytesPerPixelFromD3D(D3DFORMAT fmt) {
  switch (fmt) {
  case D3DFMT_A8R8G8B8:
  case D3DFMT_X8R8G8B8:
    return 4;
  case D3DFMT_R5G6B5:
  case D3DFMT_X1R5G5B5:
  case D3DFMT_A1R5G5B5:
  case D3DFMT_A4R4G4B4:
  case D3DFMT_V8U8:
  case D3DFMT_L6V5U5:
  case D3DFMT_A8L8:
  case D3DFMT_A8P8:
    return 2;
  case D3DFMT_R8G8B8:
    return 3;
  case D3DFMT_A8:
  case D3DFMT_L8:
  case D3DFMT_P8:
  case D3DFMT_A4L4:
    return 1;
  case D3DFMT_DXT1:
    return 8; // Per 4x4 block (8 bytes)
  case D3DFMT_DXT2:
  case D3DFMT_DXT3:
  case D3DFMT_DXT4:
  case D3DFMT_DXT5:
    return 16; // Per 4x4 block (16 bytes)
  default:
    return 4; // Fallback
  }
}

// ─────────────────────────────────────────────────────────────────
// Check if format is a 16-bit format requiring conversion
// ─────────────────────────────────────────────────────────────────
inline bool Is16BitFormat(D3DFORMAT fmt) {
  return fmt == D3DFMT_R5G6B5 || fmt == D3DFMT_X1R5G5B5 ||
         fmt == D3DFMT_A1R5G5B5 || fmt == D3DFMT_A4R4G4B4;
}

// ─────────────────────────────────────────────────────────────────
// Get texFormatType for a D3D format
// 0 = Default (standard BGRA)
// 1 = Luminance: RGB = r, A = 1.0 (from R8Unorm)
// 2 = Luminance+Alpha: RGB = r, A = g (from RG8Unorm)
// ─────────────────────────────────────────────────────────────────
inline uint32_t GetTexFormatType(D3DFORMAT fmt) {
  switch (fmt) {
  case D3DFMT_L8:
  case D3DFMT_P8:
    return 1;
  case D3DFMT_A8L8:
  case D3DFMT_A4L4:
  case D3DFMT_A8P8:
    return 2;
  default:
    return 0;
  }
}

// ─────────────────────────────────────────────────────────────────
// Convert a single 16-bit pixel to BGRA8 (32-bit)
// Returns the pixel as a uint32_t in BGRA byte order:
//   bits [7:0]=B, [15:8]=G, [23:16]=R, [31:24]=A
// ─────────────────────────────────────────────────────────────────
inline uint32_t ConvertPixel16to32(D3DFORMAT fmt, uint16_t px) {
  uint8_t B, G, R, A;

  switch (fmt) {
  case D3DFMT_R5G6B5:
    // RRRR RGGG GGGB BBBB
    B = (uint8_t)(((px      ) & 0x1F) * 255 / 31);
    G = (uint8_t)(((px >>  5) & 0x3F) * 255 / 63);
    R = (uint8_t)(((px >> 11) & 0x1F) * 255 / 31);
    A = 255;
    break;
  case D3DFMT_X1R5G5B5:
    // xRRR RRGG GGGB BBBB
    B = (uint8_t)(((px      ) & 0x1F) * 255 / 31);
    G = (uint8_t)(((px >>  5) & 0x1F) * 255 / 31);
    R = (uint8_t)(((px >> 10) & 0x1F) * 255 / 31);
    A = 255;
    break;
  case D3DFMT_A1R5G5B5:
    // ARRR RRGG GGGB BBBB
    B = (uint8_t)(((px      ) & 0x1F) * 255 / 31);
    G = (uint8_t)(((px >>  5) & 0x1F) * 255 / 31);
    R = (uint8_t)(((px >> 10) & 0x1F) * 255 / 31);
    A = (px >> 15) ? 255 : 0;
    break;
  case D3DFMT_A4R4G4B4:
    // AAAA RRRR GGGG BBBB
    B = (uint8_t)(((px      ) & 0x0F) * 255 / 15);
    G = (uint8_t)(((px >>  4) & 0x0F) * 255 / 15);
    R = (uint8_t)(((px >>  8) & 0x0F) * 255 / 15);
    A = (uint8_t)(((px >> 12) & 0x0F) * 255 / 15);
    break;
  default:
    B = G = R = A = 255;
    break;
  }

  // Metal BGRA8Unorm: byte order is B, G, R, A in memory
  return ((uint32_t)A << 24) | ((uint32_t)R << 16) |
         ((uint32_t)G << 8)  | ((uint32_t)B);
}

// ─────────────────────────────────────────────────────────────────
// Convert a buffer of 16-bit pixels to BGRA8 (32-bit).
// Returns malloc'd buffer that caller must free. Sets outPitch.
// ─────────────────────────────────────────────────────────────────
inline void *Convert16to32(D3DFORMAT fmt, const void *src, UINT width,
                           UINT height, UINT srcPitch, UINT *outPitch) {
  UINT dstPitch = width * 4;
  *outPitch = dstPitch;
  uint8_t *dst = (uint8_t *)malloc(dstPitch * height);
  if (!dst) return nullptr;

  const uint8_t *srcRow = (const uint8_t *)src;
  uint8_t *dstRow = dst;

  for (UINT y = 0; y < height; y++) {
    const uint16_t *sp = (const uint16_t *)srcRow;
    uint32_t *dp = (uint32_t *)dstRow;

    for (UINT x = 0; x < width; x++) {
      dp[x] = ConvertPixel16to32(fmt, sp[x]);
    }
    srcRow += srcPitch;
    dstRow += dstPitch;
  }
  return dst;
}
