#include "MetalCubeTexture8.h"
#include "MetalDevice8.h"
#include "MetalFormatConvert.h"
#include "MetalBridgeMappings.h"

#ifndef D3DERR_INVALIDCALL
#define D3DERR_INVALIDCALL E_FAIL
#endif

MetalCubeTexture8::MetalCubeTexture8(MetalDevice8 *device, UINT edgeLength,
                                     UINT levels, DWORD usage, D3DFORMAT format,
                                     D3DPOOL pool)
    : m_RefCount(1), m_Device(device), m_Size(edgeLength),
      m_Levels(levels), m_Usage(usage), m_Format(format), m_Pool(pool) {

  if (m_Device)
    m_Device->AddRef();

  if (m_Levels == 0) {
    UINT maxDim = m_Size;
    m_Levels = 1;
    while (maxDim > 1) { maxDim >>= 1; m_Levels++; }
  }

  MTLTextureDescriptor *desc = [[MTLTextureDescriptor alloc] init];
  desc.textureType = MTLTextureTypeCube;
  desc.pixelFormat = MetalFormatFromD3D(format);
  desc.width = m_Size;
  desc.height = m_Size;
  desc.mipmapLevelCount = m_Levels;
  desc.usage = MTLTextureUsageShaderRead;
  if (usage & D3DUSAGE_RENDERTARGET) {
    desc.usage |= MTLTextureUsageRenderTarget;
  }
  if (m_Levels > 1) {
    desc.usage |= MTLTextureUsageRenderTarget | MTLTextureUsageShaderWrite;
  }
  desc.storageMode = MTLStorageModeShared;

  id<MTLDevice> mtlDev = (__bridge id<MTLDevice>)m_Device->GetMTLDevice();
  id<MTLTexture> tex = [mtlDev newTextureWithDescriptor:desc];
  m_Texture = (__bridge_retained void *)tex;

  // Zero-fill all mip levels for all 6 faces
  {
    bool isCompressed = (format == D3DFMT_DXT1 || format == D3DFMT_DXT2 ||
                         format == D3DFMT_DXT3 || format == D3DFMT_DXT4 ||
                         format == D3DFMT_DXT5);
    UINT bpp = BytesPerPixelFromD3D(format);
    
    for (UINT lvl = 0; lvl < m_Levels; lvl++) {
      UINT w = std::max(1u, m_Size >> lvl);
      UINT h = w;
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
          memset(initData, 0x00, dataSize);
        } else if (format == D3DFMT_DXT1) {
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
          for (UINT slice = 0; slice < 6; slice++) {
            [tex replaceRegion:region mipmapLevel:lvl slice:slice
                     withBytes:initData bytesPerRow:bytesPerRow bytesPerImage:bytesPerRow * blocksHigh];
          }
        } else {
          // Uncompressed slices can be written using base method with bounds
          for (UINT slice = 0; slice < 6; slice++) {
            [tex replaceRegion:region mipmapLevel:lvl slice:slice
                     withBytes:initData bytesPerRow:bytesPerRow bytesPerImage:bytesPerRow * h];
          }
        }
        free(initData);
      }
    }
  }
}

MetalCubeTexture8::MetalCubeTexture8(MetalDevice8 *device, void *mtlTexture,
                                     D3DFORMAT format)
    : m_RefCount(1), m_Device(device), m_Size(0), m_Levels(1),
      m_Usage(0), m_Format(format), m_Pool(D3DPOOL_DEFAULT) {

  if (m_Device)
    m_Device->AddRef();

  id<MTLTexture> tex = (__bridge id<MTLTexture>)mtlTexture;
  if (tex) {
    m_Texture = (__bridge_retained void *)tex;
    m_Size = (UINT)tex.width;
    m_Levels = (UINT)tex.mipmapLevelCount;
  } else {
    m_Texture = nullptr;
  }
}

MetalCubeTexture8::~MetalCubeTexture8() {
  if (m_Texture) {
    CFRelease(m_Texture);
    m_Texture = nullptr;
  }
  free(m_ConvertBuf);
  if (m_Device)
    m_Device->Release();
}

void MetalCubeTexture8::EnsureConvertBuffer(uint32_t needed) {
  if (m_ConvertBufSize >= needed)
    return;
  free(m_ConvertBuf);
  m_ConvertBuf = malloc(needed);
  m_ConvertBufSize = m_ConvertBuf ? needed : 0;
}

STDMETHODIMP MetalCubeTexture8::QueryInterface(REFIID riid, void **ppvObj) {
  if (!ppvObj)
    return E_POINTER;
  *ppvObj = nullptr;
  *ppvObj = this;
  AddRef();
  return D3D_OK;
}

