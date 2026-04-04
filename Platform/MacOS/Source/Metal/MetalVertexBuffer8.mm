#import "MetalVertexBuffer8.h"
#import "MetalIndexBuffer8.h"
#include "dx8indexbuffer.h"
#include "dx8vertexbuffer.h"
#include "dx8wrapper.h"
#import <Metal/Metal.h>
#include <windows.h>  // macOS Win32 type shim
#include <cstdio>

extern void *g_MetalMTLDevice;

// TheSuperHackers @perf Zero-copy vertex buffer.
// On Apple Silicon with MTLResourceStorageModeShared, CPU and GPU share
// the same unified memory. We create the MTLBuffer eagerly (if the device
// is available) and Lock() returns [buf contents] directly — no system
// memory copy, no memcpy on Unlock(). This eliminates the double-copy
// overhead that existed when m_SysMemCopy was the primary storage.

MetalVertexBuffer8::MetalVertexBuffer8(unsigned int fvf, unsigned short count,
                                       unsigned int size)
    : m_FVF(fvf), m_VertexCount(count), m_VertexSize(size), m_RefCount(1),
      m_MTLBuffer(nullptr), m_SysMemCopy(nullptr) {
  if (g_MetalMTLDevice) {
    id<MTLDevice> device = (__bridge id<MTLDevice>)g_MetalMTLDevice;
    id<MTLBuffer> buf =
        [device newBufferWithLength:count * size
                            options:MTLResourceStorageModeShared];
    m_MTLBuffer = (__bridge_retained void *)buf;
  } else {
    m_SysMemCopy = new uint8_t[count * size];
  }
}

MetalVertexBuffer8::~MetalVertexBuffer8() {
  delete[] m_SysMemCopy;
  if (m_MTLBuffer) {
    id<MTLBuffer> buf = (__bridge_transfer id<MTLBuffer>)m_MTLBuffer;
    buf = nil;
  }
}

STDMETHODIMP MetalVertexBuffer8::QueryInterface(REFIID riid, void **ppvObj) {
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) MetalVertexBuffer8::AddRef() { return ++m_RefCount; }

STDMETHODIMP_(ULONG) MetalVertexBuffer8::Release() {
  if (m_RefCount > 0)
    --m_RefCount;
  return m_RefCount;
}

STDMETHODIMP MetalVertexBuffer8::GetDevice(IDirect3DDevice8 **ppDevice) {
  return E_NOTIMPL;
}

STDMETHODIMP MetalVertexBuffer8::SetPrivateData(REFGUID guid, const void *pData,
                                                DWORD SizeOfData, DWORD Flags) {
  return E_NOTIMPL;
}

STDMETHODIMP MetalVertexBuffer8::GetPrivateData(REFGUID guid, void *pData,
                                                DWORD *pSizeOfData) {
  return E_NOTIMPL;
}

STDMETHODIMP MetalVertexBuffer8::FreePrivateData(REFGUID guid) {
  return E_NOTIMPL;
}

STDMETHODIMP_(DWORD) MetalVertexBuffer8::SetPriority(DWORD PriorityNew) {
  return 0;
}

STDMETHODIMP_(DWORD) MetalVertexBuffer8::GetPriority() { return 0; }

STDMETHODIMP_(void) MetalVertexBuffer8::PreLoad() {}

STDMETHODIMP_(D3DRESOURCETYPE) MetalVertexBuffer8::GetType() {
  return D3DRTYPE_VERTEXBUFFER;
}

STDMETHODIMP MetalVertexBuffer8::Lock(UINT OffsetToLock, UINT SizeToLock,
                                      BYTE **ppbData, DWORD Flags) {
  if (!ppbData)
    return E_POINTER;

  if (Flags & D3DLOCK_DISCARD) {
    if (m_MTLBuffer && g_MetalMTLDevice) {
      id<MTLDevice> device = (__bridge id<MTLDevice>)g_MetalMTLDevice;
      id<MTLBuffer> buf =
          [device newBufferWithLength:m_VertexCount * m_VertexSize
                              options:MTLResourceStorageModeShared];
      id<MTLBuffer> old_buf = (__bridge_transfer id<MTLBuffer>)m_MTLBuffer;
      old_buf = nil; // Automatically released by ARC once command buffers finish
      m_MTLBuffer = (__bridge_retained void *)buf;
    }
  }

  if (m_MTLBuffer) {
    id<MTLBuffer> buf = (__bridge id<MTLBuffer>)m_MTLBuffer;
    *ppbData = (BYTE *)[buf contents] + OffsetToLock;
    return D3D_OK;
  }

  *ppbData = m_SysMemCopy + OffsetToLock;
  return D3D_OK;
}

STDMETHODIMP MetalVertexBuffer8::Unlock() {
  return D3D_OK;
}

void *MetalVertexBuffer8::GetMTLBuffer() {
  if (m_MTLBuffer)
    return m_MTLBuffer;

  id<MTLDevice> device = g_MetalMTLDevice
                             ? (__bridge id<MTLDevice>)g_MetalMTLDevice
                             : MTLCreateSystemDefaultDevice();
  if (!device)
    return nullptr;

  if (m_SysMemCopy) {
    id<MTLBuffer> buf =
        [device newBufferWithBytes:m_SysMemCopy
                            length:m_VertexCount * m_VertexSize
                           options:MTLResourceStorageModeShared];
    m_MTLBuffer = (__bridge_retained void *)buf;
    delete[] m_SysMemCopy;
    m_SysMemCopy = nullptr;
  } else {
    id<MTLBuffer> buf =
        [device newBufferWithLength:m_VertexCount * m_VertexSize
                            options:MTLResourceStorageModeShared];
    m_MTLBuffer = (__bridge_retained void *)buf;
  }

  return m_MTLBuffer;
}

STDMETHODIMP MetalVertexBuffer8::GetDesc(D3DVERTEXBUFFER_DESC *pDesc) {
  if (pDesc) {
    pDesc->Format = D3DFMT_UNKNOWN;
    pDesc->Type = D3DRTYPE_VERTEXBUFFER;
    pDesc->Usage = 0;
    pDesc->Pool = D3DPOOL_MANAGED;
    pDesc->Size = m_VertexCount * m_VertexSize;
    pDesc->FVF = m_FVF;
    return D3D_OK;
  }
  return E_POINTER;
}
