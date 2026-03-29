#include "MetalTexture8.h"
#include "MetalDevice8.h"
#include "MetalSurface8.h"
#include "MetalFormatConvert.h"
#include "MetalBridgeMappings.h"
#include "MetalTextureCapture.h"
#include <map>
#include <cstdio>

#ifndef D3DERR_INVALIDCALL
#define D3DERR_INVALIDCALL E_FAIL
#endif

// BytesPerPixelFromD3D() and MetalFormatFromD3D() are now in MetalFormatConvert.h / MetalBridgeMappings.h

MetalTexture8::MetalTexture8(MetalDevice8 *device, UINT width, UINT height,
                             UINT levels, DWORD usage, D3DFORMAT format,
                             D3DPOOL pool)
    : m_RefCount(1), m_Device(device), m_Width(width), m_Height(height),
      m_Levels(levels), m_Usage(usage), m_Format(format), m_Pool(pool) {

  if (m_Device)
    m_Device->AddRef();

  if (m_Levels == 0) {
    // DX8 spec: 0 means generate all mipmap levels down to 1x1
    UINT maxDim = std::max(width, height);
    m_Levels = 1;
    while (maxDim > 1) { maxDim >>= 1; m_Levels++; }
  }

  MTLTextureDescriptor *desc = [[MTLTextureDescriptor alloc] init];
  desc.pixelFormat = MetalFormatFromD3D(format);
  desc.width = width;
  desc.height = height;
  desc.mipmapLevelCount = m_Levels;
  desc.usage = MTLTextureUsageShaderRead;
  if (usage & D3DUSAGE_RENDERTARGET) {
    desc.usage |= MTLTextureUsageRenderTarget;
  }
  // All textures use Shared storage on Apple Silicon:
  // - Non-RT: so replaceRegion is immediately GPU-visible (no synchronizeResource needed)
  // - RT: so zero-fill at creation is GPU-visible, and after rendering to an RT texture
  //   it can be sampled as a shader input (e.g., RTT → fullscreen blit for soft water edges)
  desc.storageMode = MTLStorageModeShared;

  id<MTLDevice> mtlDev = (__bridge id<MTLDevice>)m_Device->GetMTLDevice();
  id<MTLTexture> tex = [mtlDev newTextureWithDescriptor:desc];
  m_Texture = (__bridge_retained void *)tex; // Retain manual ref

  // Zero-fill all mip levels — MTLStorageModeShared starts with undefined data.
  // Without this, textures that never receive LockRect/UnlockRect data uploads
  // will display as white/garbage on Apple Silicon.
  {
    bool isCompressed = (format == D3DFMT_DXT1 || format == D3DFMT_DXT2 ||
                         format == D3DFMT_DXT3 || format == D3DFMT_DXT4 ||
                         format == D3DFMT_DXT5);
    UINT bpp = BytesPerPixelFromD3D(format);
    for (UINT lvl = 0; lvl < m_Levels; lvl++) {
      UINT w = std::max(1u, width >> lvl);
      UINT h = std::max(1u, height >> lvl);
      UINT dataSize, bytesPerRow;
      if (isCompressed) {
        UINT blocksWide = std::max(1u, (w + 3) / 4);
        UINT blocksHigh = std::max(1u, (h + 3) / 4);
        bytesPerRow = blocksWide * bpp;
        dataSize = bytesPerRow * blocksHigh;
      } else {
        UINT mtlBpp = bpp;
        if (desc.pixelFormat == MTLPixelFormatBGRA8Unorm || desc.pixelFormat == MTLPixelFormatRGBA8Unorm) {
          mtlBpp = 4;
        }
        bytesPerRow = w * mtlBpp;
        dataSize = bytesPerRow * h;
      }
      
      void *initData = malloc(dataSize);
      if (initData) {
        if (usage & D3DUSAGE_RENDERTARGET) {
          memset(initData, 0x00, dataSize); // Transparent black — matches DX8 cleared RT
        } else if (format == D3DFMT_DXT1) {
          // 0x00 creates opaque black for DXT1. We need transparent (code 3).
          // 00 00 (c0) 00 00 (c1) FF FF FF FF (indices)
          uint8_t *p = (uint8_t *)initData;
          for (UINT i = 0; i < dataSize; i += 8) {
            p[i+0] = 0; p[i+1] = 0; p[i+2] = 0; p[i+3] = 0;
            p[i+4] = 0xFF; p[i+5] = 0xFF; p[i+6] = 0xFF; p[i+7] = 0xFF;
          }
        } else {
          memset(initData, 0, dataSize);
        }
        
        MTLRegion region = MTLRegionMake2D(0, 0, w, h);
        if (isCompressed) {
          UINT blocksHigh = std::max(1u, (h + 3) / 4);
          [tex replaceRegion:region mipmapLevel:lvl slice:0
                   withBytes:initData bytesPerRow:bytesPerRow bytesPerImage:bytesPerRow * blocksHigh];
        } else {
          [tex replaceRegion:region mipmapLevel:lvl withBytes:initData bytesPerRow:bytesPerRow];
        }
        free(initData);
      }
    }
  }
}