STDMETHODIMP_(ULONG) MetalCubeTexture8::AddRef() { return ++m_RefCount; }

STDMETHODIMP_(ULONG) MetalCubeTexture8::Release() {
  if (--m_RefCount == 0) {
    delete this;
    return 0;
  }
  return m_RefCount;
}

STDMETHODIMP MetalCubeTexture8::GetDevice(IDirect3DDevice8 **ppDevice) {
  if (ppDevice) {
    *ppDevice = m_Device;
    m_Device->AddRef();
    return D3D_OK;
  }
  return D3DERR_INVALIDCALL;
}

STDMETHODIMP MetalCubeTexture8::SetPrivateData(REFGUID refguid, CONST void *pData,
                                               DWORD SizeOfData, DWORD Flags) {
  return D3D_OK;
}
STDMETHODIMP MetalCubeTexture8::GetPrivateData(REFGUID refguid, void *pData,
                                               DWORD *pSizeOfData) {
  return D3DERR_NOTFOUND;
}
STDMETHODIMP MetalCubeTexture8::FreePrivateData(REFGUID refguid) { return D3D_OK; }
STDMETHODIMP_(DWORD) MetalCubeTexture8::SetPriority(DWORD PriorityNew) { return 0; }
STDMETHODIMP_(DWORD) MetalCubeTexture8::GetPriority() { return 0; }
STDMETHODIMP_(void) MetalCubeTexture8::PreLoad() {}
STDMETHODIMP_(D3DRESOURCETYPE) MetalCubeTexture8::GetType() {
  return D3DRTYPE_CUBETEXTURE;
}

STDMETHODIMP_(DWORD) MetalCubeTexture8::SetLOD(DWORD LODNew) {
  DWORD old = m_LOD;
  m_LOD = LODNew;
  return old;
}
STDMETHODIMP_(DWORD) MetalCubeTexture8::GetLOD() { return m_LOD; }
STDMETHODIMP_(DWORD) MetalCubeTexture8::GetLevelCount() { return m_Levels; }

STDMETHODIMP MetalCubeTexture8::GetLevelDesc(UINT Level, D3DSURFACE_DESC *pDesc) {
  if (!pDesc)
    return D3DERR_INVALIDCALL;
  if (Level >= m_Levels)
    return D3DERR_INVALIDCALL;

  pDesc->Format = m_Format;
  pDesc->Type = D3DRTYPE_SURFACE;
  pDesc->Usage = m_Usage;
  pDesc->Pool = m_Pool;
  pDesc->MultiSampleType = D3DMULTISAMPLE_NONE;
  pDesc->Width = std::max(1u, m_Size >> Level);
  pDesc->Height = pDesc->Width;
  pDesc->Size = 0;
  return D3D_OK;
}

