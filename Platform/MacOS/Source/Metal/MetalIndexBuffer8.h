#pragma once

#include <windows.h>  // macOS Win32 type shim
#include <d3d8.h>

/**
 * Metal implementation of IDirect3DIndexBuffer8.
 * This is a pure COM-like object — it does NOT inherit from IndexBufferClass.
 * Lifetime is managed by DX8IndexBufferClass which holds a raw pointer to this.
 *
 * TheSuperHackers @perf Zero-copy buffer access.
 * Same approach as MetalVertexBuffer8 — Lock() returns [MTLBuffer contents]
 * directly on Apple Silicon Shared storage.
 */
class MetalIndexBuffer8 : public IDirect3DIndexBuffer8 {
public:
  MetalIndexBuffer8(unsigned count, bool is32bit = false);
  virtual ~MetalIndexBuffer8();

  // IUnknown methods
  STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj) override;
  STDMETHOD_(ULONG, AddRef)(void) override;
  STDMETHOD_(ULONG, Release)(void) override;

  // IDirect3DResource8 methods
  STDMETHOD(GetDevice)(IDirect3DDevice8 **ppDevice) override;
  STDMETHOD(SetPrivateData)
  (REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags) override;
  STDMETHOD(GetPrivateData)
  (REFGUID refguid, void *pData, DWORD *pSizeOfData) override;
  STDMETHOD(FreePrivateData)(REFGUID refguid) override;
  STDMETHOD_(DWORD, SetPriority)(DWORD PriorityNew) override;
  STDMETHOD_(DWORD, GetPriority)(void) override;
  STDMETHOD_(void, PreLoad)(void) override;
  STDMETHOD_(D3DRESOURCETYPE, GetType)(void) override;

  // IDirect3DIndexBuffer8 methods
  STDMETHOD(Lock)
  (THIS_ UINT OffsetToLock, UINT SizeToLock, BYTE **ppbData, DWORD Flags)
      override;
  STDMETHOD(Unlock)(THIS) override;
  STDMETHOD(GetDesc)(D3DINDEXBUFFER_DESC *pDesc) override;

  // Metal specific
  void *GetMTLBuffer();
  bool Is_32Bit() const { return m_Is32Bit; }

protected:
  uint8_t *m_SysMemCopy;    // Fallback for early init when device not ready
  unsigned int m_Count;
  bool m_Is32Bit;
  int m_RefCount;
  void *m_MTLBuffer;         // id<MTLBuffer> — primary storage (Shared mode)
};
