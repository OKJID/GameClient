#pragma once


#include <windows.h>
#include <d3d8.h>

#ifdef __OBJC__
@protocol MTLDevice;
@protocol MTLCommandQueue;
@protocol MTLCommandBuffer;
@protocol MTLRenderCommandEncoder;
@protocol CAMetalDrawable;
@class CAMetalLayer;
#else
typedef void *id;
#endif

#include <map>
#include <vector>

static const int MAX_VS_CONSTANTS = 96;

struct VSHandleInfo {
  DWORD handle;
  DWORD fvf;
  uint32_t shaderType;
};

static const int MAX_PS_CONSTANTS = 8;

enum PSType {
  PS_NONE = 0, PS_TERRAIN = 1, PS_TERRAIN_NOISE1 = 2, PS_TERRAIN_NOISE2 = 3,
  PS_ROAD_NOISE2 = 4, PS_MONOCHROME = 5, PS_WAVE = 6, PS_FLAT_TERRAIN = 7,
  PS_FLAT_TERRAIN0 = 8, PS_FLAT_TERRAIN_NOISE1 = 9, PS_FLAT_TERRAIN_NOISE2 = 10,
  PS_WATER_TRAPEZOID = 11, PS_WATER_BUMP = 12, PS_WATER_RIVER = 13,
};

struct PSHandleInfo {
  DWORD handle;
  uint32_t psType;
  uint32_t numTexStages;
  uint32_t numArithOps;
};

class MetalSurface8;

class MetalDevice8 : public IDirect3DDevice8 {
public:
  MetalDevice8();
  virtual ~MetalDevice8();

  bool InitMetal(void *windowHandle);

  HRESULT QueryInterface(REFIID riid, void **ppvObj);
  ULONG AddRef() { return ++m_RefCount; }
  ULONG Release();