MetalTexture8::MetalTexture8(MetalDevice8 *device, void *mtlTexture,
                             D3DFORMAT format)
    : m_RefCount(1), m_Device(device), m_Width(0), m_Height(0), m_Levels(1),
      m_Usage(0), m_Format(format), m_Pool(D3DPOOL_DEFAULT) {

  if (m_Device)
    m_Device->AddRef();

  id<MTLTexture> tex = (__bridge id<MTLTexture>)mtlTexture;
  if (tex) {
    m_Texture = (__bridge_retained void *)tex;
    m_Width = (UINT)tex.width;
    m_Height = (UINT)tex.height;
    m_Levels = (UINT)tex.mipmapLevelCount;
  } else {
    m_Texture = nullptr;
  }
}

MetalTexture8::~MetalTexture8() {
  // TheSuperHackers @fix Release cached surfaces before destroying texture.
  for (auto &pair : m_CachedSurfaces) {
    if (pair.second) {
      pair.second->Release();
    }
  }
  m_CachedSurfaces.clear();

  // TheSuperHackers @perf Release double-buffer back texture.
  if (m_BackTexture) {
    CFRelease(m_BackTexture);
    m_BackTexture = nullptr;
  }

  if (m_Texture) {
    CFRelease(m_Texture);
    m_Texture = nullptr;
  }
  free(m_ConvertBuf);
  if (m_Device)
    m_Device->Release();
}

void MetalTexture8::EnsureConvertBuffer(uint32_t needed) {
  if (m_ConvertBufSize >= needed)
    return;
  free(m_ConvertBuf);
  m_ConvertBuf = malloc(needed);
  m_ConvertBufSize = m_ConvertBuf ? needed : 0;
}

STDMETHODIMP MetalTexture8::QueryInterface(REFIID riid, void **ppvObj) {
  if (!ppvObj)
    return E_POINTER;
  *ppvObj = nullptr;
  // Basic IUnknown check (omitting UUID check for brevity/uuid lib missing)
  *ppvObj = this;
  AddRef();
  return D3D_OK;
}

STDMETHODIMP_(ULONG) MetalTexture8::AddRef() { return ++m_RefCount; }

STDMETHODIMP_(ULONG) MetalTexture8::Release() {
  if (--m_RefCount == 0) {
    delete this;
    return 0;
  }
  return m_RefCount;
}

// IDirect3DResource8
STDMETHODIMP MetalTexture8::GetDevice(IDirect3DDevice8 **ppDevice) {
  if (ppDevice) {
    *ppDevice = m_Device;
    m_Device->AddRef();
    return D3D_OK;
  }
  return D3DERR_INVALIDCALL;
}

STDMETHODIMP MetalTexture8::SetPrivateData(REFGUID refguid, CONST void *pData,
                                           DWORD SizeOfData, DWORD Flags) {
  return D3D_OK;
}
STDMETHODIMP MetalTexture8::GetPrivateData(REFGUID refguid, void *pData,
                                           DWORD *pSizeOfData) {
  return D3DERR_NOTFOUND;
}
STDMETHODIMP MetalTexture8::FreePrivateData(REFGUID refguid) { return D3D_OK; }
STDMETHODIMP_(DWORD) MetalTexture8::SetPriority(DWORD PriorityNew) { return 0; }
STDMETHODIMP_(DWORD) MetalTexture8::GetPriority() { return 0; }
STDMETHODIMP_(void) MetalTexture8::PreLoad() {}
STDMETHODIMP_(D3DRESOURCETYPE) MetalTexture8::GetType() {
  return D3DRTYPE_TEXTURE;
}