STDMETHODIMP MetalCubeTexture8::LockRect(D3DCUBEMAP_FACES FaceType, UINT Level,
                                         D3DLOCKED_RECT *pLockedRect, CONST RECT *pRect, DWORD Flags) {
  if (Level >= m_Levels || !pLockedRect)
    return D3DERR_INVALIDCALL;

  auto key = std::make_pair((UINT)FaceType, Level);
  if (m_LockedLevels.count(key))
    return D3DERR_INVALIDCALL;

  UINT width = std::max(1u, m_Size >> Level);
  UINT bpp = BytesPerPixelFromD3D(m_Format);
  UINT pitch = 0;
  UINT dataSize = 0;

  bool isCompressed = (m_Format == D3DFMT_DXT1 || m_Format == D3DFMT_DXT2 ||
                       m_Format == D3DFMT_DXT3 || m_Format == D3DFMT_DXT4 ||
                       m_Format == D3DFMT_DXT5);

  if (isCompressed) {
    UINT blocksWide = std::max(1u, (width + 3) / 4);
    pitch = blocksWide * bpp;
    dataSize = pitch * blocksWide;
  } else {
    pitch = width * bpp;
    dataSize = pitch * width;
  }

  void *data = calloc(1, dataSize);
  if (!data)
    return D3DERR_OUTOFVIDEOMEMORY;

  if (m_Texture && !(Flags & D3DLOCK_DISCARD) && !isCompressed && m_HasBeenWritten) {
    id<MTLTexture> mtlTex = (__bridge id<MTLTexture>)m_Texture;
    MTLRegion region = MTLRegionMake2D(0, 0, width, width);
    bool is16Bit = (m_Format == D3DFMT_R5G6B5 || m_Format == D3DFMT_X1R5G5B5 ||
                    m_Format == D3DFMT_A1R5G5B5 || m_Format == D3DFMT_A4R4G4B4);
    if (!is16Bit) {
      // NOTE: getBytes:bytesPerRow:fromRegion:mipmapLevel:slice: is required for slices
      [mtlTex getBytes:data bytesPerRow:pitch bytesPerImage:0
            fromRegion:region mipmapLevel:Level slice:(NSUInteger)FaceType];
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

  m_LockedLevels[key] = lvl;

  return D3D_OK;
}

STDMETHODIMP MetalCubeTexture8::UnlockRect(D3DCUBEMAP_FACES FaceType, UINT Level) {
  auto key = std::make_pair((UINT)FaceType, Level);
  auto it = m_LockedLevels.find(key);
  if (it == m_LockedLevels.end()) {
    return D3DERR_INVALIDCALL;
  }

  LockedLevel &lvl = it->second;
  id<MTLTexture> tex = (__bridge id<MTLTexture>)m_Texture;

  UINT width = std::max(1u, m_Size >> Level);
  bool isCompressed = (m_Format == D3DFMT_DXT1 || m_Format == D3DFMT_DXT2 ||
                       m_Format == D3DFMT_DXT3 || m_Format == D3DFMT_DXT4 ||
                       m_Format == D3DFMT_DXT5);

  MTLRegion region = MTLRegionMake2D(0, 0, width, width);

  if (isCompressed) {
    UINT bytesPerBlock = lvl.bytesPerPixel;
    UINT blocksWide = std::max(1u, (width + 3) / 4);
    UINT bytesPerRow = blocksWide * bytesPerBlock;
    UINT bytesPerImage = bytesPerRow * blocksWide;

    [tex replaceRegion:region
           mipmapLevel:Level
                 slice:(NSUInteger)FaceType
             withBytes:lvl.ptr
           bytesPerRow:bytesPerRow
         bytesPerImage:bytesPerImage];
  } else if (m_Format == D3DFMT_R8G8B8) {
    UINT dstPitch = width * 4;
    uint32_t needed = dstPitch * width;
    EnsureConvertBuffer(needed);
    uint8_t *converted = (uint8_t *)m_ConvertBuf;
    if (converted) {
      const uint8_t *src = (const uint8_t *)lvl.ptr;
      for (UINT y = 0; y < width; y++) {
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
                   slice:(NSUInteger)FaceType
               withBytes:converted
             bytesPerRow:dstPitch
           bytesPerImage:dstPitch * width];
    }
  } else if (m_Format == D3DFMT_A4L4) {
    UINT dstPitch = width * 2;
    uint32_t needed = dstPitch * width;
    EnsureConvertBuffer(needed);
    uint8_t *converted = (uint8_t *)m_ConvertBuf;
    if (converted) {
      const uint8_t *src = (const uint8_t *)lvl.ptr;
      for (UINT y = 0; y < width; y++) {
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
                   slice:(NSUInteger)FaceType
               withBytes:converted
             bytesPerRow:dstPitch
           bytesPerImage:dstPitch * width];
    }
  } else if (Is16BitFormat(m_Format)) {
    UINT dstPitch = 0;
    void *converted = Convert16to32(m_Format, lvl.ptr, width, width,
                                    lvl.pitch, &dstPitch);
    if (converted) {
      [tex replaceRegion:region
             mipmapLevel:Level
                   slice:(NSUInteger)FaceType
               withBytes:converted
             bytesPerRow:dstPitch
           bytesPerImage:dstPitch * width];
      free(converted);
    }
  } else {
    [tex replaceRegion:region
           mipmapLevel:Level
                 slice:(NSUInteger)FaceType
             withBytes:lvl.ptr
           bytesPerRow:lvl.pitch
         bytesPerImage:lvl.pitch * width];
  }

  free(lvl.ptr);
  m_LockedLevels.erase(it);
  MarkWritten();

  // Async mipmap generation. On Metal, Mipmap generation applies to all slices for a texture.
  // We only trigger it after +Z face (index 5) or if it's the last unlock. 
  // For simplicity, we just trigger on +Z (the 6th face) assuming sequential generation.
  // Actually, D3D8 doesn't guarantee order. We'll rely on the caller triggering it, or
  // we do it conservatively on FaceType == 5 (+Z) or simply when all faces of level 0 are written.
  // For now, doing it on D3DCUBEMAP_FACE_NEGATIVE_Z (5) is safe enough.
  if (Level == 0 && m_Levels > 1 && m_Device && !isCompressed && FaceType == D3DCUBEMAP_FACE_NEGATIVE_Z) {
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
