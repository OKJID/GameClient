#pragma once

#include "always.h"
#include <d3d8.h>
#include <Metal/Metal.h>
#include <map>

class MetalDevice8;

class MetalTexture8 : public IDirect3DTexture8 {
  W3DMPO_GLUE(MetalTexture8)
public:
  MetalTexture8(MetalDevice8 *device, UINT width, UINT height, UINT levels,
                DWORD usage, D3DFORMAT format, D3DPOOL pool);
  MetalTexture8(MetalDevice8 *device, void *mtlTexture, D3DFORMAT format);
  virtual ~MetalTexture8();

  ULONG AddRef() override;
  ULONG Release() override;
  D3DRESOURCETYPE GetType() override;

  HRESULT QueryInterface(REFIID riid, void **ppvObj);
  HRESULT GetDevice(IDirect3DDevice8 **ppDevice);
  HRESULT SetPrivateData(REFGUID g, const void *d, DWORD s, DWORD f);
  HRESULT GetPrivateData(REFGUID g, void *d, DWORD *s);
  HRESULT FreePrivateData(REFGUID g);
  DWORD SetPriority(DWORD p);
  DWORD GetPriority();
  void PreLoad();

  DWORD SetLOD(DWORD LODNew) override;
  DWORD GetLOD() override;
  DWORD GetLevelCount() override;

  HRESULT GetLevelDesc(UINT Level, D3DSURFACE_DESC *pDesc) override;
  HRESULT GetSurfaceLevel(UINT Level, IDirect3DSurface8 **ppSurfaceLevel) override;
  HRESULT LockRect(UINT Level, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) override;
  HRESULT UnlockRect(UINT Level) override;
  HRESULT AddDirtyRect(const RECT *pDirtyRect) override;

  id<MTLTexture> GetMTLTexture() const {
    return (__bridge id<MTLTexture>)m_Texture;
  }
  void *GetMTLTextureVoid() const { return m_Texture; }
  void *GetMetalTexture() const { return m_Texture; }
  void MarkWritten() { m_HasBeenWritten = true; ++m_Generation; }
  bool HasBeenWritten() const { return m_HasBeenWritten; }
  D3DFORMAT GetD3DFormat() const { return m_Format; }
  uint32_t GetGeneration() const { return m_Generation; }

private:
  ULONG m_RefCount;
  MetalDevice8 *m_Device;
  void *m_Texture;

  UINT m_Width;
  UINT m_Height;
  UINT m_Levels;
  DWORD m_Usage;
  D3DFORMAT m_Format;
  D3DPOOL m_Pool;
  bool m_HasBeenWritten = false;
  DWORD m_LOD = 0;
  uint32_t m_Generation = 0;

  void *m_BackTexture = nullptr;

  struct LockedLevel {
    void *ptr;
    UINT pitch;
    UINT bytesPerPixel;
  };
  std::map<UINT, LockedLevel> m_LockedLevels;
  std::map<UINT, class MetalSurface8*> m_CachedSurfaces;

  void *m_ConvertBuf = nullptr;
  uint32_t m_ConvertBufSize = 0;
  void EnsureConvertBuffer(uint32_t needed);
};

MTLPixelFormat MetalFormatFromD3D(D3DFORMAT fmt);
UINT BytesPerPixelFromD3D(D3DFORMAT fmt);
