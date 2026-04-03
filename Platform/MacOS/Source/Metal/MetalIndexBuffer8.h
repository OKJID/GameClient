#pragma once

#include <windows.h>
#include <d3d8.h>

class MetalIndexBuffer8 : public IDirect3DIndexBuffer8 {
public:
  MetalIndexBuffer8(unsigned count, bool is32bit = false);
  virtual ~MetalIndexBuffer8();

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
  HRESULT GetDesc(D3DINDEXBUFFER_DESC *pDesc) override;

  void *GetMTLBuffer();
  bool Is_32Bit() const { return m_Is32Bit; }

protected:
  uint8_t *m_SysMemCopy;
  unsigned int m_Count;
  bool m_Is32Bit;
  int m_RefCount;
  void *m_MTLBuffer;
};