  HRESULT TestCooperativeLevel() override;
  HRESULT SetVertexShader(DWORD v) override;
  HRESULT DeleteVertexShader(DWORD v) override;
  HRESULT SetPixelShader(DWORD v) override;
  HRESULT DeletePixelShader(DWORD v) override;
  HRESULT CreatePixelShader(const DWORD *pFunction, DWORD *pHandle) override;
  HRESULT SetVertexShaderConstant(DWORD r, const void *d, DWORD c) override;
  HRESULT SetPixelShaderConstant(DWORD r, const void *d, DWORD c) override;
  HRESULT SetTransform(D3DTRANSFORMSTATETYPE t, const D3DMATRIX *m) override;
  HRESULT GetTransform(D3DTRANSFORMSTATETYPE t, D3DMATRIX *m) override;
  HRESULT LightEnable(DWORD i, BOOL b) override;
  HRESULT SetTexture(DWORD s, IDirect3DBaseTexture8 *t) override;
  HRESULT SetRenderState(D3DRENDERSTATETYPE s, DWORD v) override;
  HRESULT GetRenderState(D3DRENDERSTATETYPE s, DWORD *v) override;
  HRESULT SetTextureStageState(DWORD s, D3DTEXTURESTAGESTATETYPE t, DWORD v) override;
  HRESULT GetTextureStageState(DWORD s, D3DTEXTURESTAGESTATETYPE t, DWORD *v) override;
  HRESULT SetLight(DWORD i, const D3DLIGHT8 *l) override;
  HRESULT SetViewport(const D3DVIEWPORT8 *v) override;
  HRESULT Clear(DWORD c, const void *r, DWORD f, D3DCOLOR cl, float z, DWORD s) override;
  HRESULT BeginScene() override;
  HRESULT EndScene() override;
  HRESULT Present(const void *s, const void *d, HWND w, const void *r) override;
  HRESULT GetBackBuffer(UINT i, D3DBACKBUFFER_TYPE t, IDirect3DSurface8 **b) override;
  HRESULT GetFrontBuffer(IDirect3DSurface8 *d) override;
  HRESULT UpdateTexture(IDirect3DBaseTexture8 *s, IDirect3DBaseTexture8 *d) override;
  HRESULT SetIndices(IDirect3DIndexBuffer8 *i, UINT b) override;
  HRESULT DrawIndexedPrimitive(DWORD t, UINT m, UINT v, UINT s, UINT p) override;
  HRESULT SetStreamSource(UINT s, IDirect3DVertexBuffer8 *v, UINT d) override;
  HRESULT DrawPrimitive(DWORD t, UINT s, UINT p) override;
  HRESULT CreateTexture(UINT w, UINT h, UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DTexture8 **t) override;
  HRESULT CreateVolumeTexture(UINT w, UINT h, UINT d, UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DVolumeTexture8 **t) override;
  HRESULT CreateImageSurface(UINT w, UINT h, D3DFORMAT f, IDirect3DSurface8 **s) override;
  HRESULT CreateCubeTexture(UINT s, UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DCubeTexture8 **t) override;
  HRESULT CreateVertexBuffer(UINT l, DWORD u, DWORD f, D3DPOOL p, IDirect3DVertexBuffer8 **v) override;
  HRESULT CreateIndexBuffer(UINT l, DWORD u, D3DFORMAT f, D3DPOOL p, IDirect3DIndexBuffer8 **i) override;
  HRESULT GetRenderTarget(IDirect3DSurface8 **s) override;
  HRESULT SetRenderTarget(IDirect3DSurface8 *s, IDirect3DSurface8 *d) override;
  HRESULT GetDepthStencilSurface(IDirect3DSurface8 **s) override;
  HRESULT SetDepthStencilSurface(IDirect3DSurface8 *s) override;
  HRESULT CopyRects(IDirect3DSurface8 *s, const void *r, UINT c, IDirect3DSurface8 *d, const void *p) override;
  HRESULT Reset(D3DPRESENT_PARAMETERS *p) override;
  HRESULT GetDeviceCaps(D3DCAPS8 *c) override;
  static void FillDeviceCaps(D3DCAPS8 *pCaps);
  HRESULT GetAdapterIdentifier(UINT a, DWORD f, D3DADAPTER_IDENTIFIER8 *i) override;
  HRESULT SetMaterial(const D3DMATERIAL8 *m) override;
  HRESULT SetClipPlane(DWORD i, const float *p) override;
  HRESULT ResourceManagerDiscardBytes(DWORD Bytes) override;
  HRESULT ValidateDevice(DWORD *pPasses) override;
  HRESULT GetDisplayMode(D3DDISPLAYMODE *pMode) override;
  HRESULT CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS *p, IDirect3DSwapChain8 **s) override;
  UINT GetAvailableTextureMem() override;
  HRESULT DrawPrimitiveUP(DWORD t, UINT c, const void *d, UINT s) override;
  HRESULT DrawIndexedPrimitiveUP(DWORD t, UINT m, UINT n, UINT c, const void *i, D3DFORMAT f, const void *d, UINT s) override;
  HRESULT CreateVertexShader(const DWORD *d, const DWORD *f, DWORD *h, DWORD fl) override;
  HRESULT SetGammaRamp(DWORD Flags, const D3DGAMMARAMP *pRamp) override;
  HRESULT GetGammaRamp(D3DGAMMARAMP *pRamp) override;
  BOOL ShowCursor(BOOL bShow) override;
  HRESULT SetCursorProperties(UINT X, UINT Y, IDirect3DSurface8 *p) override;
  void SetCursorPosition(int X, int Y, DWORD Flags) override;

  // Non-override helpers
  void EnsureCurrentEncoder();
  HRESULT GetDirect3D(IDirect3D8 **ppD3D8);
  HRESULT GetViewport(D3DVIEWPORT8 *pViewport);
  HRESULT GetMaterial(D3DMATERIAL8 *pMaterial);
  HRESULT GetLight(DWORD Index, D3DLIGHT8 *pLight);
  HRESULT GetLightEnable(DWORD Index, BOOL *pEnable);
  HRESULT GetTexture(DWORD Stage, IDirect3DBaseTexture8 **ppTexture);
  HRESULT GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer8 **ppStreamData, UINT *pStride);
  HRESULT GetIndices(IDirect3DIndexBuffer8 **ppIndexData, UINT *pBaseVertexIndex);

  void *GetMTLDevice() const { return m_Device; }
  void *GetMTLCommandQueue() const { return m_CommandQueue; }
  void updateScreenSize(int width, int height);

  uint32_t GetTextureDirtyMask() const { return m_TextureDirtyMask; }
  void ClearTextureDirty() { m_TextureDirtyMask = 0; }

// Part 2 of state in MetalDevice8_state.h
#include "MetalDevice8_state.h"
};

