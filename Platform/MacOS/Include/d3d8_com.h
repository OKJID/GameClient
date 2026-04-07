/*
**  d3d8_com.h — D3D8 COM interface stubs for macOS
**  Abstract base classes matching the real IDirect3D8 vtable layout.
*/
#pragma once
#ifdef __APPLE__

#ifndef STDMETHODIMP
#define STDMETHODIMP HRESULT
#define STDMETHODIMP_(type) type
#endif

#ifndef REFIID
typedef const GUID& REFIID;
typedef const GUID& REFGUID;
#define IID_IUnknown GUID{}
#endif

#ifndef E_NOTIMPL
#define E_NOTIMPL ((HRESULT)0x80004001L)
#endif
#ifndef E_POINTER
#define E_POINTER ((HRESULT)0x80004003L)
#endif

struct IDirect3D8;
struct IDirect3DDevice8;
struct IDirect3DResource8;
struct IDirect3DBaseTexture8;
struct IDirect3DTexture8;
struct IDirect3DCubeTexture8;
struct IDirect3DVolumeTexture8;
struct IDirect3DSurface8;
struct IDirect3DVolume8;
struct IDirect3DVertexBuffer8;
struct IDirect3DIndexBuffer8;
struct IDirect3DSwapChain8;

struct IDirect3DResource8 {
  virtual ~IDirect3DResource8() = default;
  virtual ULONG AddRef() { return 1; }
  virtual ULONG Release() { return 1; }
  virtual D3DRESOURCETYPE GetType() = 0;
};

struct IDirect3DVertexBuffer8 : public IDirect3DResource8 {
  virtual HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, BYTE **ppbData, DWORD Flags) = 0;
  virtual HRESULT Unlock() = 0;
  virtual HRESULT GetDesc(D3DVERTEXBUFFER_DESC *pDesc) = 0;
};

struct IDirect3DIndexBuffer8 : public IDirect3DResource8 {
  virtual HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, BYTE **ppbData, DWORD Flags) = 0;
  virtual HRESULT Unlock() = 0;
  virtual HRESULT GetDesc(D3DINDEXBUFFER_DESC *pDesc) = 0;
};

struct IDirect3DSurface8 {
  virtual ~IDirect3DSurface8() = default;
  virtual ULONG AddRef() { return 1; }
  virtual ULONG Release() { return 1; }
  virtual HRESULT GetDesc(D3DSURFACE_DESC *pDesc) = 0;
  virtual HRESULT LockRect(D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) = 0;
  virtual HRESULT UnlockRect() = 0;
};

struct IDirect3DBaseTexture8 : public IDirect3DResource8 {
  virtual DWORD SetLOD(DWORD LODNew) = 0;
  virtual DWORD GetLOD() = 0;
  virtual DWORD GetLevelCount() = 0;
  virtual DWORD GetPriority() { return 0; }
  virtual DWORD SetPriority(DWORD PriorityNew) { return 0; }
};

struct IDirect3DTexture8 : public IDirect3DBaseTexture8 {
  virtual HRESULT GetLevelDesc(UINT Level, D3DSURFACE_DESC *pDesc) = 0;
  virtual HRESULT GetSurfaceLevel(UINT Level, IDirect3DSurface8 **ppSurfaceLevel) = 0;
  virtual HRESULT LockRect(UINT Level, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) = 0;
  virtual HRESULT UnlockRect(UINT Level) = 0;
  virtual HRESULT AddDirtyRect(const RECT *pDirtyRect) = 0;
};

struct IDirect3DVolumeTexture8 : public IDirect3DBaseTexture8 {
  virtual HRESULT GetLevelDesc(UINT Level, D3DVOLUME_DESC *pDesc) = 0;
  virtual HRESULT LockBox(UINT Level, D3DLOCKED_BOX *pLockedVolume, const void *pBox, DWORD Flags) = 0;
  virtual HRESULT UnlockBox(UINT Level) = 0;
};

struct IDirect3DCubeTexture8 : public IDirect3DBaseTexture8 {
  virtual HRESULT GetLevelDesc(UINT Level, D3DSURFACE_DESC *pDesc) = 0;
  virtual HRESULT LockRect(D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags) = 0;
  virtual HRESULT UnlockRect(D3DCUBEMAP_FACES FaceType, UINT Level) = 0;
};

struct IDirect3DVolume8 { virtual ~IDirect3DVolume8() = default; };

struct IDirect3DSwapChain8 {
  virtual ~IDirect3DSwapChain8() = default;
  virtual HRESULT Present(const void *s, const void *d, HWND w, const void *r) = 0;
  virtual HRESULT GetBackBuffer(UINT i, D3DBACKBUFFER_TYPE t, IDirect3DSurface8 **b) = 0;
};

