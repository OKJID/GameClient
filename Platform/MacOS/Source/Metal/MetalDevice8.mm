/**
 * MetalDevice8.mm — IDirect3DDevice8 implementation on Apple Metal
 *
 * Stage 0: Skeleton — all methods log and return D3D_OK.
 * BeginScene/EndScene/Present/Clear have real Metal frame lifecycle code.
 */
#ifdef __APPLE__

// Import ObjC/Metal frameworks FIRST, before win_compat.h
#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#include <unistd.h>

// Now include our header (which includes d3d8.h / win_compat.h)
#import "MetalDevice8.h"
#include "MetalBridgeMappings.h"
#include "MetalFormatConvert.h"
#include "MetalIndexBuffer8.h"
#include "MetalSurface8.h"
#include "MetalTexture8.h"
#include "MetalVertexBuffer8.h"
#include "MacOSDebugLog.h"
#include "MetalTextureCapture.h"
#include <cstdio>
#include <cstring>

// Global MTLDevice pointer for VB/IB (avoids MTLCreateSystemDefaultDevice)
// Set during MetalDevice8::InitMetal(), cleared in destructor.
void *g_MetalMTLDevice = nullptr;

// Global MetalDevice8 pointer — used by MacOSDisplayManager to update screen
// size during resolution changes. Set in InitMetal(), cleared in destructor.
MetalDevice8* g_theMetalDevice = nullptr;

// D3DXGetFVFVertexSize is inline in d3dx8core.h
#include "d3dx8core.h"

// ─────────────────────────────────────────────────────
//  Helpers: Cast opaque pointers to Metal types
// ─────────────────────────────────────────────────────
#define MTL_DEVICE ((__bridge id<MTLDevice>)m_Device)
#define MTL_QUEUE ((__bridge id<MTLCommandQueue>)m_CommandQueue)
#define MTL_LAYER ((__bridge CAMetalLayer *)m_MetalLayer)
#define MTL_CMD_BUF ((__bridge id<MTLCommandBuffer>)m_CurrentCommandBuffer)
#define MTL_DRAWABLE ((__bridge id<CAMetalDrawable>)m_CurrentDrawable)
#define MTL_ENCODER ((__bridge id<MTLRenderCommandEncoder>)m_CurrentEncoder)

#define SET_MTL(member, val)                                                   \
  do {                                                                         \
    m_##member = (__bridge_retained void *)(val);                              \
  } while (0)
#define CLEAR_MTL(member)                                                      \
  do {                                                                         \
    if (m_##member) {                                                          \
      CFRelease(m_##member);                                                   \
      m_##member = nullptr;                                                    \
    }                                                                          \
  } while (0)

// ─────────────────────────────────────────────────────
//  Construction / Destruction
// ─────────────────────────────────────────────────────

MetalDevice8::MetalDevice8()
    : m_RefCount(1), m_Device(nullptr), m_CommandQueue(nullptr),
      m_MetalLayer(nullptr), m_CurrentCommandBuffer(nullptr),
      m_CurrentDrawable(nullptr), m_CurrentEncoder(nullptr), m_InScene(false),
      m_RTTColorTexture(nullptr), m_RTTDepthTexture(nullptr),
      m_RTTSurface(nullptr), m_RTTWidth(0), m_RTTHeight(0),
      m_StreamSource(nullptr), m_StreamStride(0), m_IndexBuffer(nullptr),
      m_BaseVertexIndex(0), m_VertexShader(0), m_PixelShader(0),
      m_HWND(nullptr), m_ScreenWidth(800), m_ScreenHeight(600),
      m_Library(nullptr), m_FunctionVertex(nullptr),
      m_FunctionFragment(nullptr), m_DepthTexture(nullptr),
      m_DepthStencilState(nullptr), m_DepthStateDirty(true),
      m_DrawStateDirty(true), m_LastAppliedCull(0xFFFFFFFF), m_LastAppliedZBias(0xFFFFFFFF),
      m_ZeroBuffer(nullptr), m_FrameSemaphore(nullptr),
      m_DefaultRTSurface(nullptr),
      m_DefaultDepthSurface(nullptr),
      m_MSAASampleCount(4),
      m_MSAAColorTexture(nullptr),
      m_MSAADepthTexture(nullptr),
      m_RingBuffer(nullptr),
      m_RingBufferSize(256 * 1024),
      m_RingBufferOffset(0) {
  // Create frame semaphore for GPU-CPU sync (like DirectX's Present VSync)
  m_FrameSemaphore = (__bridge_retained void *)dispatch_semaphore_create(MAX_FRAMES_IN_FLIGHT);
  memset(m_RenderStates, 0, sizeof(m_RenderStates));
  // Initial DirectX 8 state defaults
  memset(m_TextureStageStates, 0, sizeof(m_TextureStageStates));
  for (int s = 0; s < 8; ++s) {
    m_TextureStageStates[s][D3DTSS_TEXCOORDINDEX] = s;
    if (s == 0) {
      m_TextureStageStates[s][D3DTSS_COLOROP] = D3DTOP_MODULATE;
      m_TextureStageStates[s][D3DTSS_COLORARG1] = D3DTA_TEXTURE;
      m_TextureStageStates[s][D3DTSS_COLORARG2] = D3DTA_CURRENT;
      m_TextureStageStates[s][D3DTSS_ALPHAOP] = D3DTOP_SELECTARG1;
      m_TextureStageStates[s][D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
      m_TextureStageStates[s][D3DTSS_ALPHAARG2] = D3DTA_CURRENT;
    } else {
      m_TextureStageStates[s][D3DTSS_COLOROP] = D3DTOP_DISABLE;
      m_TextureStageStates[s][D3DTSS_ALPHAOP] = D3DTOP_DISABLE;
    }
  }
  
  memset(m_Textures, 0, sizeof(m_Textures));
  memset(m_TextureGeneration, 0, sizeof(m_TextureGeneration));
  m_TextureDirtyMask = 0;
  memset(m_Transforms, 0, sizeof(m_Transforms));
  memset(&m_Viewport, 0, sizeof(m_Viewport));
  memset(&m_Material, 0, sizeof(m_Material));
  memset(m_Lights, 0, sizeof(m_Lights));
  memset(m_LightEnabled, 0, sizeof(m_LightEnabled));
  memset(m_VSConstants, 0, sizeof(m_VSConstants));
  memset(m_PSConstants, 0, sizeof(m_PSConstants));

  auto setIdentity = [](D3DMATRIX &m) {
    memset(&m, 0, sizeof(m));
    m._11 = m._22 = m._33 = m._44 = 1.0f;
  };
  setIdentity(m_Transforms[D3DTS_VIEW]);
  setIdentity(m_Transforms[D3DTS_PROJECTION]);
  setIdentity(m_Transforms[D3DTS_WORLD]);
  for (int i = 0; i < 4; ++i) {
    setIdentity(m_Transforms[D3DTS_TEXTURE0 + i]);
  }

  // DX8 default render states (per spec)
  m_RenderStates[D3DRS_ZENABLE] = TRUE;           // Depth testing on
  m_RenderStates[D3DRS_ZWRITEENABLE] = TRUE;      // Depth writing on
  m_RenderStates[D3DRS_ZFUNC] = D3DCMP_LESSEQUAL; // Standard compare
  m_RenderStates[D3DRS_CULLMODE] = D3DCULL_CCW;   // CCW culling
  m_RenderStates[D3DRS_ALPHABLENDENABLE] = FALSE;
  m_RenderStates[D3DRS_SRCBLEND] = D3DBLEND_ONE;   // DX8 default
  m_RenderStates[D3DRS_DESTBLEND] = D3DBLEND_ZERO; // DX8 default
  m_RenderStates[D3DRS_COLORWRITEENABLE] = 0xF;    // All channels

  // DX8 default texture stage states (per spec)
  // Stage 0: MODULATE color (tex * diffuse), SELECTARG1 alpha (texture alpha)
  m_TextureStageStates[0][D3DTSS_COLOROP] = D3DTOP_MODULATE;
  m_TextureStageStates[0][D3DTSS_COLORARG1] = D3DTA_TEXTURE;
  m_TextureStageStates[0][D3DTSS_COLORARG2] = D3DTA_CURRENT;
  m_TextureStageStates[0][D3DTSS_ALPHAOP] = D3DTOP_SELECTARG1;
  m_TextureStageStates[0][D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
  m_TextureStageStates[0][D3DTSS_ALPHAARG2] = D3DTA_CURRENT;
  // Stages 1+: DISABLE (default)
  for (int s = 1; s < MAX_TEXTURE_STAGES; s++) {
    m_TextureStageStates[s][D3DTSS_COLOROP] = D3DTOP_DISABLE;
    m_TextureStageStates[s][D3DTSS_ALPHAOP] = D3DTOP_DISABLE;
  }
  // Default sampler states: WRAP + LINEAR
  for (int s = 0; s < MAX_TEXTURE_STAGES; s++) {
    m_TextureStageStates[s][D3DTSS_ADDRESSU] = D3DTADDRESS_WRAP;
    m_TextureStageStates[s][D3DTSS_ADDRESSV] = D3DTADDRESS_WRAP;
    m_TextureStageStates[s][D3DTSS_MAGFILTER] = D3DTEXF_LINEAR;
    m_TextureStageStates[s][D3DTSS_MINFILTER] = D3DTEXF_LINEAR;
    m_TextureStageStates[s][D3DTSS_MIPFILTER] = D3DTEXF_NONE;
  }

  // DX8 default lighting render states
  m_RenderStates[D3DRS_LIGHTING] = TRUE;
  m_RenderStates[D3DRS_AMBIENT] = 0x00000000; // Black global ambient
  m_RenderStates[D3DRS_SPECULARENABLE] = FALSE;
  m_RenderStates[D3DRS_NORMALIZENORMALS] = FALSE;
  m_RenderStates[D3DRS_DIFFUSEMATERIALSOURCE] = D3DMCS_MATERIAL;
  m_RenderStates[D3DRS_AMBIENTMATERIALSOURCE] = D3DMCS_MATERIAL;
  m_RenderStates[D3DRS_SPECULARMATERIALSOURCE] = D3DMCS_MATERIAL;
  m_RenderStates[D3DRS_EMISSIVEMATERIALSOURCE] = D3DMCS_MATERIAL;

  // DX8 default fog render states
  m_RenderStates[D3DRS_FOGENABLE] = FALSE;
  m_RenderStates[D3DRS_FOGCOLOR] = 0x00000000;
  m_RenderStates[D3DRS_FOGTABLEMODE] = D3DFOG_NONE;
  m_RenderStates[D3DRS_FOGVERTEXMODE] = D3DFOG_NONE;
  // fogStart=0.0, fogEnd=1.0, fogDensity=1.0 stored as DWORD bit-casts of float
  {
    float fs = 0.0f;
    memcpy(&m_RenderStates[D3DRS_FOGSTART], &fs, 4);
  }
  {
    float fe = 1.0f;
    memcpy(&m_RenderStates[D3DRS_FOGEND], &fe, 4);
  }
  {
    float fd = 1.0f;
    memcpy(&m_RenderStates[D3DRS_FOGDENSITY], &fd, 4);
  }

  // DX8 default material (white diffuse/ambient, no specular/emissive)
  m_Material.Diffuse = {1.0f, 1.0f, 1.0f, 1.0f};
  m_Material.Ambient = {1.0f, 1.0f, 1.0f, 1.0f};
  m_Material.Specular = {0.0f, 0.0f, 0.0f, 0.0f};
  m_Material.Emissive = {0.0f, 0.0f, 0.0f, 0.0f};
  m_Material.Power = 0.0f;
}

MetalDevice8::~MetalDevice8() {
  // Export captured textures (if capture was enabled)
  TextureCaptureSystem::Instance().ExportCpp(
      "Platform/MacOS/Tests/captured_textures_data.cpp");

  // Release sampler state cache
  for (auto &pair : m_SamplerStateCache) {
    if (pair.second)
      CFRelease(pair.second);
  }
  m_SamplerStateCache.clear();

  // Release depth/stencil state cache
  for (auto &pair : m_DepthStencilStateCache) {
    if (pair.second)
      CFRelease(pair.second);
  }
  m_DepthStencilStateCache.clear();
  m_DepthStencilState = nullptr; // just a borrowed pointer, don't release

  // Release default surfaces
  if (m_DefaultRTSurface) {
    m_DefaultRTSurface->Release();
    m_DefaultRTSurface = nullptr;
  }
  if (m_DefaultDepthSurface) {
    m_DefaultDepthSurface->Release();
    m_DefaultDepthSurface = nullptr;
  }

  // Release depth texture
  if (m_DepthTexture) {
    CFRelease(m_DepthTexture);
    m_DepthTexture = nullptr;
  }

  // Release MSAA textures
  if (m_MSAAColorTexture) {
    CFRelease(m_MSAAColorTexture);
    m_MSAAColorTexture = nullptr;
  }
  if (m_MSAADepthTexture) {
    CFRelease(m_MSAADepthTexture);
    m_MSAADepthTexture = nullptr;
  }

  // Release PSO cache
  for (auto &pair : m_PsoCache) {
    if (pair.second)
      CFRelease(pair.second);
  }
  m_PsoCache.clear();

  // Release shader library and functions
  if (m_FunctionFragment) {
    CFRelease(m_FunctionFragment);
    m_FunctionFragment = nullptr;
  }
  if (m_FunctionVertex) {
    CFRelease(m_FunctionVertex);
    m_FunctionVertex = nullptr;
  }
  if (m_Library) {
    CFRelease(m_Library);
    m_Library = nullptr;
  }

  CLEAR_MTL(CurrentEncoder);
  CLEAR_MTL(CurrentCommandBuffer);
  CLEAR_MTL(CurrentDrawable);
  CLEAR_MTL(CommandQueue);
  g_MetalMTLDevice = nullptr;
  g_theMetalDevice = nullptr;
  CLEAR_MTL(Device);
  // MetalLayer is owned by the view, we don't release it
  m_MetalLayer = nullptr;
  fprintf(stderr, "[MetalDevice8] Destroyed\n");
}

#include <simd/simd.h>

// --- Shader Data Structures (Must match MacOSShaders.metal) ---
struct MetalUniforms {
  simd::float4x4 world;
  simd::float4x4 view;
  simd::float4x4 projection;
  simd::float4x4 texMatrix[4];  // D3DTS_TEXTURE0..3 — UV transform matrices
  simd::float2 screenSize;
  int useProjection; // 0=None, 1=3D, 2=2D(ScreenSpace)
  uint32_t shaderSettings;
  uint32_t texTransformFlags[4]; // D3DTSS_TEXTURETRANSFORMFLAGS per stage (0=disabled, 2=COUNT2)
};

// Stage 7: TextureStageConfig (matches MacOSShaders.metal)
struct TextureStageConfig {
  uint32_t colorOp;
  uint32_t colorArg1;
  uint32_t colorArg2;
  uint32_t alphaOp;
  uint32_t alphaArg1;
  uint32_t alphaArg2;
  uint32_t colorArg0;
  uint32_t _pad1;
};

// Stage 7: FragmentUniforms (matches MacOSShaders.metal, buffer 2)
struct FragmentUniforms {
  TextureStageConfig stages[4];
  simd::float4 textureFactor; // D3DRS_TEXTUREFACTOR as RGBA float
  simd::float4 fogColor;
  float fogStart;
  float fogEnd;
  float fogDensity;
  uint32_t fogMode;
  uint32_t alphaTestEnable;
  uint32_t alphaFunc; // D3DCMP enum
  float alphaRef;     // normalized 0..1
  uint32_t hasTexture[4];
  uint32_t specularEnable;
  uint32_t texCoordIndex[4]; // D3DTSS_TEXCOORDINDEX per stage
  uint32_t texFormatType[4]; // 0=Default, 1=Luminance(r,r,r,1), 2=Luminance+Alpha(r,r,r,g), 3=DXT1(BC1)
  uint32_t blendEnabled;     // D3DRS_ALPHABLENDENABLE
};

// Stage 8: LightData (matches MacOSShaders.metal)
// Per-light parameters for DX8 per-vertex lighting
struct LightData {
  simd::float4 diffuse;
  simd::float4 ambient;
  simd::float4 specular;
  simd::float3 position;
  float range;
  simd::float3 direction;
  float falloff;
  float attenuation0;
  float attenuation1;
  float attenuation2;
  float theta;   // inner cone (radians)
  float phi;     // outer cone (radians)
  uint32_t type; // 1=point, 2=spot, 3=directional
  uint32_t enabled;
  float _pad;
};

// Stage 8: LightingUniforms (matches MacOSShaders.metal, buffer 3)
struct LightingUniforms {
  LightData lights[4];
  simd::float4 materialDiffuse;
  simd::float4 materialAmbient;
  simd::float4 materialSpecular;
  simd::float4 materialEmissive;
  float materialPower;
  simd::float4 globalAmbient;
  uint32_t lightingEnabled;
  uint32_t diffuseSource; // D3DMCS: 0=material, 1=color1, 2=color2
  uint32_t ambientSource;
  uint32_t specularSource;
  uint32_t emissiveSource;
  uint32_t hasNormals; // 1 if FVF has D3DFVF_NORMAL
  // Stage 9: Fog parameters (for vertex fog computation)
  float fogStart;
  float fogEnd;
  float fogDensity;
  uint32_t fogMode; // 0=NONE, 1=EXP, 2=EXP2, 3=LINEAR
};

// Custom Vertex Shader Uniforms (buffer 4)
// Passed to Metal vertex shader when a custom DX8 vertex shader is active
struct CustomVSUniforms {
  uint32_t shaderType;    // 0=none, 1=trees, 2=water wave
  uint32_t _pad[3];       // alignment
  simd::float4 c[34];     // VS constant registers c0..c33 (covers all used registers)
};

// FVF bit definitions: see d3d8_stub.h (D3DFVF_XYZ, D3DFVF_XYZRHW, etc.)

// Helper to get FVF from an opaque IDirect3DVertexBuffer8
static DWORD GetBufferFVF(IDirect3DVertexBuffer8 *vb) {
  if (!vb)
    return 0;
  D3DVERTEXBUFFER_DESC desc;
  if (SUCCEEDED(vb->GetDesc(&desc))) {
    return desc.FVF;
  }
  return 0;
}

bool MetalDevice8::InitMetal(void *windowHandle) {
  m_HWND = windowHandle;

  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  if (!device) {
    fprintf(stderr,
            "[MetalDevice8] ERROR: MTLCreateSystemDefaultDevice failed\n");
    return false;
  }
  SET_MTL(Device, device);
  g_MetalMTLDevice = m_Device; // Global access for VB/IB
  g_theMetalDevice = this;     // Global access for MacOSDisplayManager

  // Init texture capture system (reads GENERALS_CAPTURE_TEXTURES env)
  TextureCaptureSystem::Instance().Init();
  TextureCaptureSystem::Instance().CaptureDeviceCaps(this);

  // Create a small zero buffer for default vertex attributes (missing FVF
  // components)
  {
    uint32_t defaultData[16] = {0};
    defaultData[0] = 0xFFFFFFFF; // offset 0: Opaque White (for diffuse)
    defaultData[1] = 0x00000000; // offset 4: Black (for specular)
    // offset 8+: Zeroes (for pos, normal, texcoords)
    
    id<MTLBuffer> zeroBuf =
        [device newBufferWithBytes:defaultData
                            length:sizeof(defaultData)
                           options:MTLResourceStorageModeShared];
    m_ZeroBuffer = (__bridge_retained void *)zeroBuf;
  }

  id<MTLCommandQueue> queue = [device newCommandQueue];
  SET_MTL(CommandQueue, queue);

  id<MTLLibrary> library = [device newDefaultLibrary];
  if (!library) {
    fprintf(stderr, "[MetalDevice8] ERROR: Failed to load default.metallib from app bundle resources.\n");
  } else {
    SET_MTL(Library, library);

    id<MTLFunction> vertFunc = [library newFunctionWithName:@"vertex_main"];
    if (vertFunc)
      SET_MTL(FunctionVertex, vertFunc);

    id<MTLFunction> fragFunc = [library newFunctionWithName:@"fragment_main"];
    if (fragFunc)
      SET_MTL(FunctionFragment, fragFunc);

    if (!vertFunc || !fragFunc) {
      fprintf(
          stderr,
          "[MetalDevice8] ERROR: Failed to find vertex_main/fragment_main\n");
    } else {
      fprintf(stderr, "[MetalDevice8] Shaders compiled successfully.\n");
    }
  }

  CAMetalLayer *layer = [CAMetalLayer layer];
  layer.device = device;
  layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  layer.framebufferOnly = NO;
  layer.opaque = YES; // Ignore backbuffer alpha for window compositing — DX8 uses dest alpha for soft water edges

  // === VSync / Frame Rate Control ===
  // Always disable displaySync — it causes nextDrawable to block on VSync,
  // which deadlocks our single-threaded game loop (can't pump events while
  // waiting for VSync). Frame rate is controlled by FramePacer instead.
  layer.displaySyncEnabled = NO;
  const char *fpsEnv = getenv("GENERALS_FPS_LIMIT");
  int fpsLimit = fpsEnv ? atoi(fpsEnv) : 60;
  fprintf(stderr, "[MetalDevice8] VSync: OFF (frame rate controlled by FramePacer, target=%d)\n", fpsLimit);

  m_MetalLayer = (__bridge_retained void *)layer;

  NSWindow *window = (__bridge NSWindow *)windowHandle;
  if (window) {
    window.contentView.layer = layer;
    window.contentView.wantsLayer = YES;
    CGSize viewSize = window.contentView.bounds.size;
    CGFloat scale = window.backingScaleFactor;

    // Force contentsScale=1.0: the game renders at its native resolution
    // (e.g. 800x600) and macOS scales it to fill the window on Retina displays.
    // This avoids the "squished to 1/4" problem on Retina screens.
    layer.contentsScale = 1.0;
    layer.drawableSize = CGSizeMake(viewSize.width, viewSize.height);
    m_ScreenWidth = viewSize.width;
    m_ScreenHeight = viewSize.height;

    // CRITICAL: Flush the layer transaction to the Window Server immediately.
    // Otherwise, the very first nextDrawable called in BeginScene() will block
    // infinitely because the engine update loop starts BEFORE the main thread
    // runloop has a chance to flush the render tree!
    [CATransaction flush];

    fprintf(stderr, "[MetalDevice8] Initialized: %gx%g (drawable: %gx%g, backingScale: %g, contentsScale: 1.0)\n",
            m_ScreenWidth, m_ScreenHeight, layer.drawableSize.width,
            layer.drawableSize.height, scale);
  } else {
    // Window not yet available — use fallback size
    layer.drawableSize = CGSizeMake(m_ScreenWidth, m_ScreenHeight);
    fprintf(stderr,
            "[MetalDevice8] WARNING: No window handle, using fallback %gx%g\n",
            m_ScreenWidth, m_ScreenHeight);
  }

  // --- MSAA configuration ---
  // Default to 1 (off) — MSAA causes vertical line artifacts on UI borders
  // due to render pass restart behavior in Clear(). Enable via GENERALS_MSAA=4.
  const char *msaaEnv = getenv("GENERALS_MSAA");
  m_MSAASampleCount = msaaEnv ? atoi(msaaEnv) : 1;
  if (m_MSAASampleCount < 1) m_MSAASampleCount = 1;
  if (m_MSAASampleCount > 1 && ![device supportsTextureSampleCount:m_MSAASampleCount]) {
    fprintf(stderr, "[MetalDevice8] Device does not support %dx MSAA, disabling\n", m_MSAASampleCount);
    m_MSAASampleCount = 1;
  }
  fprintf(stderr, "[MetalDevice8] MSAA: %dx\n", m_MSAASampleCount);

  // Create depth texture matching the drawable size
  UINT depthW = (UINT)layer.drawableSize.width;
  UINT depthH = (UINT)layer.drawableSize.height;
  if (depthW > 0 && depthH > 0) {
    CreateDepthTexture(depthW, depthH);
  } else {
    fprintf(stderr,
            "[MetalDevice8] WARNING: Skipping depth texture (size 0x0)\n");
  }

  // Create default render target and depth stencil surfaces
  // The engine's DX8Wrapper stores these to pass back to SetRenderTarget.
  UINT surfW = (UINT)m_ScreenWidth;
  UINT surfH = (UINT)m_ScreenHeight;
  if (surfW == 0)
    surfW = 800;
  if (surfH == 0)
    surfH = 600;

  m_DefaultRTSurface = W3DNEW MetalSurface8(this, MetalSurface8::kColor, surfW,
                                            surfH, D3DFMT_A8R8G8B8);
  m_DefaultDepthSurface = W3DNEW MetalSurface8(this, MetalSurface8::kDepth,
                                               surfW, surfH, D3DFMT_D24S8);
  fprintf(stderr,
          "[MetalDevice8] Default surfaces created: RT %ux%u, DS %ux%u\n",
          surfW, surfH, surfW, surfH);

  return true;
}

// ─────────────────────────────────────────────────────
//  Depth Buffer Helpers
// ─────────────────────────────────────────────────────

void MetalDevice8::CreateDepthTexture(UINT width, UINT height) {
  // Release old depth texture if any
  if (m_DepthTexture) {
    CFRelease(m_DepthTexture);
    m_DepthTexture = nullptr;
  }

  // Release old MSAA textures if any
  if (m_MSAAColorTexture) {
    CFRelease(m_MSAAColorTexture);
    m_MSAAColorTexture = nullptr;
  }
  if (m_MSAADepthTexture) {
    CFRelease(m_MSAADepthTexture);
    m_MSAADepthTexture = nullptr;
  }

  // Also need to recreate PSOs since depthAttachmentPixelFormat changes
  for (auto &pair : m_PsoCache) {
    if (pair.second)
      CFRelease(pair.second);
  }
  m_PsoCache.clear();

  MTLTextureDescriptor *depthDesc = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8
                                   width:width
                                  height:height
                               mipmapped:NO];
  depthDesc.usage = MTLTextureUsageRenderTarget;
  depthDesc.storageMode = MTLStorageModePrivate;

  id<MTLTexture> depthTex = [MTL_DEVICE newTextureWithDescriptor:depthDesc];
  if (depthTex) {
    depthTex.label = @"MetalDevice8 DepthStencilBuffer";
    m_DepthTexture = (__bridge_retained void *)depthTex;
    fprintf(stderr,
            "[MetalDevice8] Depth+Stencil texture created: %u x %u "
            "(Depth32Float_Stencil8)\n",
            width, height);
  } else {
    fprintf(stderr,
            "[MetalDevice8] ERROR: Failed to create depth texture %u x %u\n",
            width, height);
  }

  // --- Create MSAA textures ---
  if (m_MSAASampleCount > 1) {
    // MSAA Color texture
    MTLTextureDescriptor *msaaColorDesc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                     width:width
                                    height:height
                                 mipmapped:NO];
    msaaColorDesc.textureType = MTLTextureType2DMultisample;
    msaaColorDesc.sampleCount = m_MSAASampleCount;
    msaaColorDesc.storageMode = MTLStorageModePrivate;
    msaaColorDesc.usage = MTLTextureUsageRenderTarget;

    id<MTLTexture> msaaColor = [MTL_DEVICE newTextureWithDescriptor:msaaColorDesc];
    if (msaaColor) {
      msaaColor.label = @"MetalDevice8 MSAA Color";
      m_MSAAColorTexture = (__bridge_retained void *)msaaColor;
    }

    // MSAA Depth+Stencil texture
    MTLTextureDescriptor *msaaDepthDesc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8
                                     width:width
                                    height:height
                                 mipmapped:NO];
    msaaDepthDesc.textureType = MTLTextureType2DMultisample;
    msaaDepthDesc.sampleCount = m_MSAASampleCount;
    msaaDepthDesc.storageMode = MTLStorageModePrivate;
    msaaDepthDesc.usage = MTLTextureUsageRenderTarget;

    id<MTLTexture> msaaDepth = [MTL_DEVICE newTextureWithDescriptor:msaaDepthDesc];
    if (msaaDepth) {
      msaaDepth.label = @"MetalDevice8 MSAA Depth+Stencil";
      m_MSAADepthTexture = (__bridge_retained void *)msaaDepth;
    }

    fprintf(stderr, "[MetalDevice8] MSAA %dx textures created: %u x %u\n",
            m_MSAASampleCount, width, height);
  }

  // Force depth stencil state recreation
  m_DepthStateDirty = true;
}

