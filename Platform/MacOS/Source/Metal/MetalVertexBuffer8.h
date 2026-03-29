#pragma once

#include <windows.h>  // macOS Win32 type shim
#include <d3d8.h>

/**
 * Metal implementation of IDirect3DVertexBuffer8.
 * This is a pure COM-like object — it does NOT inherit from VertexBufferClass.
 * Lifetime is managed by DX8VertexBufferClass which holds a raw pointer to
 * this.
 *
 * TheSuperHackers @perf Zero-copy buffer access.
 * On Apple Silicon with MTLResourceStorageModeShared, CPU and GPU share the
 * same memory. Lock() returns [MTLBuffer contents] directly, eliminating the
 * system memory copy and memcpy overhead that existed before.
 */
class MetalVertexBuffer8 : public IDirect3DVertexBuffer8 {
public:
  MetalVertexBuffer8(unsigned FVF, unsigned short VertexCount,
                     unsigned vertex_size = 0);
  virtual ~MetalVertexBuffer8();

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

  // IDirect3DVertexBuffer8 methods
  STDMETHOD(Lock)
  (THIS_ UINT OffsetToLock, UINT SizeToLock, BYTE **ppbData, DWORD Flags)
      override;
  STDMETHOD(Unlock)(THIS) override;
  STDMETHOD(GetDesc)(D3DVERTEXBUFFER_DESC *pDesc) override;

  // Metal specific
  void *GetMTLBuffer();

protected:
  uint8_t *m_SysMemCopy;    // Fallback for early init when device not ready
  unsigned int m_FVF;
  unsigned int m_VertexCount;
  unsigned int m_VertexSize;
  int m_RefCount;
  void *m_MTLBuffer;         // id<MTLBuffer> — primary storage (Shared mode)
};