struct IDirect3DDevice8 {
  virtual ~IDirect3DDevice8() = default;
  virtual HRESULT TestCooperativeLevel() = 0;
  virtual HRESULT SetVertexShader(DWORD v) = 0;
  virtual HRESULT DeleteVertexShader(DWORD v) = 0;
  virtual HRESULT SetPixelShader(DWORD v) = 0;
  virtual HRESULT DeletePixelShader(DWORD v) = 0;
  virtual HRESULT CreatePixelShader(const DWORD *pFunction, DWORD *pHandle) = 0;
  virtual HRESULT SetVertexShaderConstant(DWORD r, const void *d, DWORD c) = 0;
  virtual HRESULT SetPixelShaderConstant(DWORD r, const void *d, DWORD c) = 0;
  virtual HRESULT SetTransform(D3DTRANSFORMSTATETYPE t, const D3DMATRIX *m) = 0;
  virtual HRESULT GetTransform(D3DTRANSFORMSTATETYPE t, D3DMATRIX *m) = 0;
  virtual HRESULT LightEnable(DWORD i, BOOL b) = 0;
  virtual HRESULT SetTexture(DWORD s, IDirect3DBaseTexture8 *t) = 0;
  virtual HRESULT SetRenderState(D3DRENDERSTATETYPE s, DWORD v) = 0;
  virtual HRESULT GetRenderState(D3DRENDERSTATETYPE s, DWORD *v) = 0;
  virtual HRESULT SetTextureStageState(DWORD s, D3DTEXTURESTAGESTATETYPE t, DWORD v) = 0;
  virtual HRESULT GetTextureStageState(DWORD s, D3DTEXTURESTAGESTATETYPE t, DWORD *v) = 0;
  virtual HRESULT SetLight(DWORD i, const D3DLIGHT8 *l) = 0;
  virtual HRESULT SetViewport(const D3DVIEWPORT8 *v) = 0;
  virtual HRESULT Clear(DWORD c, const void *r, DWORD f, D3DCOLOR cl, float z, DWORD s) = 0;
  virtual HRESULT BeginScene() = 0;
  virtual HRESULT EndScene() = 0;
  virtual HRESULT Present(const void *s, const void *d, HWND w, const void *r) = 0;
  virtual HRESULT GetBackBuffer(UINT i, D3DBACKBUFFER_TYPE t, IDirect3DSurface8 **b) = 0;
  virtual HRESULT GetFrontBuffer(IDirect3DSurface8 *d) = 0;
  virtual HRESULT UpdateTexture(IDirect3DBaseTexture8 *s, IDirect3DBaseTexture8 *d) = 0;
  virtual HRESULT SetIndices(IDirect3DIndexBuffer8 *i, UINT b) = 0;
  virtual HRESULT DrawIndexedPrimitive(DWORD t, UINT m, UINT v, UINT s, UINT p) = 0;
  virtual HRESULT SetStreamSource(UINT s, IDirect3DVertexBuffer8 *v, UINT d) = 0;
  virtual HRESULT DrawPrimitive(DWORD t, UINT s, UINT p) = 0;
  virtual HRESULT CreateTexture(UINT w, UINT h, UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DTexture8 **t) = 0;
  virtual HRESULT CreateVolumeTexture(UINT w, UINT h, UINT d, UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DVolumeTexture8 **t) = 0;
  virtual HRESULT CreateImageSurface(UINT w, UINT h, D3DFORMAT f, IDirect3DSurface8 **s) = 0;
  virtual HRESULT CreateCubeTexture(UINT s, UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DCubeTexture8 **t) = 0;
  virtual HRESULT CreateVertexBuffer(UINT l, DWORD u, DWORD f, D3DPOOL p, IDirect3DVertexBuffer8 **v) = 0;
  virtual HRESULT CreateIndexBuffer(UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DIndexBuffer8 **i) = 0;
  virtual HRESULT GetRenderTarget(IDirect3DSurface8 **s) = 0;
  virtual HRESULT SetRenderTarget(IDirect3DSurface8 *s, IDirect3DSurface8 *d) = 0;
  virtual HRESULT GetDepthStencilSurface(IDirect3DSurface8 **s) = 0;
  virtual HRESULT SetDepthStencilSurface(IDirect3DSurface8 *s) = 0;
  virtual HRESULT CopyRects(IDirect3DSurface8 *s, const void *r, UINT c, IDirect3DSurface8 *d, const void *p) = 0;
  virtual HRESULT Reset(D3DPRESENT_PARAMETERS *p) = 0;
  virtual HRESULT GetDeviceCaps(D3DCAPS8 *c) = 0;
  virtual HRESULT GetAdapterIdentifier(UINT a, DWORD f, D3DADAPTER_IDENTIFIER8 *i) = 0;
  virtual HRESULT SetMaterial(const D3DMATERIAL8 *m) = 0;
  virtual HRESULT SetClipPlane(DWORD i, const float *p) = 0;
  virtual HRESULT ResourceManagerDiscardBytes(DWORD Bytes) = 0;
  virtual HRESULT ValidateDevice(DWORD *pPasses) = 0;
  virtual HRESULT GetDisplayMode(D3DDISPLAYMODE *pMode) = 0;
  virtual HRESULT CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS *p, IDirect3DSwapChain8 **s) = 0;
  virtual UINT GetAvailableTextureMem() = 0;
  virtual HRESULT DrawPrimitiveUP(DWORD t, UINT c, const void *d, UINT s) = 0;
  virtual HRESULT DrawIndexedPrimitiveUP(DWORD t, UINT m, UINT n, UINT c, const void *i, D3DFORMAT f, const void *d, UINT s) = 0;
  virtual HRESULT CreateVertexShader(const DWORD *d, const DWORD *f, DWORD *h, DWORD fl) = 0;
  virtual HRESULT SetGammaRamp(DWORD Flags, const D3DGAMMARAMP *pRamp) = 0;
  virtual HRESULT GetGammaRamp(D3DGAMMARAMP *pRamp) = 0;
  virtual BOOL ShowCursor(BOOL bShow) = 0;
  virtual HRESULT SetCursorProperties(UINT X, UINT Y, IDirect3DSurface8 *p) = 0;
  virtual void SetCursorPosition(int X, int Y, DWORD Flags) = 0;
};