// MapD3DCmpToMTL() and MapD3DStencilOpToMTL() are now in MetalBridgeMappings.h

void *MetalDevice8::GetDepthStencilState() {
  if (!m_DepthStateDirty && m_DepthStencilState)
    return m_DepthStencilState;

  // Build cache key from relevant render states (depth + stencil)
  DWORD zEnable = m_RenderStates[D3DRS_ZENABLE];
  DWORD zWrite = m_RenderStates[D3DRS_ZWRITEENABLE];
  DWORD zFunc = m_RenderStates[D3DRS_ZFUNC];
  DWORD stencilEn = m_RenderStates[D3DRS_STENCILENABLE];
  DWORD stencilFunc = m_RenderStates[D3DRS_STENCILFUNC];
  DWORD stencilFail = m_RenderStates[D3DRS_STENCILFAIL];
  DWORD stencilZFail = m_RenderStates[D3DRS_STENCILZFAIL];
  DWORD stencilPass = m_RenderStates[D3DRS_STENCILPASS];

  // 64-bit key: depth bits (low 6) + stencil bits (7..31)
  uint64_t key = (zEnable & 1) | ((zWrite & 1) << 1) | ((zFunc & 0xF) << 2) |
                 ((stencilEn & 1ULL) << 6) | ((stencilFunc & 0xFULL) << 7) |
                 ((stencilFail & 0xFULL) << 11) |
                 ((stencilZFail & 0xFULL) << 15) |
                 ((stencilPass & 0xFULL) << 19);

  auto it = m_DepthStencilStateCache.find((uint32_t)key);
  if (it != m_DepthStencilStateCache.end()) {
    m_DepthStencilState = it->second;
    m_DepthStateDirty = false;
    return m_DepthStencilState;
  }

  MTLDepthStencilDescriptor *dsd = [[MTLDepthStencilDescriptor alloc] init];
  if (zEnable) {
    dsd.depthCompareFunction = MapD3DCmpToMTL(zFunc);
    dsd.depthWriteEnabled = (zWrite != 0);
  } else {
    dsd.depthCompareFunction = MTLCompareFunctionAlways;
    dsd.depthWriteEnabled = NO;
  }

  // Stencil configuration
  if (stencilEn) {
    DWORD readMask = m_RenderStates[D3DRS_STENCILMASK];
    DWORD writeMask = m_RenderStates[D3DRS_STENCILWRITEMASK];

    MTLStencilDescriptor *stencilDesc = [[MTLStencilDescriptor alloc] init];
    stencilDesc.stencilCompareFunction = MapD3DCmpToMTL(stencilFunc);
    stencilDesc.stencilFailureOperation = MapD3DStencilOpToMTL(stencilFail);
    stencilDesc.depthFailureOperation = MapD3DStencilOpToMTL(stencilZFail);
    stencilDesc.depthStencilPassOperation = MapD3DStencilOpToMTL(stencilPass);
    stencilDesc.readMask = readMask & 0xFF;
    stencilDesc.writeMask = writeMask & 0xFF;

    // DX8 doesn't have separate front/back stencil (that's DX9+)
    dsd.frontFaceStencil = stencilDesc;
    dsd.backFaceStencil = stencilDesc;
  }

  id<MTLDepthStencilState> dss =
      [MTL_DEVICE newDepthStencilStateWithDescriptor:dsd];
  if (dss) {
    m_DepthStencilStateCache[(uint32_t)key] = (__bridge_retained void *)dss;
    m_DepthStencilState = (__bridge void *)dss;
  }

  m_DepthStateDirty = false;
  return m_DepthStencilState;
}

// ─────────────────────────────────────────────────────
//  Stage 6: D3DBLEND → MTLBlendFactor mapping
//  Spec: d3d8_stub.h D3DBLEND enum
// ─────────────────────────────────────────────────────
// MapD3DBlendToMTL() and MapD3DCullToMTL() are now in MetalBridgeMappings.h

// ─────────────────────────────────────────────────────
//  Stage 6: Build 64-bit PSO cache key
//  Layout:  [FVF 20 bits | blendEn 1 | srcBlend 4 | dstBlend 4 | cwMask 4 |
//            srcAlpha 4 | dstAlpha// Build a unique key from FVF, blend state, and stride
uint64_t MetalDevice8::BuildPSOKey(DWORD fvf, UINT stride) {
  uint64_t key = fvf;
  
  // Blend state bits (approx 16 bits)
  DWORD blendEn = m_RenderStates[D3DRS_ALPHABLENDENABLE] ? 1 : 0;
  DWORD srcBlend = m_RenderStates[D3DRS_SRCBLEND] & 0x1F;
  DWORD dstBlend = m_RenderStates[D3DRS_DESTBLEND] & 0x1F;
  DWORD dwAlphaEn = m_RenderStates[D3DRS_ALPHATESTENABLE] ? 1 : 0;
  DWORD cwMask = m_RenderStates[D3DRS_COLORWRITEENABLE] & 0xF;
  if (cwMask == 0) cwMask = 0xF;

  // TheSuperHackers @fix macOS: Protect destination alpha from accidental overwrites.
  // On DX8 with X8R8G8B8 backbuffer, cwMask=0xF doesn't write alpha (X channel).
  // On Metal (BGRA8), cwMask=0xF DOES write alpha, which destroys the shoreline
  // alpha gradient used by water DESTALPHA blending.
  // Strip alpha from "write all" mask when rendering to main framebuffer.
  // Only explicit alpha-only writes (cwMask=0x8 from renderShoreLinesSorted) pass through.
  if (!m_RTTColorTexture && cwMask == 0xF) {
    cwMask = 0x7; // RGB only, preserve destination alpha
  }
  
  key |= (uint64_t)(blendEn) << 32;
  key |= (uint64_t)(srcBlend) << 33;
  key |= (uint64_t)(dstBlend) << 38;
  key |= (uint64_t)(cwMask) << 43;
  key |= (uint64_t)(dwAlphaEn) << 47;
  key |= (uint64_t)(stride) << 48; // Up to 65535 stride
  
  // RTT depth availability: PSO must match render pass depth attachment
  bool hasDepth = false;
  if (m_RTTColorTexture) {
    hasDepth = (m_RTTDepthTexture != nullptr);
  } else {
    hasDepth = (m_DepthTexture != nullptr);
  }
  if (!hasDepth) {
    key |= (uint64_t)1 << 63; // mark PSOs without depth
  }

  // MSAA: PSO sampleCount must match render target
  int sc = m_RTTColorTexture ? 1 : m_MSAASampleCount;
  key |= (uint64_t)(sc & 0x7) << 60;

  return key;
}

