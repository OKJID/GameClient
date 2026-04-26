/**
 * MetalInterface8.mm — IDirect3D8 implementation on Apple Metal
 */

#import "MetalInterface8.h"
#import "MetalDevice8.h"
#include <cstdio>
#include <cstring>
#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#include "MacOSDisplayManager.h"

// ─── Display mode cache ───────────────────────────────────────
struct DisplayModeEntry { UINT w, h, hz; };
static DisplayModeEntry s_modes[64];
static UINT s_modeCount = 0;
static bool s_modesQueried = false;

static void queryDisplayModes() {
  if (s_modesQueried) return;
  s_modesQueried = true;
  s_modeCount = 0;

  // Delegate to MacOSDisplayManager for consistent mode enumeration.
  // This ensures we use the same mode list (in points) everywhere.
  const auto& modes = MacOSDisplayManager::instance().getAvailableModes();
  for (const auto& mode : modes) {
    if (s_modeCount >= 64) break;
    s_modes[s_modeCount++] = { (UINT)mode.w, (UINT)mode.h, (UINT)mode.hz };
  }

  if (s_modeCount == 0) {
    s_modes[0] = { 800, 600, 60 };
    s_modeCount = 1;
  }
  fprintf(stderr, "[MetalInterface8] Enumerated %u display modes (via MacOSDisplayManager)\n", s_modeCount);
}

MetalInterface8::MetalInterface8() : m_RefCount(1) {
  fprintf(stderr, "[MetalInterface8] Created\n");
}

MetalInterface8::~MetalInterface8() {
  fprintf(stderr, "[MetalInterface8] Destroyed\n");
}

// IUnknown

STDMETHODIMP MetalInterface8::QueryInterface(REFIID riid, void **ppvObj) {
  if (ppvObj)
    *ppvObj = nullptr;
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) MetalInterface8::Release() {
  ULONG r = --m_RefCount;
  if (r == 0) {
    delete this;
    return 0;
  }
  return r;
}

// IDirect3D8

STDMETHODIMP MetalInterface8::RegisterSoftwareDevice(void *p) {
  return E_NOTIMPL;
}

STDMETHODIMP_(UINT) MetalInterface8::GetAdapterCount() { return 1; }

STDMETHODIMP
MetalInterface8::GetAdapterIdentifier(UINT Adapter, DWORD Flags,
                                      D3DADAPTER_IDENTIFIER8 *pId) {
  if (!pId)
    return E_POINTER;
  memset(pId, 0, sizeof(*pId));
  strncpy(pId->Description, "Apple Metal GPU", sizeof(pId->Description) - 1);
  pId->VendorId = 0x106B; // Apple
  pId->DeviceId = 0x0001;
  return D3D_OK;
}

STDMETHODIMP_(UINT) MetalInterface8::GetAdapterModeCount(UINT Adapter) {
  queryDisplayModes();
  return s_modeCount;
}

STDMETHODIMP MetalInterface8::EnumAdapterModes(UINT Adapter, UINT Mode,
                                               D3DDISPLAYMODE *pMode) {
  if (!pMode)
    return E_POINTER;
  queryDisplayModes();
  if (Mode >= s_modeCount)
    return E_FAIL;
  pMode->Width = s_modes[Mode].w;
  pMode->Height = s_modes[Mode].h;
  pMode->RefreshRate = s_modes[Mode].hz;
  pMode->Format = D3DFMT_A8R8G8B8;
  return D3D_OK;
}

STDMETHODIMP MetalInterface8::GetAdapterDisplayMode(UINT Adapter,
                                                    D3DDISPLAYMODE *pMode) {
  if (!pMode)
    return E_POINTER;
  // Return current desktop mode in POINTS — consistent with mode enumeration.
  // Previous code multiplied by backingScaleFactor which caused a unit mismatch
  // between enumerated modes (points) and this function (backing pixels).
  auto mode = MacOSDisplayManager::instance().getCurrentDesktopMode();
  pMode->Width = (UINT)mode.w;
  pMode->Height = (UINT)mode.h;
  pMode->RefreshRate = (UINT)mode.hz;
  pMode->Format = D3DFMT_A8R8G8B8;
  return D3D_OK;
}

STDMETHODIMP MetalInterface8::CheckDeviceType(UINT a, DWORD dt, D3DFORMAT df,
                                              D3DFORMAT bbf, BOOL w) {
  return D3D_OK;
}

STDMETHODIMP MetalInterface8::CheckDeviceFormat(UINT a, DWORD dt, D3DFORMAT af,
                                                DWORD u, DWORD rt,
                                                D3DFORMAT cf) {
  // Specifically reject 16-bit legacy color formats. Modern Apple Silicon GPUs
  // do not support 16-bit packed RGB formats natively, causing the Metal backend
  // to fallback to a purely software (CPU-bound) CopyRect conversion layer.
  if (cf == D3DFMT_R5G6B5 || cf == D3DFMT_X1R5G5B5 ||
      cf == D3DFMT_A1R5G5B5 || cf == D3DFMT_A4R4G4B4 || cf == D3DFMT_R3G3B2) {
    return D3DERR_NOTAVAILABLE;
  }
  return D3D_OK;
}

STDMETHODIMP MetalInterface8::CheckDeviceMultiSampleType(UINT a, DWORD dt,
                                                         D3DFORMAT sf, BOOL w,
                                                         DWORD mst) {
  return D3D_OK;
}

STDMETHODIMP MetalInterface8::CheckDepthStencilMatch(UINT a, DWORD dt,
                                                     D3DFORMAT af,
                                                     D3DFORMAT rtf,
                                                     D3DFORMAT dsf) {
  return D3D_OK;
}

STDMETHODIMP MetalInterface8::GetDeviceCaps(UINT Adapter, DWORD DeviceType,
                                            D3DCAPS8 *pCaps) {
  if (!pCaps)
    return E_POINTER;

  MetalDevice8::FillDeviceCaps(pCaps);
  return D3D_OK;
}

STDMETHODIMP_(HMONITOR) MetalInterface8::GetAdapterMonitor(UINT Adapter) {
  return nullptr;
}

STDMETHODIMP MetalInterface8::CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType,
                                           HWND hFocusWindow,
                                           DWORD BehaviorFlags,
                                           D3DPRESENT_PARAMETERS *pPP,
                                           IDirect3DDevice8 **ppDevice) {
  if (!ppDevice)
    return E_POINTER;

  MetalDevice8 *dev = new MetalDevice8();
  if (!dev->InitMetal(hFocusWindow)) {
    delete dev;
    *ppDevice = nullptr;
    return E_FAIL;
  }

  *ppDevice = dev;
  fprintf(stderr, "[MetalInterface8] CreateDevice: OK (%p)\n", dev);
  return D3D_OK;
}

// ═══════════════════════════════════════════════════════════════
//  extern "C" Factory Functions — called from D3DXStubs.cpp
// ═══════════════════════════════════════════════════════════════

extern "C" IDirect3D8 *CreateMetalInterface8() { return new MetalInterface8(); }

extern "C" IDirect3DDevice8 *CreateMetalDevice8() { return new MetalDevice8(); }

// Wrapper with void* return type — called from windows.h GetProcAddress stub
// which cannot include d3d8.h. This avoids return-type conflicts.
extern "C" void *_CreateMetalInterface8_Wrapper() {
  return static_cast<void *>(CreateMetalInterface8());
}