// IDirect3DBaseTexture8
STDMETHODIMP_(DWORD) MetalTexture8::SetLOD(DWORD LODNew) {
  DWORD old = m_LOD;
  m_LOD = LODNew;
  return old;
}
STDMETHODIMP_(DWORD) MetalTexture8::GetLOD() { return m_LOD; }
STDMETHODIMP_(DWORD) MetalTexture8::GetLevelCount() { return m_Levels; }

// IDirect3DTexture8
STDMETHODIMP MetalTexture8::GetLevelDesc(UINT Level, D3DSURFACE_DESC *pDesc) {
  if (!pDesc)
    return D3DERR_INVALIDCALL;
  if (Level >= m_Levels)
    return D3DERR_INVALIDCALL;

  pDesc->Format = m_Format;
  pDesc->Type = D3DRTYPE_SURFACE;
  pDesc->Usage = m_Usage;
  pDesc->Pool = m_Pool;
  pDesc->MultiSampleType = D3DMULTISAMPLE_NONE;
  pDesc->Width = std::max(1u, m_Width >> Level);
  pDesc->Height = std::max(1u, m_Height >> Level);
  pDesc->Size = 0; // Not used often
  return D3D_OK;
}

STDMETHODIMP
MetalTexture8::GetSurfaceLevel(UINT Level, IDirect3DSurface8 **ppSurfaceLevel) {
  if (!ppSurfaceLevel)
    return E_POINTER;
  if (Level >= m_Levels) {
    *ppSurfaceLevel = nullptr;
    return D3DERR_INVALIDCALL;
  }

  // TheSuperHackers @fix Return cached surface (D3D8 behavior).
  // D3D8 returns the same surface object for a given level with AddRef.
  // This preserves staging buffer data across multiple Lock/Unlock cycles.
  auto it = m_CachedSurfaces.find(Level);
  if (it != m_CachedSurfaces.end() && it->second) {
    it->second->AddRef();
    *ppSurfaceLevel = it->second;
    return D3D_OK;
  }

  UINT w = std::max(1u, m_Width >> Level);
  UINT h = std::max(1u, m_Height >> Level);

  auto *surface =
      new MetalSurface8(m_Device, MetalSurface8::kColor, w, h, m_Format,
                        this, Level);
  m_CachedSurfaces[Level] = surface;
  surface->AddRef(); // one ref for cache, one for caller
  *ppSurfaceLevel = surface;
  return D3D_OK;
}

STDMETHODIMP MetalTexture8::LockRect(UINT Level, D3DLOCKED_RECT *pLockedRect,
                                     CONST RECT *pRect, DWORD Flags) {
  if (Level >= m_Levels || !pLockedRect)
    return D3DERR_INVALIDCALL;

  // Check if checks already locked
  if (m_LockedLevels.count(Level))
    return D3DERR_INVALIDCALL; // Already locked

  // Allocate staging memory
  UINT width = std::max(1u, m_Width >> Level);
  UINT height = std::max(1u, m_Height >> Level);
  UINT bpp = BytesPerPixelFromD3D(m_Format);

  UINT pitch = 0;
  UINT dataSize = 0;

  bool isCompressed = (m_Format == D3DFMT_DXT1 || m_Format == D3DFMT_DXT2 ||
                       m_Format == D3DFMT_DXT3 || m_Format == D3DFMT_DXT4 ||
                       m_Format == D3DFMT_DXT5);

  if (isCompressed) {
    // Blocks are 4x4
    UINT blocksWide = std::max(1u, (width + 3) / 4);
    UINT blocksHigh = std::max(1u, (height + 3) / 4);
    pitch = blocksWide * bpp; // bpp is bytes per block (8 or 16)
    dataSize = pitch * blocksHigh;
  } else {
    pitch = width * bpp;
    dataSize = pitch * height;
  }

  void *data = calloc(1, dataSize);
  if (!data)
    return D3DERR_OUTOFVIDEOMEMORY;

  // Retrieve existing texture data if it's already uploaded.
  // Skip for compressed textures — getBytes on uninitialized BC textures can corrupt heap.
  // Also skip if D3DLOCK_DISCARD is set (caller will overwrite all data).
  if (m_Texture && !(Flags & D3DLOCK_DISCARD) && !isCompressed && m_HasBeenWritten) {
    id<MTLTexture> mtlTex = (__bridge id<MTLTexture>)m_Texture;
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    bool is16Bit = (m_Format == D3DFMT_R5G6B5 || m_Format == D3DFMT_X1R5G5B5 ||
                    m_Format == D3DFMT_A1R5G5B5 || m_Format == D3DFMT_A4R4G4B4);
    if (!is16Bit) {
      [mtlTex getBytes:data bytesPerRow:pitch fromRegion:region mipmapLevel:Level];
    }
  }

  uint8_t *pBits = (uint8_t *)data;
  if (pRect) {
    if (isCompressed) {
      pBits += (pRect->top / 4) * pitch + (pRect->left / 4) * bpp;
    } else {
      pBits += pRect->top * pitch + pRect->left * bpp;
    }
  }

  pLockedRect->pBits = pBits;
  pLockedRect->Pitch = pitch;

  LockedLevel lvl;
  lvl.ptr = data;
  lvl.pitch = pitch;
  lvl.bytesPerPixel = bpp;

  m_LockedLevels[Level] = lvl;

  return D3D_OK;
}