// ─────────────────────────────────────────────────────
//  Stage 6: Apply per-draw encoder state (cull, depth)
// ─────────────────────────────────────────────────────
void MetalDevice8::ApplyPerDrawState() {
  if (!m_CurrentEncoder)
    return;

  DWORD cullMode = m_RenderStates[D3DRS_CULLMODE];
  if (cullMode != m_LastAppliedCull) {
    [MTL_ENCODER setCullMode:MTLCullModeNone]; // FORCE NO CULLING
    [MTL_ENCODER setFrontFacingWinding:MTLWindingClockwise];
    m_LastAppliedCull = cullMode;
  }

  bool hasDepth = false;
  if (m_RTTColorTexture) {
    hasDepth = (m_RTTDepthTexture != nullptr);
  } else {
    hasDepth = (m_DepthTexture != nullptr);
  }

  if (!hasDepth)
    return;

  if (m_DepthStateDirty) {
    void *dss = GetDepthStencilState();
    if (dss) {
      [MTL_ENCODER setDepthStencilState:(__bridge id<MTLDepthStencilState>)dss];
      if (m_RenderStates[D3DRS_STENCILENABLE]) {
        [MTL_ENCODER setStencilReferenceValue:
                         (uint32_t)(m_RenderStates[D3DRS_STENCILREF] & 0xFF)];
      }
    }
  }

  DWORD zBias = m_RenderStates[D3DRS_ZBIAS];
  if (zBias != m_LastAppliedZBias) {
    if (zBias != 0) {
      float bias = -(float)zBias;
      float slopeScale = -2.0f;
      [MTL_ENCODER setDepthBias:bias slopeScale:slopeScale clamp:0.0f];
    } else {
      [MTL_ENCODER setDepthBias:0.0f slopeScale:0.0f clamp:0.0f];
    }
    m_LastAppliedZBias = zBias;
  }
}

void MetalDevice8::BindUniforms(DWORD fvf) {
  MetalUniforms u;
  memcpy(&u.world, &m_Transforms[D3DTS_WORLD], 64);
  memcpy(&u.view, &m_Transforms[D3DTS_VIEW], 64);
  memcpy(&u.projection, &m_Transforms[D3DTS_PROJECTION], 64);
  u.screenSize.x = m_ScreenWidth;
  u.screenSize.y = m_ScreenHeight;
  u.useProjection = (fvf & D3DFVF_XYZRHW) ? 2 : 1;
  u.shaderSettings = 0;

  // DIAG: dump matrices every 60th present frame
  extern int g_metalPresentCount;
  if (g_metalPresentCount % 120 == 0) {
    const float* w = (const float*)&m_Transforms[D3DTS_WORLD];
    const float* v = (const float*)&m_Transforms[D3DTS_VIEW];
    const float* p = (const float*)&m_Transforms[D3DTS_PROJECTION];
    printf("[DIAG] BindUniforms fvf=0x%x useProj=%d frame=%d\n", (unsigned)fvf, u.useProjection, g_metalPresentCount);
    printf("[DIAG]   World: [%.3f,%.3f,%.3f,%.3f | %.3f,%.3f,%.3f,%.3f | %.3f,%.3f,%.3f,%.3f | %.3f,%.3f,%.3f,%.3f]\n",
           w[0],w[1],w[2],w[3], w[4],w[5],w[6],w[7], w[8],w[9],w[10],w[11], w[12],w[13],w[14],w[15]);
    printf("[DIAG]   View:  [%.3f,%.3f,%.3f,%.3f | %.3f,%.3f,%.3f,%.3f | %.3f,%.3f,%.3f,%.3f | %.3f,%.3f,%.3f,%.3f]\n",
           v[0],v[1],v[2],v[3], v[4],v[5],v[6],v[7], v[8],v[9],v[10],v[11], v[12],v[13],v[14],v[15]);
    printf("[DIAG]   Proj:  [%.3f,%.3f,%.3f,%.3f | %.3f,%.3f,%.3f,%.3f | %.3f,%.3f,%.3f,%.3f | %.3f,%.3f,%.3f,%.3f]\n",
           p[0],p[1],p[2],p[3], p[4],p[5],p[6],p[7], p[8],p[9],p[10],p[11], p[12],p[13],p[14],p[15]);
    fflush(stdout);
  }
  for (int s = 0; s < 4; ++s) {
    memcpy(&u.texMatrix[s], &m_Transforms[D3DTS_TEXTURE0 + s], 64);
    u.texTransformFlags[s] = m_TextureStageStates[s][D3DTSS_TEXTURETRANSFORMFLAGS];
  }
  [MTL_ENCODER setVertexBytes:&u length:sizeof(u) atIndex:1];
  [MTL_ENCODER setFragmentBytes:&u length:sizeof(u) atIndex:1];

  FragmentUniforms fu;
  memset(&fu, 0, sizeof(fu));
  for (int s = 0; s < 4; s++) {
    fu.stages[s].colorOp = m_TextureStageStates[s][D3DTSS_COLOROP];
    fu.stages[s].colorArg1 = m_TextureStageStates[s][D3DTSS_COLORARG1];
    fu.stages[s].colorArg2 = m_TextureStageStates[s][D3DTSS_COLORARG2];
    fu.stages[s].alphaOp = m_TextureStageStates[s][D3DTSS_ALPHAOP];
    fu.stages[s].alphaArg1 = m_TextureStageStates[s][D3DTSS_ALPHAARG1];
    fu.stages[s].alphaArg2 = m_TextureStageStates[s][D3DTSS_ALPHAARG2];
    fu.stages[s].colorArg0 = m_TextureStageStates[s][D3DTSS_COLORARG0];
  }
  DWORD tf = m_RenderStates[D3DRS_TEXTUREFACTOR];
  fu.textureFactor.x = ((tf >> 16) & 0xFF) / 255.0f;
  fu.textureFactor.y = ((tf >> 8) & 0xFF) / 255.0f;
  fu.textureFactor.z = ((tf >> 0) & 0xFF) / 255.0f;
  fu.textureFactor.w = ((tf >> 24) & 0xFF) / 255.0f;
  fu.alphaTestEnable = m_RenderStates[D3DRS_ALPHATESTENABLE];
  fu.alphaFunc = m_RenderStates[D3DRS_ALPHAFUNC];
  fu.alphaRef = m_RenderStates[D3DRS_ALPHAREF] / 255.0f;
  {
    DWORD fogEnable = m_RenderStates[D3DRS_FOGENABLE];
    if (fogEnable) {
      uint32_t mode = m_RenderStates[D3DRS_FOGTABLEMODE];
      if (mode == D3DFOG_NONE)
        mode = m_RenderStates[D3DRS_FOGVERTEXMODE];
      fu.fogMode = mode;
    } else {
      fu.fogMode = 0;
    }
    DWORD fc = m_RenderStates[D3DRS_FOGCOLOR];
    fu.fogColor =
        simd::float4{((fc >> 16) & 0xFF) / 255.0f, ((fc >> 8) & 0xFF) / 255.0f,
                     ((fc >> 0) & 0xFF) / 255.0f, ((fc >> 24) & 0xFF) / 255.0f};
    memcpy(&fu.fogStart, &m_RenderStates[D3DRS_FOGSTART], 4);
    memcpy(&fu.fogEnd, &m_RenderStates[D3DRS_FOGEND], 4);
    memcpy(&fu.fogDensity, &m_RenderStates[D3DRS_FOGDENSITY], 4);
  }
  for (int s = 0; s < 4; ++s) {
    fu.hasTexture[s] = (m_Textures[s] != nullptr) ? 1 : 0;
  }
  // DIAG: dump TSS for first draw each frame
  if (g_metalPresentCount % 120 == 0) {
    printf("[DIAG] TSS frame=%d: s0[cOp=%u cA1=0x%x cA2=0x%x aOp=%u hasTex=%u] s1[cOp=%u hasTex=%u]\n",
           g_metalPresentCount,
           fu.stages[0].colorOp, fu.stages[0].colorArg1, fu.stages[0].colorArg2,
           fu.stages[0].alphaOp, fu.hasTexture[0],
           fu.stages[1].colorOp, fu.hasTexture[1]);
    printf("[DIAG] TSS textures: [%p, %p, %p, %p]\n",
           m_Textures[0], m_Textures[1], m_Textures[2], m_Textures[3]);
    fflush(stdout);
  }
  fu.specularEnable = m_RenderStates[D3DRS_SPECULARENABLE];
  fu.blendEnabled = m_RenderStates[D3DRS_ALPHABLENDENABLE] ? 1 : 0;
  for (int s = 0; s < 4; ++s) {
    fu.texCoordIndex[s] = m_TextureStageStates[s][D3DTSS_TEXCOORDINDEX];
    fu.texFormatType[s] = 0;
    if (m_Textures[s]) {
      D3DFORMAT fmt = ((MetalTexture8 *)m_Textures[s])->GetD3DFormat();
      if (fmt == D3DFMT_L8 || fmt == D3DFMT_P8) {
        fu.texFormatType[s] = 1;
      } else if (fmt == D3DFMT_A8L8 || fmt == D3DFMT_A4L4 || fmt == D3DFMT_A8P8) {
        fu.texFormatType[s] = 2;
      } else if (fmt == D3DFMT_DXT1) {
        fu.texFormatType[s] = 3;
      }
    }
  }
  [MTL_ENCODER setFragmentBytes:&fu length:sizeof(fu) atIndex:2];

  LightingUniforms lu;
  memset(&lu, 0, sizeof(lu));
  for (int i = 0; i < MAX_LIGHTS; i++) {
    lu.lights[i].enabled = m_LightEnabled[i] ? 1 : 0;
    if (m_LightEnabled[i]) {
      const D3DLIGHT8 &l = m_Lights[i];
      lu.lights[i].type = (uint32_t)l.Type;
      lu.lights[i].diffuse =
          simd::float4{l.Diffuse.r, l.Diffuse.g, l.Diffuse.b, l.Diffuse.a};
      lu.lights[i].ambient =
          simd::float4{l.Ambient.r, l.Ambient.g, l.Ambient.b, l.Ambient.a};
      lu.lights[i].specular =
          simd::float4{l.Specular.r, l.Specular.g, l.Specular.b, l.Specular.a};
      lu.lights[i].position =
          simd::float3{l.Position.x, l.Position.y, l.Position.z};
      lu.lights[i].direction =
          simd::float3{l.Direction.x, l.Direction.y, l.Direction.z};
      lu.lights[i].range = l.Range;
      lu.lights[i].falloff = l.Falloff;
      lu.lights[i].attenuation0 = l.Attenuation0;
      lu.lights[i].attenuation1 = l.Attenuation1;
      lu.lights[i].attenuation2 = l.Attenuation2;
      lu.lights[i].theta = l.Theta;
      lu.lights[i].phi = l.Phi;
    }
  }
  lu.materialDiffuse = simd::float4{m_Material.Diffuse.r, m_Material.Diffuse.g,
                                    m_Material.Diffuse.b, m_Material.Diffuse.a};
  lu.materialAmbient = simd::float4{m_Material.Ambient.r, m_Material.Ambient.g,
                                    m_Material.Ambient.b, m_Material.Ambient.a};
  lu.materialSpecular =
      simd::float4{m_Material.Specular.r, m_Material.Specular.g,
                   m_Material.Specular.b, m_Material.Specular.a};
  lu.materialEmissive =
      simd::float4{m_Material.Emissive.r, m_Material.Emissive.g,
                   m_Material.Emissive.b, m_Material.Emissive.a};
  lu.materialPower = m_Material.Power;
  DWORD ga = m_RenderStates[D3DRS_AMBIENT];
  lu.globalAmbient =
      simd::float4{((ga >> 16) & 0xFF) / 255.0f, ((ga >> 8) & 0xFF) / 255.0f,
                   ((ga >> 0) & 0xFF) / 255.0f, ((ga >> 24) & 0xFF) / 255.0f};
  lu.lightingEnabled = m_RenderStates[D3DRS_LIGHTING];
  lu.diffuseSource = m_RenderStates[D3DRS_DIFFUSEMATERIALSOURCE];
  lu.ambientSource = m_RenderStates[D3DRS_AMBIENTMATERIALSOURCE];
  lu.specularSource = m_RenderStates[D3DRS_SPECULARMATERIALSOURCE];
  lu.emissiveSource = m_RenderStates[D3DRS_EMISSIVEMATERIALSOURCE];
  lu.hasNormals = (fvf & D3DFVF_NORMAL) ? 1 : 0;
  {
    DWORD fogEnable = m_RenderStates[D3DRS_FOGENABLE];
    if (fogEnable) {
      uint32_t mode = m_RenderStates[D3DRS_FOGTABLEMODE];
      if (mode == D3DFOG_NONE)
        mode = m_RenderStates[D3DRS_FOGVERTEXMODE];
      lu.fogMode = mode;
    } else {
      lu.fogMode = 0;
    }
    memcpy(&lu.fogStart, &m_RenderStates[D3DRS_FOGSTART], 4);
    memcpy(&lu.fogEnd, &m_RenderStates[D3DRS_FOGEND], 4);
    memcpy(&lu.fogDensity, &m_RenderStates[D3DRS_FOGDENSITY], 4);
  }
  [MTL_ENCODER setVertexBytes:&lu length:sizeof(lu) atIndex:3];
}

void MetalDevice8::BindCustomVSUniforms() {
  CustomVSUniforms cvu;
  memset(&cvu, 0, sizeof(cvu));
  if (m_VertexShader & 0x80000000) {
    auto it = m_VSHandleMap.find(m_VertexShader);
    if (it != m_VSHandleMap.end()) {
      cvu.shaderType = it->second.shaderType;
    }
    for (int r = 0; r < 34; ++r) {
      cvu.c[r] = simd::float4{m_VSConstants[r][0], m_VSConstants[r][1],
                              m_VSConstants[r][2], m_VSConstants[r][3]};
    }
  }
  [MTL_ENCODER setVertexBytes:&cvu length:sizeof(cvu) atIndex:4];

  struct {
    uint32_t psType;
    uint32_t _pad[3];
    simd::float4 c[8];
  } psu;
  memset(&psu, 0, sizeof(psu));
  if (m_PixelShader != 0) {
    auto it = m_PSHandleMap.find(m_PixelShader);
    if (it != m_PSHandleMap.end()) {
      psu.psType = it->second.psType;
    }
    for (int r = 0; r < MAX_PS_CONSTANTS; ++r) {
      psu.c[r] = simd::float4{m_PSConstants[r][0], m_PSConstants[r][1],
                              m_PSConstants[r][2], m_PSConstants[r][3]};
    }
  }
  [MTL_ENCODER setFragmentBytes:&psu length:sizeof(psu) atIndex:5];
}

void MetalDevice8::BindTexturesAndSamplers() {
  for (int s = 0; s < 4; s++) {
    if (m_Textures[s]) {
      MetalTexture8 *tex = (MetalTexture8 *)m_Textures[s];
      id<MTLTexture> mtlTex = tex->GetMTLTexture();
      if (mtlTex) {
        [MTL_ENCODER setFragmentTexture:mtlTex atIndex:s];
      }
    }
    void *samplerState = GetSamplerState(s);
    if (samplerState) {
      [MTL_ENCODER
          setFragmentSamplerState:(__bridge id<MTLSamplerState>)samplerState
                          atIndex:s];
    }
  }
}

unsigned long MetalDevice8::MapPrimitiveType(DWORD d3dPrimType) {
  switch (d3dPrimType) {
    case D3DPT_TRIANGLELIST:  return MTLPrimitiveTypeTriangle;
    case D3DPT_TRIANGLESTRIP: return MTLPrimitiveTypeTriangleStrip;
    case D3DPT_LINELIST:      return MTLPrimitiveTypeLine;
    case D3DPT_LINESTRIP:     return MTLPrimitiveTypeLineStrip;
    case D3DPT_POINTLIST:     return MTLPrimitiveTypePoint;
    default:                  return MTLPrimitiveTypeTriangle;
  }
}

// ─────────────────────────────────────────────────────
//  Stage 7: Get or Create MTLSamplerState for a texture stage
// ─────────────────────────────────────────────────────
// MapD3DAddressToMTL(), MapD3DFilterToMTL(), MapD3DMipFilterToMTL() are now in MetalBridgeMappings.h

void *MetalDevice8::GetSamplerState(DWORD stage) {
  if (stage >= MAX_TEXTURE_STAGES)
    return nullptr;

  DWORD addrU = m_TextureStageStates[stage][D3DTSS_ADDRESSU];
  DWORD addrV = m_TextureStageStates[stage][D3DTSS_ADDRESSV];
  DWORD magF = m_TextureStageStates[stage][D3DTSS_MAGFILTER];
  DWORD minF = m_TextureStageStates[stage][D3DTSS_MINFILTER];
  DWORD mipF = m_TextureStageStates[stage][D3DTSS_MIPFILTER];

  // Build key: addrU(3) | addrV(3) | mag(3) | min(3) | mip(3) = 15 bits
  uint32_t key = (addrU & 0x7) | ((addrV & 0x7) << 3) | ((magF & 0x7) << 6) |
                 ((minF & 0x7) << 9) | ((mipF & 0x7) << 12);

  auto it = m_SamplerStateCache.find(key);
  if (it != m_SamplerStateCache.end())
    return it->second;

  MTLSamplerDescriptor *sd = [[MTLSamplerDescriptor alloc] init];
  sd.sAddressMode = MapD3DAddressToMTL(addrU);
  sd.tAddressMode = MapD3DAddressToMTL(addrV);
  sd.magFilter = MapD3DFilterToMTL(magF);
  sd.minFilter = MapD3DFilterToMTL(minF);
  sd.mipFilter = MapD3DMipFilterToMTL(mipF);

  id<MTLSamplerState> sampler = [MTL_DEVICE newSamplerStateWithDescriptor:sd];
  if (sampler) {
    m_SamplerStateCache[key] = (__bridge_retained void *)sampler;
    return (__bridge void *)sampler;
  }
  return nullptr;
}


// ─────────────────────────────────────────────────────
//  IUnknown
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::QueryInterface(REFIID riid, void **ppvObj) {
  if (ppvObj)
    *ppvObj = nullptr;
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) MetalDevice8::Release() {
  ULONG r = --m_RefCount;
  if (r == 0) {
    delete this;
    return 0;
  }
  return r;
}

