#pragma once

#include "always.h"
#include <d3d8.h>
#include <Metal/Metal.h>
#include <map>

class MetalDevice8;

class MetalCubeTexture8 : public IDirect3DCubeTexture8 {
  W3DMPO_GLUE(MetalCubeTexture8)
public:
  MetalCubeTexture8(MetalDevice8 *device, UINT edgeLength, UINT levels,
                    DWORD usage, D3DFORMAT format, D3DPOOL pool);
  MetalCubeTexture8(MetalDevice8 *device, void *mtlTexture, D3DFORMAT format);
  virtual ~MetalCubeTexture8();

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
  HRESULT LockRect(D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) override;
  HRESULT UnlockRect(D3DCUBEMAP_FACES FaceType, UINT Level) override;

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

  UINT m_Size;
  UINT m_Levels;
  DWORD m_Usage;
  D3DFORMAT m_Format;
  D3DPOOL m_Pool;
  bool m_HasBeenWritten = false;
  DWORD m_LOD = 0;
  uint32_t m_Generation = 0;

  struct LockedLevel {
    void *ptr;
    UINT pitch;
    UINT bytesPerPixel;
  };
  std::map<std::pair<UINT, UINT>, LockedLevel> m_LockedLevels; // <FaceType, Level>

  void *m_ConvertBuf = nullptr;
  uint32_t m_ConvertBufSize = 0;
  void EnsureConvertBuffer(uint32_t needed);
};
