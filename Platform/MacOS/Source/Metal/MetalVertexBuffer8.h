#pragma once

#include <windows.h>
#include <d3d8.h>

class MetalVertexBuffer8 : public IDirect3DVertexBuffer8 {
public:
  MetalVertexBuffer8(unsigned FVF, unsigned short VertexCount,
                     unsigned vertex_size = 0);
  virtual ~MetalVertexBuffer8();

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

  HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, BYTE **ppbData, DWORD Flags) override;
  HRESULT Unlock() override;
  HRESULT GetDesc(D3DVERTEXBUFFER_DESC *pDesc) override;

  void *GetMTLBuffer();

protected:
  uint8_t *m_SysMemCopy;
  unsigned int m_FVF;
  unsigned int m_VertexCount;
  unsigned int m_VertexSize;
  int m_RefCount;
  void *m_MTLBuffer;
};
