#pragma once

#include "always.h"
#include <d3d8.h>

class MetalDevice8;
class MetalTexture8;

class MetalSurface8 : public IDirect3DSurface8 {
  W3DMPO_GLUE(MetalSurface8)
public:
  enum SurfaceKind { kColor, kDepth };

  MetalSurface8(MetalDevice8 *device, SurfaceKind kind, UINT w, UINT h,
                D3DFORMAT fmt,
                MetalTexture8 *parentTexture = nullptr, UINT mipLevel = 0);
  virtual ~MetalSurface8();

  ULONG AddRef() override;
  ULONG Release() override;
  HRESULT QueryInterface(REFIID riid, void **ppvObj);
  HRESULT GetDevice(IDirect3DDevice8 **ppDevice);
  HRESULT SetPrivateData(REFGUID g, const void *d, DWORD s, DWORD f);
  HRESULT GetPrivateData(REFGUID g, void *d, DWORD *s);
  HRESULT FreePrivateData(REFGUID g);
  DWORD SetPriority(DWORD p);
  DWORD GetPriority();
  void PreLoad();
  D3DRESOURCETYPE GetType();
  HRESULT GetContainer(REFIID riid, void **ppContainer);

  HRESULT GetDesc(D3DSURFACE_DESC *pDesc) override;
  HRESULT LockRect(D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) override;
  HRESULT UnlockRect() override;

  SurfaceKind GetKind() const { return m_Kind; }
  MetalTexture8 *GetParentTexture() const { return m_ParentTexture; }
  UINT GetWidth() const { return m_Width; }
  UINT GetHeight() const { return m_Height; }
  D3DFORMAT GetD3DFormat() const { return m_Format; }
  void *GetLockedData() const { return m_LockedData; }
  UINT GetLockedPitch() const { return m_LockedPitch; }

private:
  ULONG m_RefCount;
  MetalDevice8 *m_Device;
  SurfaceKind m_Kind;
  UINT m_Width;
  UINT m_Height;
  D3DFORMAT m_Format;
  void *m_LockedData = nullptr;
  UINT m_LockedPitch = 0;
  bool m_LockedReadOnly = false;
  MetalTexture8 *m_ParentTexture = nullptr;
  UINT m_MipLevel = 0;
};