// ─────────────────────────────────────────────────────
//  Device Status
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::TestCooperativeLevel() { return D3D_OK; }
STDMETHODIMP_(UINT) MetalDevice8::GetAvailableTextureMem() {
  return 512 * 1024 * 1024;
}
STDMETHODIMP MetalDevice8::ResourceManagerDiscardBytes(DWORD Bytes) {
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::GetAdapterIdentifier(UINT a, DWORD f,
                                                D3DADAPTER_IDENTIFIER8 *i) {
  if (!i)
    return E_POINTER;
  memset(i, 0, sizeof(*i));

  // TheSuperHackers @feature macOS: Emulate GeForce4 Ti 4600 — the best
  // consumer GPU of the Generals era. This enables all engine rendering
  // features and triggers known-good Vendor_Specific_Hacks for NVIDIA.
  strncpy(i->Description, "Apple Metal (GeForce4 Ti 4600 emulated)",
          sizeof(i->Description) - 1);
  strncpy(i->Driver, "metal.dll", sizeof(i->Driver) - 1);
  i->VendorId = 0x10DE;  // NVIDIA
  i->DeviceId = 0x0250;  // GeForce4 Ti 4600
  i->SubSysId = 0;
  i->Revision = 1;
  i->DriverVersion.HighPart = (1 << 16) | 0;  // Product=1, Version=0
  i->DriverVersion.LowPart  = (53 << 16) | 3;  // SubVersion=53, Build=3

  return D3D_OK;
}

STDMETHODIMP MetalDevice8::GetDeviceCaps(D3DCAPS8 *pCaps) {
  if (!pCaps)
    return E_POINTER;
  memset(pCaps, 0, sizeof(*pCaps));
  pCaps->DeviceType = D3DDEVTYPE_HAL;
  pCaps->DevCaps = D3DDEVCAPS_HWTRANSFORMANDLIGHT;
  pCaps->MaxSimultaneousTextures = 8;
  pCaps->MaxTextureBlendStages = 8;
  pCaps->VertexShaderVersion = 0x0101;
  pCaps->PixelShaderVersion = 0x0101;
  pCaps->MaxPrimitiveCount = 0xFFFFFF;
  pCaps->MaxVertexIndex = 0xFFFFFF;
  pCaps->MaxStreams = 8;
  pCaps->MaxActiveLights = 4;
  pCaps->MaxTextureWidth = 4096;
  pCaps->MaxTextureHeight = 4096;
  pCaps->RasterCaps =
      D3DPRASTERCAPS_FOGRANGE | 0x00000100 | 0x00000200 | D3DPRASTERCAPS_ZBIAS;
  pCaps->TextureCaps = 0x00000001 | 0x00000002 | 0x00000004;
  pCaps->TextureOpCaps =
      D3DTEXOPCAPS_DISABLE | D3DTEXOPCAPS_SELECTARG1 | D3DTEXOPCAPS_SELECTARG2 |
      D3DTEXOPCAPS_MODULATE | D3DTEXOPCAPS_MODULATE2X | D3DTEXOPCAPS_MODULATE4X |
      D3DTEXOPCAPS_ADD | D3DTEXOPCAPS_ADDSIGNED | D3DTEXOPCAPS_ADDSIGNED2X |
      D3DTEXOPCAPS_SUBTRACT | D3DTEXOPCAPS_ADDSMOOTH |
      D3DTEXOPCAPS_BLENDDIFFUSEALPHA | D3DTEXOPCAPS_BLENDTEXTUREALPHA |
      D3DTEXOPCAPS_BLENDFACTORALPHA | D3DTEXOPCAPS_BLENDCURRENTALPHA |
      D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR | D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA |
      D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR | D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA |
      D3DTEXOPCAPS_DOTPRODUCT3 | D3DTEXOPCAPS_MULTIPLYADD | D3DTEXOPCAPS_LERP;
  pCaps->PrimitiveMiscCaps = D3DPMISCCAPS_COLORWRITEENABLE;
  pCaps->Caps2 = D3DCAPS2_FULLSCREENGAMMA;
  pCaps->SrcBlendCaps = 0x1FFF;
  pCaps->DestBlendCaps = 0x1FFF;
  pCaps->ZCmpCaps = 0xFF;
  pCaps->AlphaCmpCaps = 0xFF;
  pCaps->StencilCaps = 0xFF;
  // TextureFilterCaps: lets _Init_Filters set FILTER_TYPE_BEST=LINEAR.
  // FILTER_TYPE_DEFAULT is then overridden back to POINT in texturefilter.cpp (#ifdef __APPLE__)
  // to prevent DXT1 BC1-block boundary artifacts on UI buttons drawn via Render2DClass.
  pCaps->TextureFilterCaps =
      D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MINFLINEAR |
      D3DPTFILTERCAPS_MINFANISOTROPIC |
      D3DPTFILTERCAPS_MAGFPOINT | D3DPTFILTERCAPS_MAGFLINEAR |
      D3DPTFILTERCAPS_MIPFPOINT | D3DPTFILTERCAPS_MIPFLINEAR;
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::GetDisplayMode(D3DDISPLAYMODE *pMode) {
  if (!pMode)
    return E_POINTER;
  pMode->Width = (UINT)m_ScreenWidth;
  pMode->Height = (UINT)m_ScreenHeight;
  pMode->RefreshRate = 60;
  pMode->Format = D3DFMT_A8R8G8B8;
  return D3D_OK;
}

// ─────────────────────────────────────────────────────
//  Swap Chain / Present
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS *p,
                                                     IDirect3DSwapChain8 **s) {
  // Metal uses a single CAMetalLayer; additional swap chains not supported.
  if (s) *s = nullptr;
  return D3DERR_NOTAVAILABLE;
}

STDMETHODIMP MetalDevice8::Reset(D3DPRESENT_PARAMETERS *p) { return D3D_OK; }

int g_metalPresentCount = 0;
int g_metalDrawCallsThisFrame = 0;

STDMETHODIMP MetalDevice8::Present(const void *s, const void *d, HWND w,
                                   const void *r) {
  printf("[DIAG] Present frame=%d drawable=%p cmdBuf=%p encoder=%p drawCalls=%d\n",
         g_metalPresentCount, m_CurrentDrawable, m_CurrentCommandBuffer, m_CurrentEncoder, g_metalDrawCallsThisFrame);
  fflush(stdout);
  g_metalDrawCallsThisFrame = 0;
  if (m_CurrentEncoder) {
    [MTL_ENCODER endEncoding];
    CLEAR_MTL(CurrentEncoder);
  }
  if (m_CurrentDrawable && m_CurrentCommandBuffer) {
    [MTL_CMD_BUF presentDrawable:MTL_DRAWABLE];
  }
  if (m_CurrentCommandBuffer) {
    [MTL_CMD_BUF commit];
    // Wait for GPU to finish — matches DirectX 8's Present() which blocked
    // until VSync. Without this, CPU races ahead causing resource conflicts.
    // displaySyncEnabled=YES on CAMetalLayer handles the actual frame rate cap.
    [MTL_CMD_BUF waitUntilCompleted];

    // DIAG: Read back center pixel to verify GPU actually rendered something
    if (m_CurrentDrawable && (g_metalPresentCount % 60 == 0)) {
      id<MTLTexture> tex = MTL_DRAWABLE.texture;
      if (tex && tex.width > 0 && tex.height > 0) {
        uint8_t pixel[4] = {0};
        NSUInteger cx = tex.width / 2;
        NSUInteger cy = tex.height / 2;
        [tex getBytes:pixel bytesPerRow:tex.width*4 fromRegion:MTLRegionMake2D(cx, cy, 1, 1) mipmapLevel:0];
        printf("[DIAG] Present frame=%d CENTER_PIXEL BGRA=[%u,%u,%u,%u] tex=%lux%lu\n",
               g_metalPresentCount, pixel[0], pixel[1], pixel[2], pixel[3],
               (unsigned long)tex.width, (unsigned long)tex.height);
        // Also sample corners
        uint8_t tl[4]={0}, br[4]={0};
        [tex getBytes:tl bytesPerRow:tex.width*4 fromRegion:MTLRegionMake2D(0, 0, 1, 1) mipmapLevel:0];
        [tex getBytes:br bytesPerRow:tex.width*4 fromRegion:MTLRegionMake2D(tex.width-1, tex.height-1, 1, 1) mipmapLevel:0];
        printf("[DIAG] Present frame=%d TL=[%u,%u,%u,%u] BR=[%u,%u,%u,%u]\n",
               g_metalPresentCount, tl[0], tl[1], tl[2], tl[3], br[0], br[1], br[2], br[3]);
        fflush(stdout);
      }
    }

    CLEAR_MTL(CurrentCommandBuffer);
  }
  CLEAR_MTL(CurrentDrawable);
  m_InScene = false;
  m_RingBufferOffset = 0;
  g_metalPresentCount++;
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::GetBackBuffer(UINT i, D3DBACKBUFFER_TYPE t,
                                         IDirect3DSurface8 **b) {
  if (!b)
    return E_POINTER;
  if (m_DefaultRTSurface) {
    m_DefaultRTSurface->AddRef();
    *b = m_DefaultRTSurface;
    return D3D_OK;
  }
  *b = nullptr;
  return D3DERR_NOTFOUND;
}

// ─────────────────────────────────────────────────────
//  Gamma
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::SetGammaRamp(DWORD f, const D3DGAMMARAMP *p) {
  if (!p) return D3D_OK;
  memcpy(&m_GammaRamp, p, sizeof(D3DGAMMARAMP));

  // Convert 16-bit ramp (0-65535) to float (0.0-1.0) for CoreGraphics
  CGGammaValue red[256], green[256], blue[256];
  for (int i = 0; i < 256; i++) {
    red[i]   = p->red[i]   / 65535.0f;
    green[i] = p->green[i] / 65535.0f;
    blue[i]  = p->blue[i]  / 65535.0f;
  }
  CGSetDisplayTransferByTable(CGMainDisplayID(), 256, red, green, blue);

  static bool logged = false;
  if (!logged) {
    printf("[MetalDevice8] SetGammaRamp applied (first call)\n");
    fflush(stdout);
    logged = true;
  }
  return D3D_OK;
}
STDMETHODIMP MetalDevice8::GetGammaRamp(D3DGAMMARAMP *p) {
  if (p) memcpy(p, &m_GammaRamp, sizeof(D3DGAMMARAMP));
  return D3D_OK;
}

// ─────────────────────────────────────────────────────
//  Cursor — no-ops (macOS uses NSCursor natively)
// ─────────────────────────────────────────────────────

STDMETHODIMP_(BOOL) MetalDevice8::ShowCursor(BOOL bShow) { return FALSE; }
STDMETHODIMP MetalDevice8::SetCursorProperties(UINT XHotSpot, UINT YHotSpot,
                                                IDirect3DSurface8 *pCursorBitmap) {
  return D3D_OK;
}
STDMETHODIMP_(void) MetalDevice8::SetCursorPosition(int X, int Y, DWORD Flags) {
  // no-op
}

// ─────────────────────────────────────────────────────
//  Resource Creation
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::CreateTexture(UINT w, UINT h, UINT l, DWORD u,
                                         D3DFORMAT f, D3DPOOL p,
                                         IDirect3DTexture8 **t) {
  if (!t)
    return E_POINTER;
  *t = W3DNEW MetalTexture8(this, w, h, l, u, f, p);
  
  static int s_createTexCount = 0;
  s_createTexCount++;
  // Get return address to identify caller
  void* ra = __builtin_return_address(0);
  void* ra2 = __builtin_return_address(1);
  fprintf(stderr, "[MetalDevice8::CreateTexture] #%d: %ux%u fmt=%u mips=%u pool=%u tex=%p caller=%p caller2=%p\n",
          s_createTexCount, w, h, (unsigned)f, l, (unsigned)p, (void*)*t, ra, ra2);
  
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::CreateVolumeTexture(UINT w, UINT h, UINT d, UINT l,
                                               DWORD u, D3DFORMAT f, D3DPOOL p,
                                               IDirect3DVolumeTexture8 **t) {
  // Volume textures not implemented — engine handles nullptr gracefully.
  if (t) *t = nullptr;
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::CreateCubeTexture(UINT s, UINT l, DWORD u,
                                             D3DFORMAT f, D3DPOOL p,
                                             IDirect3DCubeTexture8 **t) {
  // Cube textures not implemented — engine handles nullptr gracefully.
  if (t) *t = nullptr;
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::CreateVertexBuffer(UINT Length, DWORD Usage,
                                              DWORD FVF, D3DPOOL Pool,
                                              IDirect3DVertexBuffer8 **ppVB) {
  if (!ppVB)
    return E_POINTER;
  UINT vertexSize = D3DXGetFVFVertexSize(FVF);
  if (vertexSize == 0)
    vertexSize = 32;
  UINT count = Length / vertexSize;
  *ppVB = new MetalVertexBuffer8(FVF, (unsigned short)count, vertexSize);
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::CreateIndexBuffer(UINT Length, DWORD Usage,
                                             D3DFORMAT Format, D3DPOOL Pool,
                                             IDirect3DIndexBuffer8 **ppIB) {
  if (!ppIB)
    return E_POINTER;
  bool is32bit = (Format == D3DFMT_INDEX32);
  UINT count = Length / (is32bit ? 4 : 2);
  *ppIB = new MetalIndexBuffer8(count, is32bit);
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::CreateImageSurface(UINT w, UINT h, D3DFORMAT f,
                                              IDirect3DSurface8 **s) {
  if (!s)
    return E_POINTER;
  *s = W3DNEW MetalSurface8(this, MetalSurface8::kColor, w, h, f);
  return D3D_OK;
}

// ─────────────────────────────────────────────────────
//  Surface / Texture Operations
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::CopyRects(IDirect3DSurface8 *src, const void *sr,
                                     UINT c, IDirect3DSurface8 *dst,
                                     const void *dp) {
  if (!src || !dst) return E_FAIL;

  MetalSurface8 *srcSurf = (MetalSurface8 *)src;
  MetalSurface8 *dstSurf = (MetalSurface8 *)dst;

  // Get destination Metal texture
  MetalTexture8 *dstTex = dstSurf->GetParentTexture();
  MetalTexture8 *srcTex = srcSurf->GetParentTexture();

  // ── Case 1: GPU src → GPU dst (both have parent textures) ──
  if (srcTex && srcTex->HasBeenWritten() && srcTex->GetMTLTexture() &&
      dstTex && dstTex->GetMTLTexture()) {
    id<MTLTexture> mtlSrc = srcTex->GetMTLTexture();
    id<MTLTexture> mtlDst = dstTex->GetMTLTexture();
    void *queuePtr = m_CommandQueue;
    if (queuePtr) {
      id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)queuePtr;
      id<MTLCommandBuffer> cmdBuf = [queue commandBuffer];
      if (cmdBuf) {
        id<MTLBlitCommandEncoder> blit = [cmdBuf blitCommandEncoder];
        UINT copyW = std::min((UINT)mtlSrc.width, (UINT)mtlDst.width);
        UINT copyH = std::min((UINT)mtlSrc.height, (UINT)mtlDst.height);
        [blit copyFromTexture:mtlSrc sourceSlice:0 sourceLevel:0
              sourceOrigin:MTLOriginMake(0, 0, 0) sourceSize:MTLSizeMake(copyW, copyH, 1)
              toTexture:mtlDst destinationSlice:0 destinationLevel:0
              destinationOrigin:MTLOriginMake(0, 0, 0)];
        [blit endEncoding];
        [cmdBuf commit];
        [cmdBuf waitUntilCompleted];
      }
    }
    dstTex->MarkWritten();
    return D3D_OK;
  }

  // ── Case 2: GPU src → standalone dst (no parent texture) ──
  // Read back GPU texture data into dst surface's locked buffer.
  // This is used by Recolor_Texture_One_Time to copy texture data before remapping.
  if (srcTex && srcTex->HasBeenWritten() && srcTex->GetMTLTexture() && !dstTex) {
    id<MTLTexture> mtlSrc = srcTex->GetMTLTexture();
    UINT srcW = (UINT)mtlSrc.width;
    UINT srcH = (UINT)mtlSrc.height;
    UINT dstW = dstSurf->GetWidth();
    UINT dstH = dstSurf->GetHeight();
    UINT copyW = std::min(srcW, dstW);
    UINT copyH = std::min(srcH, dstH);

    D3DLOCKED_RECT dstLocked;
    HRESULT hr = dstSurf->LockRect(&dstLocked, nullptr, 0);
    if (FAILED(hr)) return hr;

    D3DFORMAT dstFmt = dstSurf->GetD3DFormat();
    UINT dstBpp = BytesPerPixelFromD3D(dstFmt);
    bool is16bit = Is16BitFormat(dstFmt);

    UINT mtlPitch = copyW * 4;
    void *tmpBuf = malloc(mtlPitch * copyH);
    if (tmpBuf) {
      MTLRegion region = MTLRegionMake2D(0, 0, copyW, copyH);
      [mtlSrc getBytes:tmpBuf bytesPerRow:mtlPitch fromRegion:region mipmapLevel:0];

      if (is16bit) {
        const uint32_t *src32 = (const uint32_t *)tmpBuf;
        uint8_t *dstRow = (uint8_t *)dstLocked.pBits;
        for (UINT y = 0; y < copyH; y++) {
          uint16_t *dst16 = (uint16_t *)dstRow;
          for (UINT x = 0; x < copyW; x++) {
            uint32_t px = src32[y * copyW + x];
            uint8_t B = (px >>  0) & 0xFF;
            uint8_t G = (px >>  8) & 0xFF;
            uint8_t R = (px >> 16) & 0xFF;
            uint8_t A = (px >> 24) & 0xFF;
            uint16_t out = 0;
            switch (dstFmt) {
            case D3DFMT_R5G6B5:
              out = ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | ((B & 0xF8) >> 3);
              break;
            case D3DFMT_X1R5G5B5:
              out = ((R & 0xF8) << 7) | ((G & 0xF8) << 2) | ((B & 0xF8) >> 3);
              break;
            case D3DFMT_A1R5G5B5:
              out = ((A >> 7) << 15) | ((R & 0xF8) << 7) | ((G & 0xF8) << 2) | ((B & 0xF8) >> 3);
              break;
            case D3DFMT_A4R4G4B4:
              out = ((A & 0xF0) << 8) | ((R & 0xF0) << 4) | (G & 0xF0) | ((B & 0xF0) >> 4);
              break;
            default:
              break;
            }
            dst16[x] = out;
          }
          dstRow += dstLocked.Pitch;
        }
      } else {
        const uint8_t *srcRow = (const uint8_t *)tmpBuf;
        uint8_t *dstRow = (uint8_t *)dstLocked.pBits;
        UINT rowBytes = copyW * dstBpp;
        if (dstBpp == 4) rowBytes = copyW * 4;
        for (UINT y = 0; y < copyH; y++) {
          memcpy(dstRow, srcRow, rowBytes);
          srcRow += mtlPitch;
          dstRow += dstLocked.Pitch;
        }
      }
      free(tmpBuf);
    }

    dstSurf->UnlockRect();
    return D3D_OK;
  }

  // ── Case 3 & 4: CPU src → GPU/standalone dst ──
  // Need destination GPU texture for upload path
  if (!dstTex) return E_FAIL;
  id<MTLTexture> mtlDst = dstTex->GetMTLTexture();
  if (!mtlDst) return E_FAIL;

  // Source data: the surface may have a persistent locked buffer
  // (W3DShroud's lock-once pattern), or we may need to lock it.
  const void *srcBits = srcSurf->GetLockedData();
  UINT srcPitch = srcSurf->GetLockedPitch();
  bool didLock = false;
  
  if (!srcBits) {
    // No persistent buffer — try locking
    D3DLOCKED_RECT srcLocked;
    HRESULT hr = srcSurf->LockRect(&srcLocked, nullptr, D3DLOCK_READONLY);
    if (FAILED(hr)) return hr;
    srcBits = srcLocked.pBits;
    srcPitch = srcLocked.Pitch;
    didLock = true;
  }

  const RECT *srcRects = (const RECT *)sr;
  const POINT *dstPoints = (const POINT *)dp;

  D3DFORMAT srcFmt = srcSurf->GetD3DFormat();
  UINT srcBpp = BytesPerPixelFromD3D(srcFmt);
  bool is16bit = Is16BitFormat(srcFmt);

  // If c == 0 and srcRects == nullptr: copy entire surface
  UINT numRects = (c == 0 && srcRects == nullptr) ? 1 : c;
  if (numRects == 0) numRects = 1;

  for (UINT i = 0; i < numRects; i++) {
    UINT srcX = 0, srcY = 0, copyW = 0, copyH = 0;
    UINT dstX = 0, dstY = 0;

    if (srcRects) {
      srcX = srcRects[i].left;
      srcY = srcRects[i].top;
      copyW = srcRects[i].right - srcRects[i].left;
      copyH = srcRects[i].bottom - srcRects[i].top;
    } else {
      // Get surface desc for full copy
      D3DSURFACE_DESC desc;
      srcSurf->GetDesc(&desc);
      copyW = desc.Width;
      copyH = desc.Height;
    }

    if (dstPoints) {
      dstX = dstPoints[i].x;
      dstY = dstPoints[i].y;
    }

    if (copyW == 0 || copyH == 0) continue;

    // Source data pointer offset by srcX, srcY
    const uint8_t *srcRow = (const uint8_t *)srcBits
                            + srcY * srcPitch
                            + srcX * srcBpp;

    if (is16bit) {
      // Convert 16-bit source to 32-bit BGRA8 and upload
      UINT dstPitch = copyW * 4;
      uint8_t *converted = (uint8_t *)malloc(dstPitch * copyH);
      if (converted) {
        for (UINT y = 0; y < copyH; y++) {
          const uint16_t *sp = (const uint16_t *)(srcRow + y * srcPitch);
          uint32_t *dpx = (uint32_t *)(converted + y * dstPitch);
          for (UINT x = 0; x < copyW; x++) {
            dpx[x] = ConvertPixel16to32(srcFmt, sp[x]);
          }
        }
        MTLRegion region = MTLRegionMake2D(dstX, dstY, copyW, copyH);
        [mtlDst replaceRegion:region
               mipmapLevel:0
                 withBytes:converted
               bytesPerRow:dstPitch];
        free(converted);
      }
    } else {
      // Direct copy for 32-bit formats
      MTLRegion region = MTLRegionMake2D(dstX, dstY, copyW, copyH);
      // Must provide contiguous data — copy row by row if srcPitch != copyW*bpp
      UINT dstPitch = copyW * srcBpp;
      if ((UINT)srcPitch == dstPitch) {
        [mtlDst replaceRegion:region
               mipmapLevel:0
                 withBytes:srcRow
               bytesPerRow:dstPitch];
      } else {
        uint8_t *tmp = (uint8_t *)malloc(dstPitch * copyH);
        if (tmp) {
          for (UINT y = 0; y < copyH; y++) {
            memcpy(tmp + y * dstPitch,
                   srcRow + y * srcPitch,
                   dstPitch);
          }
          [mtlDst replaceRegion:region
                 mipmapLevel:0
                   withBytes:tmp
                 bytesPerRow:dstPitch];
          free(tmp);
        }
      }
    }
  }

  if (didLock) {
    srcSurf->UnlockRect();
  }

  // Mark the destination texture as written
  dstTex->MarkWritten();

  static int s_copyRectsLog = 0;
  if (s_copyRectsLog < 10) {
    printf("[CopyRects] #%d: src=%p dst=%p numRects=%u fmt=%u is16bit=%d\n",
           s_copyRectsLog, (void*)src, (void*)dst, numRects,
           (unsigned)srcFmt, (int)is16bit);
    fflush(stdout);
    s_copyRectsLog++;
  }

  return D3D_OK;
}

STDMETHODIMP MetalDevice8::UpdateTexture(IDirect3DBaseTexture8 *s,
                                         IDirect3DBaseTexture8 *d) {
  if (!s || !d) return E_FAIL;

  // Both must be IDirect3DTexture8 (our MetalTexture8)
  MetalTexture8 *srcTex = (MetalTexture8 *)s;
  MetalTexture8 *dstTex = (MetalTexture8 *)d;

  id<MTLTexture> mtlDst = dstTex->GetMTLTexture();
  if (!mtlDst) return E_FAIL;

  UINT levels = srcTex->GetLevelCount();
  UINT dstLevels = dstTex->GetLevelCount();
  if (levels > dstLevels) levels = dstLevels;

  D3DFORMAT srcFmt = srcTex->GetD3DFormat();
  bool is16bit = Is16BitFormat(srcFmt);

  for (UINT level = 0; level < levels; level++) {
    D3DLOCKED_RECT srcLocked;
    HRESULT hr = srcTex->LockRect(level, &srcLocked, nullptr, D3DLOCK_READONLY);
    if (FAILED(hr)) continue;

    D3DSURFACE_DESC desc;
    srcTex->GetLevelDesc(level, &desc);
    UINT w = desc.Width;
    UINT h = desc.Height;

    if (is16bit) {
      // Convert 16-bit to 32-bit BGRA8
      UINT dstPitch = w * 4;
      uint8_t *converted = (uint8_t *)malloc(dstPitch * h);
      if (converted) {
        for (UINT y = 0; y < h; y++) {
          const uint16_t *sp = (const uint16_t *)((uint8_t *)srcLocked.pBits + y * srcLocked.Pitch);
          uint32_t *dp = (uint32_t *)(converted + y * dstPitch);
          for (UINT x = 0; x < w; x++) {
            dp[x] = ConvertPixel16to32(srcFmt, sp[x]);
          }
        }
        MTLRegion region = MTLRegionMake2D(0, 0, w, h);
        [mtlDst replaceRegion:region mipmapLevel:level withBytes:converted bytesPerRow:dstPitch];
        free(converted);
      }
    } else {
      // Direct upload (32-bit or matching format)
      MTLRegion region = MTLRegionMake2D(0, 0, w, h);
      [mtlDst replaceRegion:region mipmapLevel:level
              withBytes:srcLocked.pBits bytesPerRow:w * 4];
    }

    srcTex->UnlockRect(level);
  }

  dstTex->MarkWritten();

  static int s_updateTexLog = 0;
  if (s_updateTexLog < 5) {
    printf("[UpdateTexture] src=%p dst=%p levels=%u fmt=%u\n",
           (void*)s, (void*)d, levels, (unsigned)srcFmt);
    fflush(stdout);
    s_updateTexLog++;
  }

  return D3D_OK;
}

STDMETHODIMP MetalDevice8::GetFrontBuffer(IDirect3DSurface8 *d) {
  return D3D_OK;
}

// ─────────────────────────────────────────────────────
//  Render Target
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::SetRenderTarget(IDirect3DSurface8 *s,
                                           IDirect3DSurface8 *d) {
  // Restoring default render target?
  if (s == nullptr || s == m_DefaultRTSurface) {
    if (m_RTTSurface) {
      fprintf(stderr, "[MetalDevice8] SetRenderTarget: restoring default RT\n");
      m_RTTSurface->Release();
      m_RTTSurface = nullptr;
    }
    m_RTTColorTexture = nullptr;
    m_RTTDepthTexture = nullptr;
    m_RTTWidth = 0;
    m_RTTHeight = 0;

    // End current encoder so next draw/Clear creates a new render pass
    // targeting the drawable.
    if (m_CurrentEncoder) {
      [MTL_ENCODER endEncoding];
      CLEAR_MTL(CurrentEncoder);
    }
    return D3D_OK;
  }

  // Setting a custom render target (render-to-texture)
  MetalSurface8 *surf = (MetalSurface8 *)s;
  MetalTexture8 *tex = surf->GetParentTexture();
  if (!tex) {
    fprintf(stderr, "[MetalDevice8] SetRenderTarget: surface has no parent texture — ignoring\n");
    return D3D_OK;
  }

  id<MTLTexture> mtl = tex->GetMTLTexture();
  if (!mtl) {
    fprintf(stderr, "[MetalDevice8] SetRenderTarget: parent texture has no MTLTexture — ignoring\n");
    return D3D_OK;
  }

  // End current encoder so next draw/Clear creates a new render pass
  // targeting the RTT texture.
  if (m_CurrentEncoder) {
    [MTL_ENCODER endEncoding];
    CLEAR_MTL(CurrentEncoder);
  }

  // Store RTT state
  if (m_RTTSurface) {
    m_RTTSurface->Release();
  }
  m_RTTSurface = s;
  m_RTTSurface->AddRef();
  m_RTTColorTexture = (__bridge void *)mtl;
  m_RTTWidth = surf->GetWidth();
  m_RTTHeight = surf->GetHeight();

  // Depth target
  if (d) {
    MetalSurface8 *dsurf = (MetalSurface8 *)d;
    MetalTexture8 *dtex = dsurf->GetParentTexture();
    if (dtex) {
      m_RTTDepthTexture = dtex->GetMetalTexture();
    } else {
      m_RTTDepthTexture = nullptr; // use default depth
    }
  } else {
    m_RTTDepthTexture = nullptr;
  }

  fprintf(stderr, "[MetalDevice8] SetRenderTarget: RTT %ux%u mtl=%p\n",
          m_RTTWidth, m_RTTHeight, m_RTTColorTexture);
  return D3D_OK;
}
STDMETHODIMP MetalDevice8::GetRenderTarget(IDirect3DSurface8 **s) {
  if (!s)
    return E_POINTER;
  if (m_DefaultRTSurface) {
    m_DefaultRTSurface->AddRef();
    *s = m_DefaultRTSurface;
    return D3D_OK;
  }
  *s = nullptr;
  return D3DERR_NOTFOUND;
}
STDMETHODIMP MetalDevice8::GetDepthStencilSurface(IDirect3DSurface8 **s) {
  if (!s)
    return E_POINTER;
  if (m_DefaultDepthSurface) {
    m_DefaultDepthSurface->AddRef();
    *s = m_DefaultDepthSurface;
    return D3D_OK;
  }
  *s = nullptr;
  return D3DERR_NOTFOUND;
}
STDMETHODIMP MetalDevice8::SetDepthStencilSurface(IDirect3DSurface8 *s) {
  return D3D_OK;
}

// ─────────────────────────────────────────────────────
//  Scene
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::BeginScene() {
  DLOG_RFLOW(1, "BeginScene m_InScene=%d", m_InScene);
  if (m_InScene)
    return D3D_OK;
  m_InScene = true;

  // Reuse existing drawable/cmdBuf if we haven't presented yet.
  // DX8 games may call BeginScene/EndScene multiple times per frame
  // (for render-to-texture passes). We must keep drawing to the same
  // drawable until Present() commits and releases it.
  if (m_CurrentDrawable && m_CurrentCommandBuffer) {
    return D3D_OK; // Still have a valid drawable from this frame
  }

  // TheSuperHackers @fix macOS: nextDrawable can return nil if all drawables
  // are in flight. With displaySyncEnabled=NO this should not block for VSync.
  id<MTLCommandBuffer> cmdBuf = [MTL_QUEUE commandBuffer];
  SET_MTL(CurrentCommandBuffer, cmdBuf);

  id<CAMetalDrawable> drawable = [MTL_LAYER nextDrawable];
  if (!drawable) {
    printf("[DIAG] BeginScene: nextDrawable returned nil! layer=%p\n", m_MetalLayer);
    fflush(stdout);
    m_InScene = false;
    CLEAR_MTL(CurrentCommandBuffer);
    return E_FAIL;
  }
  printf("[DIAG] BeginScene: got drawable=%p texture=%p %lux%lu\n",
         drawable, drawable.texture, drawable.texture.width, drawable.texture.height);
  fflush(stdout);
  SET_MTL(CurrentDrawable, drawable);

  return D3D_OK;
}

STDMETHODIMP MetalDevice8::EndScene() {
  DLOG_RFLOW(1, "EndScene m_InScene=%d", m_InScene);
  if (!m_InScene)
    return D3D_OK;
  m_InScene = false;
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::Clear(DWORD Count, const void *pRects, DWORD Flags,
                                 D3DCOLOR Color, float Z, DWORD Stencil) {
  DLOG_RFLOW(2, "Clear flags=0x%x color=0x%08x Z=%f", (unsigned)Flags, (unsigned)Color, Z);

  // WW3D calls Clear() BEFORE BeginScene(), so auto-start if needed.
  if (!m_CurrentDrawable) {
    HRESULT bshr = BeginScene();

  }
  if (!m_CurrentDrawable)
    return D3D_OK;

  if (m_CurrentEncoder) {
    [MTL_ENCODER endEncoding];
    CLEAR_MTL(CurrentEncoder);
  }

  MTLRenderPassDescriptor *rpd = [MTLRenderPassDescriptor renderPassDescriptor];
  // --- Color attachment: RTT, MSAA, or Drawable ---
  bool useMSAA = (m_MSAASampleCount > 1 && !m_RTTColorTexture && m_MSAAColorTexture);

  if (m_RTTColorTexture) {
    // RTT: render directly to RTT texture (no MSAA)
    rpd.colorAttachments[0].texture = (__bridge id<MTLTexture>)m_RTTColorTexture;
  } else if (useMSAA) {
    // MSAA: render to multisample texture, resolve to drawable
    rpd.colorAttachments[0].texture = (__bridge id<MTLTexture>)m_MSAAColorTexture;
    rpd.colorAttachments[0].resolveTexture = MTL_DRAWABLE.texture;
  } else {
    // No MSAA: render directly to drawable
    rpd.colorAttachments[0].texture = MTL_DRAWABLE.texture;
  }

  if (Flags & D3DCLEAR_TARGET) {
    float a = ((Color >> 24) & 0xFF) / 255.0f;
    float r = ((Color >> 16) & 0xFF) / 255.0f;
    float g = ((Color >> 8) & 0xFF) / 255.0f;
    float b = ((Color >> 0) & 0xFF) / 255.0f;
    // Use alpha from D3DCOLOR (typically 1.0 from D3DCOLOR_XRGB).
    // layer.opaque=YES ensures macOS ignores alpha for window compositing.
    rpd.colorAttachments[0].loadAction = MTLLoadActionClear;
    rpd.colorAttachments[0].clearColor = MTLClearColorMake(r, g, b, a);
  } else {
    rpd.colorAttachments[0].loadAction = MTLLoadActionLoad;
  }
  // Use StoreAndMultisampleResolve so MSAA texture content survives across
  // render pass boundaries (Clear calls end+restart the render pass).
  // Without this, shoreline alpha gradients written in one pass would be lost
  // before the water rendering pass can read them via destination alpha blend.
  rpd.colorAttachments[0].storeAction = useMSAA
      ? MTLStoreActionStoreAndMultisampleResolve
      : MTLStoreActionStore;

  // --- Depth attachment ---
  // Use RTT depth if set, MSAA depth if MSAA on, otherwise default depth
  id<MTLTexture> depthTarget = nil;
  if (m_RTTColorTexture && m_RTTDepthTexture) {
    depthTarget = (__bridge id<MTLTexture>)m_RTTDepthTexture;
  } else if (useMSAA && m_MSAADepthTexture) {
    depthTarget = (__bridge id<MTLTexture>)m_MSAADepthTexture;
  } else if (m_DepthTexture) {
    depthTarget = (__bridge id<MTLTexture>)m_DepthTexture;
  }

  if (depthTarget) {
    rpd.depthAttachment.texture = depthTarget;
    rpd.depthAttachment.storeAction = useMSAA
        ? MTLStoreActionDontCare  // MSAA depth doesn't need resolve
        : MTLStoreActionStore;

    if (Flags & D3DCLEAR_ZBUFFER) {
      rpd.depthAttachment.loadAction = MTLLoadActionClear;
      rpd.depthAttachment.clearDepth = Z; // DX8 typically passes 1.0
    } else {
      rpd.depthAttachment.loadAction = MTLLoadActionLoad;
    }

    rpd.stencilAttachment.texture = depthTarget;
    rpd.stencilAttachment.storeAction = useMSAA
        ? MTLStoreActionDontCare
        : MTLStoreActionStore;
    if (Flags & D3DCLEAR_STENCIL) {
      rpd.stencilAttachment.loadAction = MTLLoadActionClear;
      rpd.stencilAttachment.clearStencil = Stencil;
    } else {
      rpd.stencilAttachment.loadAction = MTLLoadActionLoad;
    }
  }

  // --- Viewport: use RTT dimensions or screen ---
  UINT vpW = m_RTTColorTexture ? m_RTTWidth : (UINT)(m_Viewport.Width > 0 ? m_Viewport.Width : MTL_LAYER.drawableSize.width);
  UINT vpH = m_RTTColorTexture ? m_RTTHeight : (UINT)(m_Viewport.Height > 0 ? m_Viewport.Height : MTL_LAYER.drawableSize.height);

  id<MTLRenderCommandEncoder> encoder =
      [MTL_CMD_BUF renderCommandEncoderWithDescriptor:rpd];
  [encoder setLabel:@"MetalDevice8 RenderPass"];
  SET_MTL(CurrentEncoder, encoder);
  m_LastAppliedCull = 0xFFFFFFFF;
  m_LastAppliedZBias = 0xFFFFFFFF;

  // --- Apply Depth Stencil State ---
  if (m_DepthTexture) {
    void *dss = GetDepthStencilState();
    if (dss) {
      [encoder setDepthStencilState:(__bridge id<MTLDepthStencilState>)dss];
    }
  }

  MTLViewport vp;
  vp.originX = m_RTTColorTexture ? 0 : m_Viewport.X;
  vp.originY = m_RTTColorTexture ? 0 : m_Viewport.Y;
  vp.width = vpW;
  vp.height = vpH;
  vp.znear = m_Viewport.MinZ;
  vp.zfar = m_Viewport.MaxZ > 0 ? m_Viewport.MaxZ : 1.0;
  [MTL_ENCODER setViewport:vp];

  return D3D_OK;
}

// ─────────────────────────────────────────────────────
//  Transforms
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::SetTransform(D3DTRANSFORMSTATETYPE State,
                                        const D3DMATRIX *pMatrix) {
  if (!pMatrix)
    return E_POINTER;
  if ((int)State >= 0 && (int)State < 260) {
    m_Transforms[(int)State] = *pMatrix;
  }

  // DIAG: log transform changes for key states
  if (State == D3DTS_WORLD || State == D3DTS_VIEW || State == D3DTS_PROJECTION) {
    const float* f = (const float*)pMatrix;
    const char* name = (State == D3DTS_WORLD) ? "WORLD" : (State == D3DTS_VIEW) ? "VIEW" : "PROJ";
    printf("[DIAG] SetTransform %s(%d): diag=[%.3f,%.3f,%.3f,%.3f] [%.3f,%.3f,%.3f,%.3f]\n",
           name, (int)State, f[0],f[5],f[10],f[15], f[12],f[13],f[14],f[15]);
    fflush(stdout);
  }

  return D3D_OK;
}

STDMETHODIMP MetalDevice8::GetTransform(D3DTRANSFORMSTATETYPE State,
                                        D3DMATRIX *pMatrix) {
  if (!pMatrix)
    return E_POINTER;
  if ((int)State >= 0 && (int)State < 260) {
    *pMatrix = m_Transforms[(int)State];
  }
  return D3D_OK;
}

// ─────────────────────────────────────────────────────
//  Viewport
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::SetViewport(const D3DVIEWPORT8 *pViewport) {
  if (!pViewport)
    return E_POINTER;
  printf("[DIAG] SetViewport x=%u y=%u w=%u h=%u minZ=%.3f maxZ=%.3f\n",
         pViewport->X, pViewport->Y, pViewport->Width, pViewport->Height,
         pViewport->MinZ, pViewport->MaxZ);
  fflush(stdout);
  m_Viewport = *pViewport;

  if (m_CurrentEncoder) {
    MTLViewport vp;
    vp.originX = pViewport->X;
    vp.originY = pViewport->Y;
    vp.width = pViewport->Width;
    vp.height = pViewport->Height;
    vp.znear = pViewport->MinZ;
    vp.zfar = pViewport->MaxZ;
    [MTL_ENCODER setViewport:vp];
  }
  return D3D_OK;
}

HRESULT MetalDevice8::GetViewport(D3DVIEWPORT8 *pViewport) {
  if (!pViewport)
    return E_POINTER;
  *pViewport = m_Viewport;
  return D3D_OK;
}

// ─────────────────────────────────────────────────────
//  Material / Lighting
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::SetMaterial(const D3DMATERIAL8 *p) {
  if (!p)
    return E_POINTER;
  m_Material = *p;
  return D3D_OK;
}

HRESULT MetalDevice8::GetMaterial(D3DMATERIAL8 *p) {
  if (!p)
    return E_POINTER;
  *p = m_Material;
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::SetLight(DWORD i, const D3DLIGHT8 *l) {
  if (i < MAX_LIGHTS && l)
    m_Lights[i] = *l;
  return D3D_OK;
}

HRESULT MetalDevice8::GetLight(DWORD i, D3DLIGHT8 *l) {
  if (i < MAX_LIGHTS && l)
    *l = m_Lights[i];
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::LightEnable(DWORD i, BOOL b) {
  if (i < MAX_LIGHTS)
    m_LightEnabled[i] = b;
  return D3D_OK;
}

HRESULT MetalDevice8::GetLightEnable(DWORD i, BOOL *b) {
  if (i < MAX_LIGHTS && b)
    *b = m_LightEnabled[i];
  return D3D_OK;
}

// ─────────────────────────────────────────────────────
//  Clip Planes
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::SetClipPlane(DWORD i, const float *p) {
  return D3D_OK;
}

// ─────────────────────────────────────────────────────
//  Render State
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::SetRenderState(D3DRENDERSTATETYPE State,
                                          DWORD Value) {
  if ((int)State < 256) {
    DWORD old = m_RenderStates[(int)State];
    m_RenderStates[(int)State] = Value;

    if (old != Value) {
      if (State == D3DRS_ZENABLE || State == D3DRS_ZWRITEENABLE ||
          State == D3DRS_ZFUNC || State == D3DRS_STENCILENABLE ||
          State == D3DRS_STENCILFUNC || State == D3DRS_STENCILFAIL ||
          State == D3DRS_STENCILZFAIL || State == D3DRS_STENCILPASS ||
          State == D3DRS_STENCILMASK || State == D3DRS_STENCILWRITEMASK) {
        m_DepthStateDirty = true;
      }
      if (State == D3DRS_CULLMODE || State == D3DRS_ZBIAS) {
        m_DrawStateDirty = true;
      }
    }
  }
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::GetRenderState(D3DRENDERSTATETYPE State,
                                          DWORD *pValue) {
  if (!pValue)
    return E_POINTER;
  if ((int)State < 256)
    *pValue = m_RenderStates[(int)State];
  return D3D_OK;
}

// ─────────────────────────────────────────────────────
//  Textures / Texture Stage States
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::SetTexture(DWORD Stage,
                                      IDirect3DBaseTexture8 *pTexture) {
  if (Stage < MAX_TEXTURE_STAGES) {
    // Generation-based caching: skip if same pointer AND same content.
    // DX8Wrapper skips its own cache on Apple (#ifndef __APPLE__) because
    // 2D UI reuses the same IDirect3DTexture8* with new pixel data.
    // Here we restore caching by checking the texture's generation counter:
    // generation increments on every UnlockRect (content update).
    if (m_Textures[Stage] == pTexture && pTexture != nullptr) {
      MetalTexture8 *mt = (MetalTexture8 *)pTexture;
      uint32_t gen = mt->GetGeneration();
      if (gen == m_TextureGeneration[Stage]) {
        return D3D_OK; // same texture, same content — skip
      }
      m_TextureGeneration[Stage] = gen;
    } else {
      m_Textures[Stage] = pTexture;
      if (pTexture) {
        m_TextureGeneration[Stage] = ((MetalTexture8 *)pTexture)->GetGeneration();
      } else {
        m_TextureGeneration[Stage] = 0;
      }
    }
    m_TextureDirtyMask |= (1u << Stage);
  }

  if (Stage == 0) {
    if (pTexture) {
      MetalTexture8 *mt = (MetalTexture8 *)pTexture;
      id<MTLTexture> mtl = mt->GetMTLTexture();
      DLOG_RFLOW(17, "SetTexture stage=0 tex=%p mtl=%p %lux%lu fmt=%lu",
        (void*)pTexture, mtl ? (__bridge void*)mtl : nullptr,
        mtl ? (unsigned long)mtl.width : 0, mtl ? (unsigned long)mtl.height : 0,
        mtl ? (unsigned long)mtl.pixelFormat : 0);
    } else {
      DLOG_RFLOW(17, "SetTexture stage=0 tex=NULL");
    }
  }
  return D3D_OK;
}

HRESULT MetalDevice8::GetTexture(DWORD Stage,
                                 IDirect3DBaseTexture8 **ppTexture) {
  if (!ppTexture)
    return E_POINTER;
  if (Stage < MAX_TEXTURE_STAGES) {
    *ppTexture = m_Textures[Stage];
    if (*ppTexture)
      (*ppTexture)->AddRef();
  } else {
    *ppTexture = nullptr;
  }
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::SetTextureStageState(DWORD Stage,
                                                D3DTEXTURESTAGESTATETYPE Type,
                                                DWORD Value) {
  if (Stage < MAX_TEXTURE_STAGES && (int)Type < 32) {
    m_TextureStageStates[Stage][(int)Type] = Value;
  }
  return D3D_OK;
}

HRESULT MetalDevice8::GetTextureStageState(DWORD Stage,
                                           D3DTEXTURESTAGESTATETYPE Type,
                                           DWORD *pValue) {
  if (!pValue)
    return E_POINTER;
  if (Stage < MAX_TEXTURE_STAGES && (int)Type < 32) {
    *pValue = m_TextureStageStates[Stage][(int)Type];
  }
  return D3D_OK;
}

// ─────────────────────────────────────────────────────
//  Validate
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::ValidateDevice(DWORD *pNumPasses) {
  if (pNumPasses)
    *pNumPasses = 1;
  return D3D_OK;
}

// ─────────────────────────────────────────────────────
//  Drawing — Stage 0 stubs
// ─────────────────────────────────────────────────────

// Helper: Get or Create PSO for FVF + current blend state
void *MetalDevice8::GetPSO(DWORD fvf, UINT stride) {
  // 1. Build key from FVF + blend state
  uint64_t key = BuildPSOKey(fvf, stride);
  auto it = m_PsoCache.find(key);
  if (it != m_PsoCache.end()) {
    return it->second;
  }

  // 2. Create Descriptor
  MTLRenderPipelineDescriptor *pd = [[MTLRenderPipelineDescriptor alloc] init];
  pd.vertexFunction = (__bridge id<MTLFunction>)m_FunctionVertex;
  pd.fragmentFunction = (__bridge id<MTLFunction>)m_FunctionFragment;
  pd.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

  // MSAA: PSO sampleCount must match render target
  pd.rasterSampleCount = m_RTTColorTexture ? 1 : m_MSAASampleCount;

  // Depth attachment pixel format must match the render pass depth attachment
  bool hasDepth = false;
  if (m_RTTColorTexture) {
    hasDepth = (m_RTTDepthTexture != nullptr);
  } else {
    hasDepth = (m_DepthTexture != nullptr);
  }
  if (hasDepth) {
    pd.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    pd.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
  }

  // --- Stage 6: Dynamic Blend State ---
  DWORD blendEn = m_RenderStates[D3DRS_ALPHABLENDENABLE];
  DWORD srcBlend = m_RenderStates[D3DRS_SRCBLEND];
  DWORD dstBlend = m_RenderStates[D3DRS_DESTBLEND];
  DWORD cwMask = m_RenderStates[D3DRS_COLORWRITEENABLE];
  if (cwMask == 0)
    cwMask = 0xF; // default: write all
  // TheSuperHackers @fix macOS: Same dest alpha protection as in BuildPSOKey
  if (!m_RTTColorTexture && cwMask == 0xF) {
    cwMask = 0x7; // RGB only, preserve destination alpha
  }

  pd.colorAttachments[0].blendingEnabled = (blendEn != 0) ? YES : NO;
  pd.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
  pd.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
  pd.colorAttachments[0].sourceRGBBlendFactor = MapD3DBlendToMTL(srcBlend);
  pd.colorAttachments[0].sourceAlphaBlendFactor = MapD3DBlendToMTL(srcBlend);
  pd.colorAttachments[0].destinationRGBBlendFactor = MapD3DBlendToMTL(dstBlend);
  pd.colorAttachments[0].destinationAlphaBlendFactor =
      MapD3DBlendToMTL(dstBlend);

  // Color write mask: D3DCOLORWRITEENABLE_RED=1, GREEN=2, BLUE=4, ALPHA=8
  MTLColorWriteMask mtlMask = MTLColorWriteMaskNone;
  if (cwMask & 1)
    mtlMask |= MTLColorWriteMaskRed;
  if (cwMask & 2)
    mtlMask |= MTLColorWriteMaskGreen;
  if (cwMask & 4)
    mtlMask |= MTLColorWriteMaskBlue;
  if (cwMask & 8)
    mtlMask |= MTLColorWriteMaskAlpha;
  pd.colorAttachments[0].writeMask = mtlMask;

  // 3. Define Vertex Layout based on FVF
  MTLVertexDescriptor *vd = [MTLVertexDescriptor vertexDescriptor];

  // Stride tracking
  NSUInteger currentOffset = 0;

  // Track which attributes are provided by the FVF
  bool hasPosition = false;
  bool hasDiffuse = false;
  bool hasTexCoord0 = false;
  bool hasNormal = false;
  bool hasSpecular = false;
  bool hasTexCoord1 = false;

  // --- Position ---
  if (fvf & D3DFVF_XYZRHW) {
    vd.attributes[0].format = MTLVertexFormatFloat4;
    vd.attributes[0].offset = currentOffset;
    vd.attributes[0].bufferIndex = 0;
    currentOffset += 16;
    hasPosition = true;
  } else if (fvf & D3DFVF_XYZ) {
    vd.attributes[0].format = MTLVertexFormatFloat3;
    vd.attributes[0].offset = currentOffset;
    vd.attributes[0].bufferIndex = 0;
    currentOffset += 12;
    hasPosition = true;
  }

  // --- Normal --- mapped to attribute(3) for lighting
  if (fvf & D3DFVF_NORMAL) {
    vd.attributes[3].format = MTLVertexFormatFloat3;
    vd.attributes[3].offset = currentOffset;
    vd.attributes[3].bufferIndex = 0;
    currentOffset += 12;
    hasNormal = true;
  }

  // --- Diffuse Color ---
  // D3DCOLOR is 0xAARRGGBB → bytes [BB,GG,RR,AA] in little-endian.
  // MTLVertexFormatUChar4Normalized_BGRA interprets [B,G,R,A] → shader
  // (R,G,B,A).
  if (fvf & D3DFVF_DIFFUSE) {
    vd.attributes[1].format = MTLVertexFormatUChar4Normalized_BGRA;
    vd.attributes[1].offset = currentOffset;
    vd.attributes[1].bufferIndex = 0;
    currentOffset += 4;
    hasDiffuse = true;
  }

  // --- Specular Color --- mapped to attribute(4)
  // Same D3DCOLOR byte order as diffuse.
  if (fvf & 0x080) { // D3DFVF_SPECULAR
    vd.attributes[4].format = MTLVertexFormatUChar4Normalized_BGRA;
    vd.attributes[4].offset = currentOffset;
    vd.attributes[4].bufferIndex = 0;
    currentOffset += 4;
    hasSpecular = true;
  }

  // --- Texture Coordinates ---
  // D3DFVF_TEX* is a counted field (bits 8-11), not bitmask flags
  UINT texCount = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
  if (texCount >= 1) {
    vd.attributes[2].format = MTLVertexFormatFloat2; // texCoord0 → attribute(2)
    vd.attributes[2].offset = currentOffset;
    vd.attributes[2].bufferIndex = 0;
    currentOffset += 8;
    hasTexCoord0 = true;
  }
  if (texCount >= 2) {
    vd.attributes[5].format = MTLVertexFormatFloat2; // texCoord1 → attribute(5)
    vd.attributes[5].offset = currentOffset;
    vd.attributes[5].bufferIndex = 0;
    currentOffset += 8;
    hasTexCoord1 = true;
  }
  
  // Use the ACTUAL stride provided by the caller (which accounts for structure padding
  // defined in the game engine's C++ structs), NOT the currentOffset which is just 
  // the tightly-packed sum of the attributes.
  // E.g. game uses 32-byte 2D vertices, but currentOffset is 28. Using 28 scrambles array!
  if (currentOffset > 0) {
    vd.layouts[0].stride = stride;
    vd.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
  }

  bool needDefaultBuffer = !hasPosition || !hasDiffuse || !hasTexCoord0 ||
                           !hasNormal || !hasSpecular || !hasTexCoord1;
  if (needDefaultBuffer) {
    // Layout for constant buffer
    vd.layouts[30].stride = 64;
    vd.layouts[30].stepFunction = MTLVertexStepFunctionConstant;
    vd.layouts[30].stepRate = 0;

    if (!hasPosition) {
      vd.attributes[0].format = MTLVertexFormatFloat3;
      vd.attributes[0].offset = 8;
      vd.attributes[0].bufferIndex = 30;
    }
    if (!hasDiffuse) {
      vd.attributes[1].format = MTLVertexFormatUChar4Normalized_BGRA;
      vd.attributes[1].offset = 0; // White
      vd.attributes[1].bufferIndex = 30;
    }
    if (!hasTexCoord0) {
      vd.attributes[2].format = MTLVertexFormatFloat2;
      vd.attributes[2].offset = 8;
      vd.attributes[2].bufferIndex = 30;
    }
    if (!hasNormal) {
      vd.attributes[3].format = MTLVertexFormatFloat3;
      vd.attributes[3].offset = 8;
      vd.attributes[3].bufferIndex = 30;
    }
    if (!hasSpecular) {
      vd.attributes[4].format = MTLVertexFormatUChar4Normalized_BGRA;
      vd.attributes[4].offset = 4; // Black
      vd.attributes[4].bufferIndex = 30;
    }
    if (!hasTexCoord1) {
      vd.attributes[5].format = MTLVertexFormatFloat2;
      vd.attributes[5].offset = 8;
      vd.attributes[5].bufferIndex = 30;
    }
  }

  pd.vertexDescriptor = vd;

  NSError *err = nil;
  id<MTLRenderPipelineState> pso = nil;
  @try {
    pso = [(__bridge id<MTLDevice>)m_Device
        newRenderPipelineStateWithDescriptor:pd
                                       error:&err];
  } @catch (NSException *exception) {
    fprintf(stderr,
            "[MetalDevice8] Exception creating PSO for FVF 0x%x key 0x%llx: %s\n",
            fvf, key, [[exception reason] UTF8String]);
    return nil;
  }
  if (!pso) {
    fprintf(stderr,
            "[MetalDevice8] Error creating PSO for FVF %x key %llx: %s\n", fvf,
            key, err ? [[err localizedDescription] UTF8String] : "(no error)");
    return nil;
  }

  m_PsoCache[key] = (__bridge_retained void *)pso;
  return (__bridge void *)pso;
}

// ─────────────────────────────────────────────────────
//  Drawing
// ─────────────────────────────────────────────────────

void MetalDevice8::EnsureCurrentEncoder() {
  if (m_CurrentEncoder)
    return;
  if (!m_CurrentDrawable)
    return;

  MTLRenderPassDescriptor *rpd = [MTLRenderPassDescriptor renderPassDescriptor];
  bool useMSAA = (m_MSAASampleCount > 1 && !m_RTTColorTexture && m_MSAAColorTexture);

  if (m_RTTColorTexture) {
    rpd.colorAttachments[0].texture = (__bridge id<MTLTexture>)m_RTTColorTexture;
  } else if (useMSAA) {
    rpd.colorAttachments[0].texture = (__bridge id<MTLTexture>)m_MSAAColorTexture;
    rpd.colorAttachments[0].resolveTexture = MTL_DRAWABLE.texture;
  } else {
    rpd.colorAttachments[0].texture = MTL_DRAWABLE.texture;
  }

  rpd.colorAttachments[0].loadAction = MTLLoadActionLoad;
  rpd.colorAttachments[0].storeAction = useMSAA
      ? MTLStoreActionStoreAndMultisampleResolve
      : MTLStoreActionStore;

  id<MTLTexture> depthTarget = nil;
  if (m_RTTColorTexture && m_RTTDepthTexture) {
    depthTarget = (__bridge id<MTLTexture>)m_RTTDepthTexture;
  } else if (useMSAA && m_MSAADepthTexture) {
    depthTarget = (__bridge id<MTLTexture>)m_MSAADepthTexture;
  } else if (m_DepthTexture) {
    depthTarget = (__bridge id<MTLTexture>)m_DepthTexture;
  }

  if (depthTarget) {
    rpd.depthAttachment.texture = depthTarget;
    rpd.depthAttachment.storeAction = useMSAA ? MTLStoreActionDontCare : MTLStoreActionStore;
    rpd.depthAttachment.loadAction = MTLLoadActionLoad;

    rpd.stencilAttachment.texture = depthTarget;
    rpd.stencilAttachment.storeAction = useMSAA ? MTLStoreActionDontCare : MTLStoreActionStore;
    rpd.stencilAttachment.loadAction = MTLLoadActionLoad;
  }

  id<MTLRenderCommandEncoder> encoder = [MTL_CMD_BUF renderCommandEncoderWithDescriptor:rpd];
  SET_MTL(CurrentEncoder, encoder);
}

STDMETHODIMP MetalDevice8::DrawPrimitive(DWORD pt, UINT sv, UINT pc) {
  EnsureCurrentEncoder();
  if (!m_CurrentEncoder || !m_StreamSource)
    return D3D_OK;

  DLOG_RFLOW(14, "DrawPrimitive pt=%u startVert=%u primCount=%u fvf=0x%x",
    (unsigned)pt, sv, pc, (unsigned)GetBufferFVF(m_StreamSource));

  // 1. Get FVF and PSO
  DWORD fvf = GetBufferFVF(m_StreamSource);
  id<MTLRenderPipelineState> pso =
      (__bridge id<MTLRenderPipelineState>)GetPSO(fvf, m_StreamStride);
  if (!pso)
    return D3D_OK;

  // 2. Set State
  [MTL_ENCODER setRenderPipelineState:pso];

  // 2b. Apply per-draw state (cull mode, depth/stencil)
  ApplyPerDrawState();

  // 3. Bind Vertex Buffer
  MetalVertexBuffer8 *vb = (MetalVertexBuffer8 *)m_StreamSource;
  [MTL_ENCODER setVertexBuffer:(__bridge id<MTLBuffer>)vb->GetMTLBuffer()
                        offset:0
                       atIndex:0];

  // 3b. Bind zero buffer for missing vertex attributes (FVF defaults)
  if (m_ZeroBuffer) {
    [MTL_ENCODER setVertexBuffer:(__bridge id<MTLBuffer>)m_ZeroBuffer
                          offset:0
                         atIndex:30];
  }


  BindUniforms(fvf);
  BindCustomVSUniforms();
  BindTexturesAndSamplers();

  MTLPrimitiveType mtlPt = (MTLPrimitiveType)MapPrimitiveType(pt);
  UINT vertexCount = 0;

  if (pt == D3DPT_TRIANGLEFAN) {
    UINT indexCount = pc * 3;
    std::vector<uint16_t> fan(indexCount);
    for (UINT i = 0; i < pc; i++) {
      fan[i * 3 + 0] = 0;
      fan[i * 3 + 1] = i + 1;
      fan[i * 3 + 2] = i + 2;
    }
    id<MTLBuffer> tempIdxBuffer = [MTL_DEVICE newBufferWithBytes:fan.data()
                                                            length:indexCount * sizeof(uint16_t)
                                                           options:MTLResourceStorageModeShared];
    [MTL_ENCODER drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                            indexCount:indexCount
                             indexType:MTLIndexTypeUInt16
                           indexBuffer:tempIdxBuffer
                     indexBufferOffset:0
                         instanceCount:1
                            baseVertex:(NSInteger)sv
                          baseInstance:0];
    return D3D_OK;
  }

  if (pt == D3DPT_TRIANGLELIST)
    vertexCount = pc * 3;
  else if (pt == D3DPT_TRIANGLESTRIP)
    vertexCount = pc + 2;
  else if (pt == D3DPT_LINELIST)
    vertexCount = pc * 2;
  else if (pt == D3DPT_POINTLIST)
    vertexCount = pc;

  if (vertexCount > 0) {
    [MTL_ENCODER drawPrimitives:mtlPt vertexStart:sv vertexCount:vertexCount];
  }
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::DrawIndexedPrimitive(DWORD pt, UINT mi, UINT nv,
                                                UINT si, UINT pc) {
  EnsureCurrentEncoder();
  DLOG_RFLOW(15, "DrawIndexedPrimitive pt=%u minIdx=%u numVerts=%u startIdx=%u primCount=%u encoder=%p",
    (unsigned)pt, mi, nv, si, pc, m_CurrentEncoder);
  if (!m_CurrentEncoder || !m_StreamSource || !m_IndexBuffer) {
    printf("[DIAG] DrawIndexedPrimitive SKIPPED: encoder=%p streamSrc=%p indexBuf=%p\n",
           m_CurrentEncoder, m_StreamSource, m_IndexBuffer);
    fflush(stdout);
    return D3D_OK;
  }

  // 1. Get FVF and PSO
  DWORD fvf = GetBufferFVF(m_StreamSource);




  id<MTLRenderPipelineState> pso =
      (__bridge id<MTLRenderPipelineState>)GetPSO(fvf, m_StreamStride);
  if (!pso) {
    DLOG_RFLOW(15, "DrawIndexedPrimitive NO PSO for fvf=0x%x", (unsigned)fvf);
    return D3D_OK;
  }

  // 2. Set State
  [MTL_ENCODER setRenderPipelineState:pso];

  // 2b. Apply per-draw state (cull mode, depth/stencil)
  ApplyPerDrawState();

  // 3. Bind VB
  MetalVertexBuffer8 *vb = (MetalVertexBuffer8 *)m_StreamSource;
  [MTL_ENCODER setVertexBuffer:(__bridge id<MTLBuffer>)vb->GetMTLBuffer()
                        offset:0
                       atIndex:0];

  // 3b. Bind zero buffer for missing vertex attributes (FVF defaults)
  if (m_ZeroBuffer) {
    [MTL_ENCODER setVertexBuffer:(__bridge id<MTLBuffer>)m_ZeroBuffer
                          offset:0
                         atIndex:30];
  }

  BindUniforms(fvf);
  BindCustomVSUniforms();
  BindTexturesAndSamplers();

  MTLPrimitiveType mtlPt = (MTLPrimitiveType)MapPrimitiveType(pt);
  UINT indexCount = 0;
  MetalIndexBuffer8 *ib = (MetalIndexBuffer8 *)m_IndexBuffer;
  MTLIndexType idxType = ib->Is_32Bit() ? MTLIndexTypeUInt32 : MTLIndexTypeUInt16;
  uint32_t offset = si * (ib->Is_32Bit() ? 4 : 2);
  id<MTLBuffer> mtlIdxBuf = (__bridge id<MTLBuffer>)ib->GetMTLBuffer();
  id<MTLBuffer> tempIdxBuffer = nil;

  if (pt == D3DPT_TRIANGLEFAN) {
    mtlPt = MTLPrimitiveTypeTriangle;
    indexCount = pc * 3;
    uint8_t *idxData = (uint8_t *)[mtlIdxBuf contents];
    if (idxData) {
      if (ib->Is_32Bit()) {
        std::vector<uint32_t> fan(indexCount);
        uint32_t *src = (uint32_t *)(idxData + offset);
        for (UINT i = 0; i < pc; i++) {
          fan[i * 3 + 0] = src[0];
          fan[i * 3 + 1] = src[i + 1];
          fan[i * 3 + 2] = src[i + 2];
        }
        tempIdxBuffer = [MTL_DEVICE newBufferWithBytes:fan.data() length:indexCount * 4 options:MTLResourceStorageModeShared];
        offset = 0;
      } else {
        std::vector<uint16_t> fan(indexCount);
        uint16_t *src = (uint16_t *)(idxData + offset);
        for (UINT i = 0; i < pc; i++) {
          fan[i * 3 + 0] = src[0];
          fan[i * 3 + 1] = src[i + 1];
          fan[i * 3 + 2] = src[i + 2];
        }
        tempIdxBuffer = [MTL_DEVICE newBufferWithBytes:fan.data() length:indexCount * 2 options:MTLResourceStorageModeShared];
        offset = 0;
      }
    }
    if (tempIdxBuffer) {
      mtlIdxBuf = tempIdxBuffer;
    }
  } else if (pt == D3DPT_TRIANGLELIST) {
    indexCount = pc * 3;
  } else if (pt == D3DPT_TRIANGLESTRIP) {
    indexCount = pc + 2;
  }

  // m_BaseVertexIndex comes from DX8 SetIndices(ib, BaseVertexIndex).
  // DX8 adds this to every index value before fetching the vertex.
  // Metal's drawIndexedPrimitives:baseVertex does the same thing.
  if (indexCount > 0 && mtlIdxBuf) {
    extern int g_metalDrawCallsThisFrame;
    g_metalDrawCallsThisFrame++;
    DLOG_RFLOW(16, "DrawIndexedPrimitive EXEC idxCount=%u fvf=0x%x pso=%p useProj=%d",
               indexCount, (unsigned)fvf, pso, (fvf & D3DFVF_XYZRHW) ? 2 : 1);
    [MTL_ENCODER drawIndexedPrimitives:mtlPt
                            indexCount:indexCount
                             indexType:idxType
                           indexBuffer:mtlIdxBuf
                     indexBufferOffset:offset
                         instanceCount:1
                            baseVertex:(NSInteger)m_BaseVertexIndex
                          baseInstance:0];
  }


  return D3D_OK;
}

STDMETHODIMP MetalDevice8::DrawPrimitiveUP(DWORD pt, UINT pc, const void *data,
                                           UINT stride) {
  EnsureCurrentEncoder();
  if (!m_CurrentEncoder || !data || pc == 0)
    return D3D_OK;

  // Use current FVF (from SetVertexShader or stream source)
  DWORD fvf = m_VertexShader;
  // If top bit is set, it's a custom vertex shader handle, not an FVF.
  // Use the FVF from the VS handle info if available, else fall back.
  if (fvf & 0x80000000) {
    auto it = m_VSHandleMap.find(fvf);
    if (it != m_VSHandleMap.end()) {
      fvf = it->second.fvf;
    } else {
      fvf = m_StreamSource ? GetBufferFVF(m_StreamSource) : 0;
      if (fvf == 0) {
        fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1; // 0x142
      }
    }
  }
  if (fvf == 0 && m_StreamSource) {
    fvf = GetBufferFVF(m_StreamSource);
  }
  if (fvf == 0) {
    fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;
  }



  // Determine vertex count and convert D3DPT_TRIANGLEFAN to TriangleList if needed
  UINT vertexCount = 0;
  MTLPrimitiveType mtlPrimType;
  std::vector<uint8_t> fanBuffer;

  if (pt == D3DPT_TRIANGLEFAN) {
    vertexCount = pc * 3;
    mtlPrimType = MTLPrimitiveTypeTriangle;
    fanBuffer.resize(vertexCount * stride);
    const uint8_t *srcData = (const uint8_t *)data;
    for (UINT i = 0; i < pc; i++) {
      memcpy(fanBuffer.data() + (i * 3 + 0) * stride, srcData, stride);
      memcpy(fanBuffer.data() + (i * 3 + 1) * stride, srcData + (i + 1) * stride, stride);
      memcpy(fanBuffer.data() + (i * 3 + 2) * stride, srcData + (i + 2) * stride, stride);
    }
    data = fanBuffer.data();
  } else {
    switch (pt) {
    case D3DPT_TRIANGLELIST:
      vertexCount = pc * 3;
      mtlPrimType = MTLPrimitiveTypeTriangle;
      break;
    case D3DPT_TRIANGLESTRIP:
      vertexCount = pc + 2;
      mtlPrimType = MTLPrimitiveTypeTriangleStrip;
      break;
    case D3DPT_LINELIST:
      vertexCount = pc * 2;
      mtlPrimType = MTLPrimitiveTypeLine;
      break;
    case D3DPT_LINESTRIP:
      vertexCount = pc + 1;
      mtlPrimType = MTLPrimitiveTypeLineStrip;
      break;
    case D3DPT_POINTLIST:
      vertexCount = pc;
      mtlPrimType = MTLPrimitiveTypePoint;
      break;
    default:
      return D3D_OK;
    }
  }

  // Use current FVF (from SetVertexShader or stream source)
  // (already evaluated above)

  id<MTLRenderPipelineState> pso =
      (__bridge id<MTLRenderPipelineState>)GetPSO(fvf, stride);
  if (!pso)
    return D3D_OK;

  [MTL_ENCODER setRenderPipelineState:pso];

  // For XYZRHW (2D/UI) vertices, force-disable depth test & depth write.
  // DX8 spec: pretransformed vertices bypass the transform pipeline and
  // typically render with Z-test disabled. Without this, 2D UI quads at z=0
  // fail the depth test against previously rendered 3D geometry.
  bool is2D = (fvf & D3DFVF_XYZRHW) != 0;
  DWORD savedZEnable = 0;
  DWORD savedZWrite  = 0;
  if (is2D) {
    savedZEnable = m_RenderStates[D3DRS_ZENABLE];
    savedZWrite  = m_RenderStates[D3DRS_ZWRITEENABLE];
    m_RenderStates[D3DRS_ZENABLE] = FALSE;
    m_RenderStates[D3DRS_ZWRITEENABLE] = FALSE;
    m_DepthStateDirty = true;
  }
  ApplyPerDrawState();

  // For XYZRHW (2D/UI) vertices, force-disable back-face culling.
  // The vertex shader flips Y (screenPos.y = 1.0 - y/screenH * 2.0) which
  // reverses the triangle winding order from CW to CCW in NDC space.
  // With the default CW front-face winding + back-face culling, all 2D
  // triangles would be discarded as back-facing. Must set AFTER
  // ApplyPerDrawState() which sets cull mode from D3D render state.
  if (is2D) {
    [MTL_ENCODER setCullMode:MTLCullModeNone];
  }

  // Upload vertex data inline (up to 4KB via setVertexBytes)
  UINT dataSize = vertexCount * stride;
  if (dataSize <= 4096) {
    [MTL_ENCODER setVertexBytes:data length:dataSize atIndex:0];
  } else {
    // TheSuperHackers @perf Ring buffer for DrawPrimitiveUP temp vertex data.
    // Pre-allocated 256KB shared buffer, offset advances per call, resets each frame.
    if (!m_RingBuffer) {
      id<MTLBuffer> rb = [MTL_DEVICE newBufferWithLength:m_RingBufferSize
                                                options:MTLResourceStorageModeShared];
      m_RingBuffer = (__bridge_retained void *)rb;
    }

    uint32_t aligned = (dataSize + 255) & ~255u;
    if (m_RingBufferOffset + aligned > m_RingBufferSize) {
      m_RingBufferOffset = 0;
    }

    if (aligned <= m_RingBufferSize) {
      id<MTLBuffer> rb = (__bridge id<MTLBuffer>)m_RingBuffer;
      memcpy((uint8_t *)[rb contents] + m_RingBufferOffset, data, dataSize);
      [MTL_ENCODER setVertexBuffer:rb offset:m_RingBufferOffset atIndex:0];
      m_RingBufferOffset += aligned;
    } else {
      id<MTLBuffer> tmpBuf =
          [MTL_DEVICE newBufferWithBytes:data
                                  length:dataSize
                                 options:MTLResourceStorageModeShared];
      if (!tmpBuf)
        return D3D_OK;
      [MTL_ENCODER setVertexBuffer:tmpBuf offset:0 atIndex:0];
    }
  }

  // Bind zero buffer for missing vertex attributes (FVF defaults)
  if (m_ZeroBuffer) {
    [MTL_ENCODER setVertexBuffer:(__bridge id<MTLBuffer>)m_ZeroBuffer
                          offset:0
                         atIndex:30];
  }


  BindUniforms(fvf);
  BindCustomVSUniforms();
  BindTexturesAndSamplers();

  // Draw
  [MTL_ENCODER drawPrimitives:mtlPrimType
                  vertexStart:0
                  vertexCount:vertexCount];

  // Restore depth state for XYZRHW (2D) draws
  if (is2D) {
    m_RenderStates[D3DRS_ZENABLE] = savedZEnable;
    m_RenderStates[D3DRS_ZWRITEENABLE] = savedZWrite;
    m_DepthStateDirty = true;
  }

  return D3D_OK;
}
STDMETHODIMP
MetalDevice8::DrawIndexedPrimitiveUP(DWORD pt, UINT mvi, UINT nvi, UINT pc,
                                     const void *idata, D3DFORMAT ifmt,
                                     const void *vdata, UINT vstride) {
  EnsureCurrentEncoder();
  // TODO: Implement if needed — currently no callers in the engine
  return D3D_OK;
}

// ─────────────────────────────────────────────────────
//  Vertex Shaders
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::CreateVertexShader(const DWORD *decl,
                                              const DWORD *func, DWORD *handle,
                                              DWORD usage) {
  static DWORD s_nextVS = 1;
  if (handle) {
    // Top bit set means it's a shader handle, not an FVF
    DWORD h = (1 << 31) | s_nextVS++;
    *handle = h;

    // Parse the vertex declaration to extract FVF
    // D3DVSD tokens: stream 0, position, normal, diffuse, tex coords
    // We detect shader type by the handle ordinal:
    //   handle 1 (0x80000001) = Trees.vso (first shader created)
    //   handle 2 (0x80000002) = Trees.pso (pixel shader, ignored)
    //   handle 3+ = water wave etc.
    // Better approach: count how many VS handles (not PS) we've created
    VSHandleInfo info;
    info.handle = h;
    info.fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1; // default
    info.shaderType = 0; // unknown
    
    // Tree VS uses declaration: stream 0, XYZ, NORMAL, DIFFUSE, TEX(2 floats)
    // which is effectively FVF 0x152 = XYZ|NORMAL|DIFFUSE|TEX1
    // Water wave VS: XYZ, DIFFUSE, TEX(2 floats) = 0x142 = XYZ|DIFFUSE|TEX1
    //
    // We determine shader type from the declaration structure:
    // Parse D3DVSD tokens to determine what vertex elements are declared
    if (decl) {
      bool hasPosition = false;
      bool hasNormal = false;
      bool hasDiffuse = false;
      int texCount = 0;
      for (int i = 0; decl[i] != 0xFFFFFFFF && i < 64; i++) {
        DWORD token = decl[i];
        // D3DVSD_STREAM(s) has bit 31 set — skip stream tokens
        if (token & 0x80000000) continue;
        // D3DVSD_REG(r, t) = r | (t << 16), bit 31 clear
        DWORD dataType = (token >> 16) & 0xF;
        // dataType: 0=float1, 1=float2, 2=float3, 3=float4, 4=D3DCOLOR
        if (dataType == 2) { // float3 = position or normal
          if (!hasPosition) {
            hasPosition = true;
          } else {
            hasNormal = true;
          }
        } else if (dataType == 4) { // D3DCOLOR = diffuse
          hasDiffuse = true;
        } else if (dataType == 1) { // float2 = texcoord
          texCount++;
        }
      }
      // Build FVF from parsed declaration
      DWORD parsedFVF = D3DFVF_XYZ;
      if (hasNormal) parsedFVF |= D3DFVF_NORMAL;
      if (hasDiffuse) parsedFVF |= D3DFVF_DIFFUSE;
      if (texCount >= 1) parsedFVF |= D3DFVF_TEX1;
      if (texCount >= 2) parsedFVF |= D3DFVF_TEX2;
      info.fvf = parsedFVF;
      
      // Shader type heuristic:
      // Trees: XYZ + NORMAL + DIFFUSE + TEX1 (FVF 0x152)
      // Water: XYZ + DIFFUSE + TEX1 (FVF 0x142)
      if (hasNormal && hasDiffuse && texCount >= 1) {
        info.shaderType = 1; // Trees
      } else if (!hasNormal && hasDiffuse && texCount >= 1) {
        info.shaderType = 2; // Water wave
      }
    }
    
    m_VSHandleMap[h] = info;
    
    printf("[VS] CreateVertexShader: handle=0x%08x fvf=0x%x type=%u\n",
           (unsigned)h, (unsigned)info.fvf, info.shaderType);
    fflush(stdout);
  }
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::SetVertexShader(DWORD h) {
  m_VertexShader = h;
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::DeleteVertexShader(DWORD h) { return D3D_OK; }

STDMETHODIMP MetalDevice8::SetVertexShaderConstant(DWORD r, const void *d,
                                                   DWORD c) {
  if (d && r < MAX_VS_CONSTANTS) {
    DWORD count = c;
    if (r + count > MAX_VS_CONSTANTS) {
      count = MAX_VS_CONSTANTS - r;
    }
    memcpy(&m_VSConstants[r], d, count * 4 * sizeof(float));
  }
  return D3D_OK;
}

// ─────────────────────────────────────────────────────
//  Stream Source / Indices
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::SetStreamSource(UINT streamNum,
                                           IDirect3DVertexBuffer8 *vb,
                                           UINT stride) {
  DLOG_RFLOW(10, "SetStreamSource stream=%u vb=%p stride=%u", streamNum, vb, stride);
  if (streamNum == 0) {
    m_StreamSource = vb;
    m_StreamStride = stride;
  }
  return D3D_OK;
}

HRESULT MetalDevice8::GetStreamSource(UINT streamNum,
                                      IDirect3DVertexBuffer8 **vb,
                                      UINT *stride) {
  if (streamNum == 0) {
    if (vb)
      *vb = m_StreamSource;
    if (stride)
      *stride = m_StreamStride;
  }
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::SetIndices(IDirect3DIndexBuffer8 *ib, UINT base) {
  DLOG_RFLOW(10, "SetIndices ib=%p base=%u", ib, base);
  m_IndexBuffer = ib;
  m_BaseVertexIndex = base;
  return D3D_OK;
}

HRESULT MetalDevice8::GetIndices(IDirect3DIndexBuffer8 **ib, UINT *base) {
  if (ib)
    *ib = m_IndexBuffer;
  if (base)
    *base = m_BaseVertexIndex;
  return D3D_OK;
}

// ─────────────────────────────────────────────────────
//  Pixel Shaders
// ─────────────────────────────────────────────────────

STDMETHODIMP MetalDevice8::CreatePixelShader(const DWORD *func, DWORD *handle) {
  static DWORD s_nextPS = 1;
  if (!handle) return D3D_OK;

  DWORD h = 0xC0000000 | s_nextPS++;
  *handle = h;

  PSHandleInfo info;
  info.handle = h;
  info.psType = PS_NONE;
  info.numTexStages = 0;
  info.numArithOps = 0;

  if (func) {
    DWORD version = func[0];

    if ((version & 0xFFFF0000) == 0xFFFF0000) {
      uint32_t numTex = 0;
      uint32_t numArith = 0;
      bool hasDp3 = false;
      bool hasLrp = false;
      bool hasMad = false;
      bool hasMul = false;
      bool hasAdd = false;
      bool hasTexbem = false;

      // PS 1.x bytecode format:
      //   DWORD 0: version token (0xFFFF0101 for ps_1_1, etc.)
      //   Then instruction tokens until END token (0x0000FFFF).
      //   Each instruction: opcode DWORD, then operand DWORDs.
      //   For PS 1.x, instruction length is NOT encoded in the opcode token
      //   (that's only ps_2_0+). Instead we use known operand counts per opcode.
      int i = 1;
      while (i < 256) {
        DWORD token = func[i];
        if (token == 0x0000FFFF) break; // END token
        i++; // consume opcode token

        uint32_t opcode = token & 0xFFFF;

        // Determine operand count for PS 1.x instructions
        int operandCount = 0;

        if (opcode == 0x00) {
          // nop
          operandCount = 0;
        } else if (opcode == 0x01) {
          // mov: dest, src
          operandCount = 2;
        } else if (opcode == 0x02) {
          // add: dest, src0, src1
          hasAdd = true; numArith++;
          operandCount = 3;
        } else if (opcode == 0x03) {
          // sub: dest, src0, src1
          numArith++;
          operandCount = 3;
        } else if (opcode == 0x04) {
          // mad: dest, src0, src1, src2
          hasMad = true; numArith++;
          operandCount = 4;
        } else if (opcode == 0x05) {
          // mul: dest, src0, src1
          hasMul = true; numArith++;
          operandCount = 3;
        } else if (opcode == 0x06) {
          // rcp: dest, src
          numArith++;
          operandCount = 2;
        } else if (opcode == 0x08) {
          // dp3: dest, src0, src1
          hasDp3 = true; numArith++;
          operandCount = 3;
        } else if (opcode == 0x09) {
          // dp3 (alternative encoding)
          hasDp3 = true; numArith++;
          operandCount = 3;
        } else if (opcode == 0x0A) {
          // dp4: dest, src0, src1
          numArith++;
          operandCount = 3;
        } else if (opcode == 0x12) {
          // lrp: dest, src0, src1, src2
          hasLrp = true; numArith++;
          operandCount = 4;
        } else if (opcode == 0x40) {
          // tex / texcoord: dest only in ps_1_1-1_3
          numTex++;
          operandCount = 1;
        } else if (opcode == 0x41) {
          // texbem: dest, src
          hasTexbem = true; numTex++;
          operandCount = 2;
        } else if (opcode == 0x42) {
          // In ps_1_1/1_2/1_3: tex = dest only (1 operand)
          // In ps_1_4: texld = dest, src (2 operands)
          numTex++;
          uint32_t minor = version & 0xFF;
          operandCount = (minor <= 3) ? 1 : 2;
        } else if (opcode == 0x43) {
          // texreg2gb: dest, src
          numTex++;
          operandCount = 2;
        } else if (opcode == 0x44) {
          // texm3x2pad: dest, src
          numTex++;
          operandCount = 2;
        } else if (opcode == 0x45) {
          // texm3x2tex: dest, src
          numTex++;
          operandCount = 2;
        } else if (opcode == 0x46) {
          // texm3x3pad: dest, src
          numTex++;
          operandCount = 2;
        } else if (opcode == 0x47) {
          // texm3x3tex: dest, src
          numTex++;
          operandCount = 2;
        } else if (opcode == 0x48) {
          // reserved
          numTex++;
          operandCount = 2;
        } else if (opcode == 0x49) {
          // texm3x3spec: dest, src0, src1
          numTex++;
          operandCount = 3;
        } else if (opcode == 0x4A) {
          // texm3x3vspec: dest, src
          numTex++;
          operandCount = 2;
        } else if (opcode == 0x50) {
          // cnd: dest, src0, src1, src2
          numArith++;
          operandCount = 4;
        } else if (opcode == 0x51) {
          // def: dest, float, float, float, float (constant definition)
          // NOT a tex instruction! 5 DWORDs follow opcode
          operandCount = 5;
        } else if (opcode == 0x58) {
          // cmp: dest, src0, src1, src2
          numArith++;
          operandCount = 4;
        } else if (opcode >= 0x4B && opcode <= 0x5F) {
          // other tex ops (but not def/cnd/cmp already handled)
          numTex++;
          operandCount = 2;
        } else if (opcode == 0xFFFE) {
          // comment: next DWORD is length in DWORDs
          uint32_t commentLen = (token >> 16) & 0xFFFF;
          i += commentLen;
          continue;
        } else if (opcode >= 0x02 && opcode <= 0x3F) {
          // Other arithmetic — assume dest + 2 src
          numArith++;
          operandCount = 3;
        } else {
          // Unknown — skip conservatively
          operandCount = 0;
        }

        i += operandCount; // skip operands
      }

      info.numTexStages = numTex;
      info.numArithOps = numArith;

      if (numTex == 1 && hasDp3) {
        info.psType = PS_MONOCHROME;
      } else if (hasTexbem && numTex >= 3) {
        info.psType = PS_WATER_BUMP;
      } else if (hasTexbem) {
        info.psType = PS_WAVE;
      } else if (numTex == 2 && hasLrp) {
        info.psType = PS_TERRAIN;
      } else if (numTex == 3 && hasLrp) {
        info.psType = PS_TERRAIN_NOISE1;
      } else if (numTex == 4 && hasLrp) {
        info.psType = PS_TERRAIN_NOISE2;
      } else if (numTex == 3 && !hasLrp) {
        info.psType = PS_ROAD_NOISE2;
      } else if (numTex == 2 && !hasLrp && !hasDp3) {
        info.psType = PS_FLAT_TERRAIN;
      } else if (numTex == 4 && !hasLrp && hasMad) {
        info.psType = PS_WATER_TRAPEZOID;
      } else if (numTex == 4 && !hasLrp && !hasMad) {
        if (hasAdd) {
          info.psType = PS_WATER_RIVER;
        } else {
          info.psType = PS_FLAT_TERRAIN_NOISE2;
        }
      } else {
        info.psType = PS_TERRAIN;
      }


    }
  }

  m_PSHandleMap[h] = info;
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::SetPixelShader(DWORD h) {
  m_PixelShader = h;
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::DeletePixelShader(DWORD h) {
  m_PSHandleMap.erase(h);
  return D3D_OK;
}

STDMETHODIMP MetalDevice8::SetPixelShaderConstant(DWORD r, const void *d,
                                                   DWORD c) {
  if (!d) return D3D_OK;
  const float *src = (const float *)d;
  for (DWORD i = 0; i < c && (r + i) < MAX_PS_CONSTANTS; i++) {
    m_PSConstants[r + i][0] = src[i * 4 + 0];
    m_PSConstants[r + i][1] = src[i * 4 + 1];
    m_PSConstants[r + i][2] = src[i * 4 + 2];
    m_PSConstants[r + i][3] = src[i * 4 + 3];
  }
  return D3D_OK;
}


// ─────────────────────────────────────────────────────
//  Non-override helpers
// ─────────────────────────────────────────────────────

HRESULT MetalDevice8::GetDirect3D(IDirect3D8 **ppD3D8) {
  if (ppD3D8)
    *ppD3D8 = nullptr;
  return D3D_OK;
}

// ─────────────────────────────────────────────────────
//  updateScreenSize — called by MacOSDisplayManager
//  Updates screen dimensions, recreates depth texture,
//  and resets the viewport to the new size.
// ─────────────────────────────────────────────────────

void MetalDevice8::updateScreenSize(int width, int height) {
  fprintf(stderr, "[MetalDevice8] updateScreenSize: %gx%g -> %dx%d\n",
          m_ScreenWidth, m_ScreenHeight, width, height);

  m_ScreenWidth = (float)width;
  m_ScreenHeight = (float)height;

  // Recreate depth texture to match new size
  if (width > 0 && height > 0) {
    CreateDepthTexture((UINT)width, (UINT)height);
  }

  // Reset viewport to cover the entire new screen
  D3DVIEWPORT8 vp;
  vp.X = 0;
  vp.Y = 0;
  vp.Width = (DWORD)width;
  vp.Height = (DWORD)height;
  vp.MinZ = 0.0f;
  vp.MaxZ = 1.0f;
  SetViewport(&vp);

  // Recreate default surfaces at new size
  if (m_DefaultRTSurface) {
    m_DefaultRTSurface->Release();
    m_DefaultRTSurface = nullptr;
  }
  if (m_DefaultDepthSurface) {
    m_DefaultDepthSurface->Release();
    m_DefaultDepthSurface = nullptr;
  }
  m_DefaultRTSurface = W3DNEW MetalSurface8(this, MetalSurface8::kColor,
                                            (UINT)width, (UINT)height, D3DFMT_A8R8G8B8);
  m_DefaultDepthSurface = W3DNEW MetalSurface8(this, MetalSurface8::kDepth,
                                               (UINT)width, (UINT)height, D3DFMT_D24S8);

  fprintf(stderr, "[MetalDevice8] Screen size updated to %dx%d\n", width, height);
}

// Extern C bridge — called from MacOSDisplayManager.mm
extern "C" void MacOS_UpdateMetalDeviceScreenSize(int width, int height) {
  if (g_theMetalDevice) {
    g_theMetalDevice->updateScreenSize(width, height);
  } else {
    fprintf(stderr, "[MetalDevice8] WARNING: MacOS_UpdateMetalDeviceScreenSize called but g_theMetalDevice is null\n");
  }
}

// Extern C bridges for texture dirty tracking — called from dx8wrapper.cpp
extern "C" uint32_t MacOS_GetTextureDirtyMask(void *device) {
  if (device) {
    return static_cast<MetalDevice8 *>((IDirect3DDevice8 *)device)->GetTextureDirtyMask();
  }
  return 0;
}

extern "C" void MacOS_ClearTextureDirty(void *device) {
  if (device) {
    static_cast<MetalDevice8 *>((IDirect3DDevice8 *)device)->ClearTextureDirty();
  }
}

#endif // __APPLE__