struct IDirect3D8 {
  virtual ~IDirect3D8() = default;
  virtual HRESULT RegisterSoftwareDevice(void *p) = 0;
  virtual UINT GetAdapterCount() = 0;
  virtual HRESULT GetAdapterIdentifier(UINT a, DWORD f, D3DADAPTER_IDENTIFIER8 *i) = 0;
  virtual UINT GetAdapterModeCount(UINT a) = 0;
  virtual HRESULT EnumAdapterModes(UINT a, UINT m, D3DDISPLAYMODE *p) = 0;
  virtual HRESULT GetAdapterDisplayMode(UINT a, D3DDISPLAYMODE *p) = 0;
  virtual HRESULT CheckDeviceType(UINT a, DWORD t, D3DFORMAT d, D3DFORMAT b, BOOL w) = 0;
  virtual HRESULT CheckDeviceFormat(UINT a, DWORD t, D3DFORMAT af, DWORD u, DWORD r, D3DFORMAT f) = 0;
  virtual HRESULT CheckDeviceMultiSampleType(UINT a, DWORD t, D3DFORMAT f, BOOL w, DWORD m) = 0;
  virtual HRESULT CheckDepthStencilMatch(UINT a, DWORD t, D3DFORMAT af, D3DFORMAT rf, D3DFORMAT df) = 0;
  virtual HRESULT GetDeviceCaps(UINT a, DWORD t, D3DCAPS8 *c) = 0;
  virtual HMONITOR GetAdapterMonitor(UINT a) = 0;
  virtual HRESULT CreateDevice(UINT a, D3DDEVTYPE t, HWND w, DWORD f, D3DPRESENT_PARAMETERS *p, IDirect3DDevice8 **d) = 0;
};

struct ID3DXBuffer {
  virtual ~ID3DXBuffer() = default;
  virtual void *GetBufferPointer() = 0;
  virtual DWORD GetBufferSize() = 0;
  virtual ULONG Release() = 0;
};

typedef IDirect3D8 *LPDIRECT3D8;
typedef IDirect3DDevice8 *LPDIRECT3DDEVICE8;
typedef IDirect3DResource8 *LPDIRECT3DRESOURCE8;
typedef IDirect3DBaseTexture8 *LPDIRECT3DBASETEXTURE8;
typedef IDirect3DTexture8 *LPDIRECT3DTEXTURE8;
typedef IDirect3DCubeTexture8 *LPDIRECT3DCUBETEXTURE8;
typedef IDirect3DVolumeTexture8 *LPDIRECT3DVOLUMETEXTURE8;
typedef IDirect3DSurface8 *LPDIRECT3DSURFACE8;
typedef IDirect3DVolume8 *LPDIRECT3DVOLUME8;
typedef IDirect3DVertexBuffer8 *LPDIRECT3DVERTEXBUFFER8;
typedef IDirect3DIndexBuffer8 *LPDIRECT3DINDEXBUFFER8;
typedef IDirect3DSwapChain8 *LPDIRECT3DSWAPCHAIN8;

#endif // __APPLE__
