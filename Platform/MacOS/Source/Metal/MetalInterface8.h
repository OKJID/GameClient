#pragma once


#include <windows.h>
#include <d3d8.h>

class MetalInterface8 : public IDirect3D8 {
public:
  MetalInterface8();
  virtual ~MetalInterface8();

  HRESULT QueryInterface(REFIID riid, void **ppvObj);
  ULONG AddRef() { return ++m_RefCount; }
  ULONG Release();

  HRESULT RegisterSoftwareDevice(void *pInitializeFunction) override;
  UINT GetAdapterCount() override;
  HRESULT GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER8 *pIdentifier) override;
  UINT GetAdapterModeCount(UINT Adapter) override;
  HRESULT EnumAdapterModes(UINT Adapter, UINT Mode, D3DDISPLAYMODE *pMode) override;
  HRESULT GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE *pMode) override;
  HRESULT CheckDeviceType(UINT Adapter, DWORD CheckType, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed) override;
  HRESULT CheckDeviceFormat(UINT Adapter, DWORD DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, DWORD RType, D3DFORMAT CheckFormat) override;
  HRESULT CheckDeviceMultiSampleType(UINT Adapter, DWORD DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, DWORD MultiSampleType) override;
  HRESULT CheckDepthStencilMatch(UINT Adapter, DWORD DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) override;
  HRESULT GetDeviceCaps(UINT Adapter, DWORD DeviceType, D3DCAPS8 *pCaps) override;
  HMONITOR GetAdapterMonitor(UINT Adapter) override;
  HRESULT CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DDevice8 **ppReturnedDeviceInterface) override;

private:
  ULONG m_RefCount;
};