// Is16BitFormat() and Convert16to32() are now in MetalFormatConvert.h

// ─────────────────────────────────────────────────────────────────

STDMETHODIMP MetalTexture8::UnlockRect(UINT Level) {
  auto it = m_LockedLevels.find(Level);
  if (it == m_LockedLevels.end()) {
    return D3DERR_INVALIDCALL;
  }

  LockedLevel &lvl = it->second;



  // ── Texture Capture (for golden-data tests) ──
  if (Level == 0 && TextureCaptureSystem::Instance().IsEnabled()) {
    UINT capW = std::max(1u, m_Width >> Level);
    UINT capH = std::max(1u, m_Height >> Level);
    uint32_t dataSize = capH * lvl.pitch;
    TextureCaptureSystem::Instance().CaptureTexture(
        m_Format, capW, capH, lvl.pitch, lvl.ptr, dataSize);
  }

  // Upload to Metal Texture
  id<MTLTexture> tex = (__bridge id<MTLTexture>)m_Texture;

  // TheSuperHackers @perf Double-buffer for single-level dynamic textures.
  // On Apple Silicon with Shared storage, replaceRegion while GPU reads causes tearing.
  // Instead of allocating a new MTLTexture every unlock (expensive), we pre-allocate
  // a back buffer and swap front/back on each unlock.
  if (m_Levels == 1) {
    if (!m_BackTexture) {
      MTLTextureDescriptor *desc = [[MTLTextureDescriptor alloc] init];
      desc.pixelFormat = tex.pixelFormat;
      desc.width = tex.width;
      desc.height = tex.height;
      desc.mipmapLevelCount = 1;
      desc.usage = tex.usage;
      desc.storageMode = MTLStorageModeShared;
      id<MTLTexture> backTex = [tex.device newTextureWithDescriptor:desc];
      m_BackTexture = (__bridge_retained void *)backTex;
    }
    // Swap front ↔ back
    void *tmp = m_Texture;
    m_Texture = m_BackTexture;
    m_BackTexture = tmp;
    tex = (__bridge id<MTLTexture>)m_Texture;
  }

  UINT width = std::max(1u, m_Width >> Level);
  UINT height = std::max(1u, m_Height >> Level);

  bool isCompressed = (m_Format == D3DFMT_DXT1 || m_Format == D3DFMT_DXT2 ||
                       m_Format == D3DFMT_DXT3 || m_Format == D3DFMT_DXT4 ||
                       m_Format == D3DFMT_DXT5);

  MTLRegion region = MTLRegionMake2D(0, 0, width, height);

  if (isCompressed) {
    // For BC compressed formats, bytesPerRow = blocksWide * bytesPerBlock
    UINT bytesPerBlock = lvl.bytesPerPixel; // 8 for DXT1, 16 for DXT2-5
    UINT blocksWide = std::max(1u, (width + 3) / 4);
    UINT blocksHigh = std::max(1u, (height + 3) / 4);
    UINT bytesPerRow = blocksWide * bytesPerBlock;
    UINT bytesPerImage = bytesPerRow * blocksHigh;

    [tex replaceRegion:region
           mipmapLevel:Level
                 slice:0
             withBytes:lvl.ptr
           bytesPerRow:bytesPerRow
         bytesPerImage:bytesPerImage];
  } else if (m_Format == D3DFMT_R8G8B8) {
    UINT dstPitch = width * 4;
    uint32_t needed = dstPitch * height;
    EnsureConvertBuffer(needed);
    uint8_t *converted = (uint8_t *)m_ConvertBuf;
    if (converted) {
      const uint8_t *src = (const uint8_t *)lvl.ptr;
      for (UINT y = 0; y < height; y++) {
        const uint8_t *srow = src + y * lvl.pitch;
        uint8_t *drow = converted + y * dstPitch;
        for (UINT x = 0; x < width; x++) {
          drow[x * 4 + 0] = srow[x * 3 + 0];
          drow[x * 4 + 1] = srow[x * 3 + 1];
          drow[x * 4 + 2] = srow[x * 3 + 2];
          drow[x * 4 + 3] = 255;
        }
      }
      [tex replaceRegion:region
             mipmapLevel:Level
               withBytes:converted
             bytesPerRow:dstPitch];
    }
  } else if (m_Format == D3DFMT_A4L4) {
    UINT dstPitch = width * 2;
    uint32_t needed = dstPitch * height;
    EnsureConvertBuffer(needed);
    uint8_t *converted = (uint8_t *)m_ConvertBuf;
    if (converted) {
      const uint8_t *src = (const uint8_t *)lvl.ptr;
      for (UINT y = 0; y < height; y++) {
        const uint8_t *srow = src + y * lvl.pitch;
        uint8_t *drow = converted + y * dstPitch;
        for (UINT x = 0; x < width; x++) {
          uint8_t px = srow[x];
          drow[x * 2 + 0] = (uint8_t)(((px     ) & 0x0F) * 255 / 15);
          drow[x * 2 + 1] = (uint8_t)(((px >> 4) & 0x0F) * 255 / 15);
        }
      }
      [tex replaceRegion:region
             mipmapLevel:Level
               withBytes:converted
             bytesPerRow:dstPitch];
    }
  } else if (Is16BitFormat(m_Format)) {
    // Convert 16-bit source data to 32-bit BGRA8 before uploading to Metal
    UINT dstPitch = 0;
    void *converted = Convert16to32(m_Format, lvl.ptr, width, height,
                                    lvl.pitch, &dstPitch);
    if (converted) {
      [tex replaceRegion:region
             mipmapLevel:Level
               withBytes:converted
             bytesPerRow:dstPitch];
      free(converted);
    }
  } else {
    [tex replaceRegion:region
           mipmapLevel:Level
             withBytes:lvl.ptr
           bytesPerRow:lvl.pitch];
  }

  free(lvl.ptr);
  m_LockedLevels.erase(it);
  MarkWritten(); // sets m_HasBeenWritten + increments m_Generation for texture cache

  // TheSuperHackers @perf Async mipmap generation.
  // Metal guarantees command buffer ordering within a queue — the blit
  // will complete before any subsequent render pass that uses this texture.
  // No need for waitUntilCompleted (which was blocking CPU per texture).
  if (Level == 0 && m_Levels > 1 && m_Device && !isCompressed) {
    void *queuePtr = m_Device->GetMTLCommandQueue();
    if (queuePtr) {
      id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)queuePtr;
      id<MTLCommandBuffer> cmdBuf = [queue commandBuffer];
      if (cmdBuf) {
        id<MTLBlitCommandEncoder> blit = [cmdBuf blitCommandEncoder];
        [blit generateMipmapsForTexture:tex];
        [blit endEncoding];
        [cmdBuf commit];
      }
    }
  }

  return D3D_OK;
}

STDMETHODIMP MetalTexture8::AddDirtyRect(CONST RECT *pDirtyRect) {
  return D3D_OK;
}
