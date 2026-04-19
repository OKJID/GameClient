// dx8wrapper_metal.mm — Metal implementation of DX8Wrapper static methods
//
// This file replaces dx8wrapper.cpp on macOS (which is guarded by #ifndef __APPLE__).
// All static fields and methods of DX8Wrapper class are defined here.

#include "dx8wrapper.h"
#include "dx8caps.h"
#include "dx8texman.h"
#include "dx8renderer.h"
#include "rddesc.h"
#include "formconv.h"
#include "ww3d.h"
#include "wwstring.h"
#include "matrix4.h"
#include "vertmaterial.h"
#include "lightenvironment.h"
#include "statistics.h"
#include "textureloader.h"
#include "missingtexture.h"
#include "pot.h"
#include "bound.h"
#include "dx8vertexbuffer.h"
#include "dx8indexbuffer.h"
#include "sortingrenderer.h"
#include "wwprofile.h"

#include "MetalTexture8.h"
#include "render2d.h"
#include "thread.h"
#include "boxrobj.h"
#include "pointgr.h"
#include "shattersystem.h"
#include "shdlib.h"
#include "surfaceclass.h"
#include "texture.h"
#include "ffactory.h"
#include "assetmgr.h"

#include <d3dx8core.h>

#include "MetalDevice8.h"
#include "MetalInterface8.h"
#include "MetalSurface8.h"
#include "../Utils/MacDebug.h"

// ── Constants (mirrors dx8wrapper.cpp lines 91-95) ──

const int DEFAULT_RESOLUTION_WIDTH = 640;
const int DEFAULT_RESOLUTION_HEIGHT = 480;
const int DEFAULT_BIT_DEPTH = 32;
const int DEFAULT_TEXTURE_BIT_DEPTH = 16;
const D3DMULTISAMPLE_TYPE DEFAULT_MSAA = D3DMULTISAMPLE_NONE;

#define WW3D_DEVTYPE D3DDEVTYPE_HAL

#ifndef HIWORD
#define HIWORD(l) ((unsigned short)(((unsigned long)(l) >> 16) & 0xFFFF))
#endif
#ifndef LOWORD
#define LOWORD(l) ((unsigned short)((unsigned long)(l) & 0xFFFF))
#endif

bool DX8Wrapper_IsWindowed = true;
int DX8Wrapper_PreserveFPU = 0;

// ── Static field definitions (mirrors dx8wrapper.cpp lines 108-215) ──

static HWND _Hwnd = nullptr;

bool DX8Wrapper::IsInitted = false;
bool DX8Wrapper::_EnableTriangleDraw = true;

int DX8Wrapper::CurRenderDevice = -1;
int DX8Wrapper::ResolutionWidth = DEFAULT_RESOLUTION_WIDTH;
int DX8Wrapper::ResolutionHeight = DEFAULT_RESOLUTION_HEIGHT;
int DX8Wrapper::BitDepth = DEFAULT_BIT_DEPTH;
int DX8Wrapper::TextureBitDepth = DEFAULT_TEXTURE_BIT_DEPTH;
bool DX8Wrapper::IsWindowed = false;
D3DFORMAT DX8Wrapper::DisplayFormat = D3DFMT_UNKNOWN;
D3DMULTISAMPLE_TYPE DX8Wrapper::MultiSampleAntiAliasing = DEFAULT_MSAA;

D3DMATRIX DX8Wrapper::old_world;
D3DMATRIX DX8Wrapper::old_view;
D3DMATRIX DX8Wrapper::old_prj;

DWORD DX8Wrapper::Vertex_Shader = 0;
DWORD DX8Wrapper::Pixel_Shader = 0;

Vector4 DX8Wrapper::Vertex_Shader_Constants[MAX_VERTEX_SHADER_CONSTANTS];
Vector4 DX8Wrapper::Pixel_Shader_Constants[MAX_PIXEL_SHADER_CONSTANTS];

LightEnvironmentClass* DX8Wrapper::Light_Environment = nullptr;
RenderInfoClass* DX8Wrapper::Render_Info = nullptr;

DWORD DX8Wrapper::Vertex_Processing_Behavior = 0;
ZTextureClass* DX8Wrapper::Shadow_Map[MAX_SHADOW_MAPS];

Vector3 DX8Wrapper::Ambient_Color;

bool DX8Wrapper::world_identity;
unsigned DX8Wrapper::RenderStates[256];
unsigned DX8Wrapper::TextureStageStates[MAX_TEXTURE_STAGES][32];
IDirect3DBaseTexture8* DX8Wrapper::Textures[MAX_TEXTURE_STAGES];
RenderStateStruct DX8Wrapper::render_state;
unsigned DX8Wrapper::render_state_changed;

bool DX8Wrapper::FogEnable = false;
D3DCOLOR DX8Wrapper::FogColor = 0;

IDirect3D8* DX8Wrapper::D3DInterface = nullptr;
IDirect3DDevice8* DX8Wrapper::D3DDevice = nullptr;
IDirect3DSurface8* DX8Wrapper::CurrentRenderTarget = nullptr;
IDirect3DSurface8* DX8Wrapper::CurrentDepthBuffer = nullptr;
IDirect3DSurface8* DX8Wrapper::DefaultRenderTarget = nullptr;
IDirect3DSurface8* DX8Wrapper::DefaultDepthBuffer = nullptr;
bool DX8Wrapper::IsRenderToTexture = false;

unsigned DX8Wrapper::matrix_changes = 0;
unsigned DX8Wrapper::material_changes = 0;
unsigned DX8Wrapper::vertex_buffer_changes = 0;
unsigned DX8Wrapper::index_buffer_changes = 0;
unsigned DX8Wrapper::light_changes = 0;
unsigned DX8Wrapper::texture_changes = 0;
unsigned DX8Wrapper::render_state_changes = 0;
unsigned DX8Wrapper::texture_stage_state_changes = 0;
unsigned DX8Wrapper::draw_calls = 0;
unsigned DX8Wrapper::_MainThreadID = 0;
bool DX8Wrapper::CurrentDX8LightEnables[4];
bool DX8Wrapper::IsDeviceLost;
int DX8Wrapper::ZBias;
float DX8Wrapper::ZNear;
float DX8Wrapper::ZFar;
D3DMATRIX DX8Wrapper::ProjectionMatrix;
D3DMATRIX DX8Wrapper::DX8Transforms[D3DTS_WORLD + 1];

DX8Caps* DX8Wrapper::CurrentCaps = nullptr;
unsigned DX8Wrapper::DrawPolygonLowBoundLimit = 0;
D3DADAPTER_IDENTIFIER8 DX8Wrapper::CurrentAdapterIdentifier;
unsigned long DX8Wrapper::FrameCount = 0;

bool _DX8SingleThreaded = false;
unsigned number_of_DX8_calls = 0;

static unsigned last_frame_matrix_changes = 0;
static unsigned last_frame_material_changes = 0;
static unsigned last_frame_vertex_buffer_changes = 0;
static unsigned last_frame_index_buffer_changes = 0;
static unsigned last_frame_light_changes = 0;
static unsigned last_frame_texture_changes = 0;
static unsigned last_frame_render_state_changes = 0;
static unsigned last_frame_texture_stage_state_changes = 0;
static unsigned last_frame_number_of_DX8_calls = 0;
static unsigned last_frame_draw_calls = 0;

DX8_CleanupHook* DX8Wrapper::m_pCleanupHook = nullptr;
#ifdef EXTENDED_STATS
DX8_Stats DX8Wrapper::stats;
#endif

// ── Metal state (not part of DX8Wrapper class) ──

static id<MTLDevice> s_metalDevice = nil;
static id<MTLCommandQueue> s_commandQueue = nil;
static MTKView* s_metalView = nil;

// ── File-scoped statics (mirrors dx8wrapper.cpp lines 200-210) ──

static D3DDISPLAYMODE DesktopMode;
static D3DPRESENT_PARAMETERS _PresentParameters;
static DynamicVectorClass<StringClass> _RenderDeviceNameTable;
static DynamicVectorClass<StringClass> _RenderDeviceShortNameTable;
static DynamicVectorClass<RenderDeviceDescClass> _RenderDeviceDescriptionTable;

// ── Init (mirrors dx8wrapper.cpp lines 275-361) ──

bool DX8Wrapper::Init(void* hwnd, bool lite)
{
	// printf("[DIAG] DX8Wrapper::Init hwnd=%p lite=%d\n", hwnd, lite);
	fflush(stdout);
	WWASSERT(!IsInitted);

	memset(Textures,0,sizeof(IDirect3DBaseTexture8*)*MAX_TEXTURE_STAGES);
	memset(RenderStates,0,sizeof(unsigned)*256);
	memset(TextureStageStates,0,sizeof(unsigned)*32*MAX_TEXTURE_STAGES);
	memset(Vertex_Shader_Constants,0,sizeof(Vector4)*MAX_VERTEX_SHADER_CONSTANTS);
	memset(Pixel_Shader_Constants,0,sizeof(Vector4)*MAX_PIXEL_SHADER_CONSTANTS);
	memset(&render_state,0,sizeof(RenderStateStruct));
	memset(Shadow_Map,0,sizeof(ZTextureClass*)*MAX_SHADOW_MAPS);

	_Hwnd = (HWND)hwnd;
	_MainThreadID=ThreadClass::_Get_Current_Thread_ID();
	CurRenderDevice = -1;
	ResolutionWidth = DEFAULT_RESOLUTION_WIDTH;
	ResolutionHeight = DEFAULT_RESOLUTION_HEIGHT;
	Render2DClass::Set_Screen_Resolution( RectClass( 0, 0, ResolutionWidth, ResolutionHeight ) );
	BitDepth = DEFAULT_BIT_DEPTH;
	IsWindowed = false;
	DX8Wrapper_IsWindowed = false;

	for (int light=0;light<4;++light) CurrentDX8LightEnables[light]=false;

	::ZeroMemory(&old_world, sizeof(D3DMATRIX));
	::ZeroMemory(&old_view, sizeof(D3DMATRIX));
	::ZeroMemory(&old_prj, sizeof(D3DMATRIX));

	D3DInterface = nullptr;
	D3DDevice = nullptr;

	Reset_Statistics();
	Invalidate_Cached_Render_States();

	if (!lite) {
		D3DInterface = new MetalInterface8();
		if (D3DInterface == nullptr) {
			return false;
		}
		IsInitted = true;

		Enumerate_Devices();
	}

	return true;
}
// ── Shutdown (mirrors dx8wrapper.cpp lines 364-407) ──

void DX8Wrapper::Shutdown()
{
	if (D3DDevice) {
		Set_Render_Target((IDirect3DSurface8*)nullptr);
		Release_Device();
	}

	if (D3DInterface) {
		delete D3DInterface;
		D3DInterface=nullptr;
	}

	if (CurrentCaps)
	{
		int max=CurrentCaps->Get_Max_Textures_Per_Pass();
		for (int i = 0; i < max; i++)
		{
			if (Textures[i])
			{
				Textures[i]->Release();
				Textures[i] = nullptr;
			}
		}
	}

	DX8Caps::Shutdown();
	IsInitted = false;
}

// ── Do_Onetime_Device_Dependent_Inits (mirrors dx8wrapper.cpp lines 409-430) ──

void DX8Wrapper::Do_Onetime_Device_Dependent_Inits()
{
	Compute_Caps(D3DFormat_To_WW3DFormat(DisplayFormat));

	MissingTexture::_Init();
	TextureFilterClass::_Init_Filters((TextureFilterClass::TextureFilterMode)WW3D::Get_Texture_Filter());
	TheDX8MeshRenderer.Init();
	SHD_INIT;
	BoxRenderObjClass::Init();
	VertexMaterialClass::Init();
	PointGroupClass::_Init();
	ShatterSystem::Init();
	TextureLoader::Init();

	Set_Default_Global_Render_States();
}

// ── Do_Onetime_Device_Dependent_Shutdowns (mirrors dx8wrapper.cpp lines 498-529) ──

void DX8Wrapper::Do_Onetime_Device_Dependent_Shutdowns()
{
	int i;
	for (i=0;i<MAX_VERTEX_STREAMS;++i) {
		if (render_state.vertex_buffers[i]) render_state.vertex_buffers[i]->Release_Engine_Ref();
		REF_PTR_RELEASE(render_state.vertex_buffers[i]);
	}
	if (render_state.index_buffer) render_state.index_buffer->Release_Engine_Ref();
	REF_PTR_RELEASE(render_state.index_buffer);
	REF_PTR_RELEASE(render_state.material);
	for (i=0;i<CurrentCaps->Get_Max_Textures_Per_Pass();++i) REF_PTR_RELEASE(render_state.Textures[i]);

	TextureLoader::Deinit();
	SortingRendererClass::Deinit();
	DynamicVBAccessClass::_Deinit();
	DynamicIBAccessClass::_Deinit();
	ShatterSystem::Shutdown();
	PointGroupClass::_Shutdown();
	VertexMaterialClass::Shutdown();
	BoxRenderObjClass::Shutdown();
	SHD_SHUTDOWN;
	TheDX8MeshRenderer.Shutdown();
	MissingTexture::_Deinit();

	delete CurrentCaps;
	CurrentCaps=nullptr;
}

// ── Create_Device (mirrors dx8wrapper.cpp lines 532-655, adapted for Metal) ──

bool DX8Wrapper::Create_Device()
{
	// printf("[DIAG] DX8Wrapper::Create_Device hwnd=%p res=%dx%d\n", _Hwnd, ResolutionWidth, ResolutionHeight);
	fflush(stdout);
	WWASSERT(D3DDevice==nullptr);

	D3DCAPS8 caps;
	if (FAILED(D3DInterface->GetDeviceCaps(CurRenderDevice, WW3D_DEVTYPE, &caps)))
	{
		return false;
	}

	::ZeroMemory(&CurrentAdapterIdentifier, sizeof(D3DADAPTER_IDENTIFIER8));
	D3DInterface->GetAdapterIdentifier(CurRenderDevice, 0, &CurrentAdapterIdentifier);

	Vertex_Processing_Behavior = D3DCREATE_MIXED_VERTEXPROCESSING;
	_DX8SingleThreaded = true;

	D3DPRESENT_PARAMETERS pp;
	::ZeroMemory(&pp, sizeof(pp));
	pp.BackBufferWidth = ResolutionWidth;
	pp.BackBufferHeight = ResolutionHeight;
	pp.BackBufferFormat = D3DFMT_A8R8G8B8;
	pp.BackBufferCount = 1;
	pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	pp.hDeviceWindow = _Hwnd;
	pp.Windowed = TRUE;
	pp.EnableAutoDepthStencil = TRUE;
	pp.AutoDepthStencilFormat = D3DFMT_D24S8;

	HRESULT hr = D3DInterface->CreateDevice(
		CurRenderDevice,
		WW3D_DEVTYPE,
		_Hwnd,
		Vertex_Processing_Behavior,
		&pp,
		&D3DDevice
	);

	if (FAILED(hr))
	{
		return false;
	}

	Do_Onetime_Device_Dependent_Inits();
	return true;
}

void DX8Wrapper::Release_Device() {}
bool DX8Wrapper::Reset_Device(bool reload_assets) { return true; }
// ── Enumerate_Devices (mirrors dx8wrapper.cpp lines 726-846) ──

void DX8Wrapper::Enumerate_Devices()
{
	DX8_Assert();

	int adapter_count = D3DInterface->GetAdapterCount();
	for (int adapter_index = 0; adapter_index < adapter_count; adapter_index++) {

		D3DADAPTER_IDENTIFIER8 id;
		::ZeroMemory(&id, sizeof(D3DADAPTER_IDENTIFIER8));
		HRESULT res = D3DInterface->GetAdapterIdentifier(adapter_index, 0, &id);

		if (res == D3D_OK) {

			RenderDeviceDescClass desc;
			desc.set_device_name(id.Description);
			desc.set_driver_name(id.Driver);

			char buf[64];
			sprintf(buf, "%d.%d.%d.%d",
				HIWORD(id.DriverVersion.HighPart), LOWORD(id.DriverVersion.HighPart),
				HIWORD(id.DriverVersion.LowPart), LOWORD(id.DriverVersion.LowPart));
			desc.set_driver_version(buf);

			D3DInterface->GetDeviceCaps(adapter_index, WW3D_DEVTYPE, &desc.Caps);
			D3DInterface->GetAdapterIdentifier(adapter_index, 0, &desc.AdapterIdentifier);

			DX8Caps dx8caps(D3DInterface, desc.Caps, WW3D_FORMAT_UNKNOWN, desc.AdapterIdentifier);

			desc.reset_resolution_list();
			int mode_count = D3DInterface->GetAdapterModeCount(adapter_index);
			for (int mode_index = 0; mode_index < mode_count; mode_index++) {
				D3DDISPLAYMODE d3dmode;
				::ZeroMemory(&d3dmode, sizeof(D3DDISPLAYMODE));
				HRESULT mres = D3DInterface->EnumAdapterModes(adapter_index, mode_index, &d3dmode);

				if (mres == D3D_OK) {
					int bits = 0;
					switch (d3dmode.Format) {
					case D3DFMT_R8G8B8:
					case D3DFMT_A8R8G8B8:
					case D3DFMT_X8R8G8B8:
						bits = 32;
						break;
					case D3DFMT_R5G6B5:
					case D3DFMT_X1R5G5B5:
						bits = 16;
						break;
					default:
						break;
					}

					if (!dx8caps.Is_Valid_Display_Format(d3dmode.Width, d3dmode.Height,
						D3DFormat_To_WW3DFormat(d3dmode.Format))) {
						bits = 0;
					}

					if (bits) {
						desc.add_resolution(d3dmode.Width, d3dmode.Height, bits);
					}
				}
			}

			_RenderDeviceNameTable.Add(id.Description);
			_RenderDeviceShortNameTable.Add(id.Description);
			_RenderDeviceDescriptionTable.Add(desc);
		}
	}
}

// ── Compute_Caps (mirrors dx8wrapper.cpp Compute_Caps) ──

void DX8Wrapper::Compute_Caps(WW3DFormat display_format)
{
	delete CurrentCaps;
	CurrentCaps = new DX8Caps(D3DInterface, D3DDevice, D3DFormat_To_WW3DFormat(DisplayFormat), CurrentAdapterIdentifier);
}

inline DWORD F2DW(float f) { return *((unsigned*)&f); }

void DX8Wrapper::Set_Default_Global_Render_States()
{
	DX8_THREAD_ASSERT();
	const D3DCAPS8 &caps = Get_Current_Caps()->Get_DX8_Caps();

	Set_DX8_Render_State(D3DRS_RANGEFOGENABLE, (caps.RasterCaps & D3DPRASTERCAPS_FOGRANGE) ? TRUE : FALSE);
	Set_DX8_Render_State(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
	Set_DX8_Render_State(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
	Set_DX8_Render_State(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
	Set_DX8_Render_State(D3DRS_COLORVERTEX, TRUE);
	Set_DX8_Render_State(D3DRS_ZBIAS,0);
	Set_DX8_Texture_Stage_State(1, D3DTSS_BUMPENVLSCALE, F2DW(1.0f));
	Set_DX8_Texture_Stage_State(1, D3DTSS_BUMPENVLOFFSET, F2DW(0.0f));
	Set_DX8_Texture_Stage_State(0, D3DTSS_BUMPENVMAT00,F2DW(1.0f));
	Set_DX8_Texture_Stage_State(0, D3DTSS_BUMPENVMAT01,F2DW(0.0f));
	Set_DX8_Texture_Stage_State(0, D3DTSS_BUMPENVMAT10,F2DW(0.0f));
	Set_DX8_Texture_Stage_State(0, D3DTSS_BUMPENVMAT11,F2DW(1.0f));
}

bool DX8Wrapper::Validate_Device()
{
	DWORD numPasses=0;
	HRESULT hRes = _Get_D3D_Device8()->ValidateDevice(&numPasses);
	return (hRes == D3D_OK);
}

// ── Invalidate_Cached_Render_States (mirrors dx8wrapper.cpp lines 465-496) ──

void DX8Wrapper::Invalidate_Cached_Render_States()
{
	render_state_changed=0;

	int a;
	for (a=0;a<(int)(sizeof(RenderStates)/sizeof(unsigned));++a) {
		RenderStates[a]=0x12345678;
	}
	for (a=0;a<MAX_TEXTURE_STAGES;++a)
	{
		for (int b=0; b<32;b++)
		{
			TextureStageStates[a][b]=0x12345678;
		}
		if (_Get_D3D_Device8())
			_Get_D3D_Device8()->SetTexture(a,nullptr);
		if (Textures[a] != nullptr) {
			Textures[a]->Release();
		}
		Textures[a]=nullptr;
	}

	ShaderClass::Invalidate();

	Release_Render_State();

	memset(&DX8Transforms, 0, sizeof(DX8Transforms));
}

// ── Render device selection (mirrors dx8wrapper.cpp lines 874-1377) ──

bool DX8Wrapper::Set_Any_Render_Device()
{
	int dev_number = 0;
	for (; dev_number < _RenderDeviceNameTable.Count(); dev_number++) {
		if (Set_Render_Device(dev_number,-1,-1,-1,0,false)) {
			return true;
		}
	}

	for (dev_number = 0; dev_number < _RenderDeviceNameTable.Count(); dev_number++) {
		if (Set_Render_Device(dev_number,-1,-1,-1,1,false)) {
			return true;
		}
	}

	return false;
}

bool DX8Wrapper::Set_Render_Device(const char* dev_name, int width, int height, int bits, int windowed, bool resize_window)
{
	for (int dev_number = 0; dev_number < _RenderDeviceNameTable.Count(); dev_number++) {
		if (strcmp(dev_name, _RenderDeviceNameTable[dev_number]) == 0) {
			return Set_Render_Device(dev_number, width, height, bits, windowed, resize_window);
		}
		if (strcmp(dev_name, _RenderDeviceShortNameTable[dev_number]) == 0) {
			return Set_Render_Device(dev_number, width, height, bits, windowed, resize_window);
		}
	}
	return false;
}

bool DX8Wrapper::Set_Render_Device(int dev, int width, int height, int bits, int windowed, bool resize_window, bool reset_device, bool restore_assets)
{
	WWASSERT(IsInitted);
	WWASSERT(dev >= -1);
	WWASSERT(dev < _RenderDeviceNameTable.Count());

	if ((CurRenderDevice == -1) && (dev == -1)) {
		CurRenderDevice = 0;
	} else if (dev != -1) {
		CurRenderDevice = dev;
	}

	if (width != -1) ResolutionWidth = width;
	if (height != -1) ResolutionHeight = height;

	Render2DClass::Set_Screen_Resolution( RectClass( 0, 0, ResolutionWidth, ResolutionHeight ) );
	DEBUG_RENDERING_MAC(("Set_Render_Device: res=%dx%d Render2D set", ResolutionWidth, ResolutionHeight));

	if (bits != -1) BitDepth = bits;
	if (windowed != -1) IsWindowed = (windowed != 0);
	DX8Wrapper_IsWindowed = IsWindowed;

	// Mirrors Windows dx8wrapper.cpp line 1077: resize window before device creation
	if (resize_window) {
		Resize_And_Position_Window();
	}

	WWASSERT(reset_device || D3DDevice == nullptr);

	::ZeroMemory(&_PresentParameters, sizeof(D3DPRESENT_PARAMETERS));

	_PresentParameters.BackBufferFormat = D3DFMT_A8R8G8B8;

	_PresentParameters.BackBufferWidth = ResolutionWidth;
	_PresentParameters.BackBufferHeight = ResolutionHeight;
	_PresentParameters.BackBufferCount = IsWindowed ? 1 : 2;
	_PresentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	_PresentParameters.hDeviceWindow = _Hwnd;
	_PresentParameters.Windowed = TRUE;
	_PresentParameters.EnableAutoDepthStencil = TRUE;
	_PresentParameters.Flags = 0;
	_PresentParameters.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	_PresentParameters.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	DisplayFormat = D3DFMT_A8R8G8B8;
	BitDepth = 32;
	_PresentParameters.AutoDepthStencilFormat = D3DFMT_D24S8;

	bool ret;

	if (reset_device)
	{
		ret = Reset_Device(restore_assets);
	}
	else
	{
		ret = Create_Device();
	}

	return ret;
}

bool DX8Wrapper::Set_Next_Render_Device()
{
	int new_dev = (CurRenderDevice + 1) % _RenderDeviceNameTable.Count();
	return Set_Render_Device(new_dev);
}

extern "C" void MacOS_UpdateMetalDeviceScreenSize(int width, int height);
extern "C" void MacOS_ApplyDisplayResolution(int w, int h, bool isWindowed);

static int s_windowedWidth = 800;
static int s_windowedHeight = 600;

class DX8WrapperHack : public DX8Wrapper {
public:
	static void SetWindowedState(bool w) {
		IsWindowed = w;
	}
};

extern "C" void MacOS_InitWindowedState(bool isWindowed, int xRes, int yRes) {
	DX8WrapperHack::SetWindowedState(isWindowed);
	DX8Wrapper_IsWindowed = isWindowed;
	s_windowedWidth = xRes;
	s_windowedHeight = yRes;
}

bool DX8Wrapper::Toggle_Windowed() {
	if (!_Hwnd) return false;

	NSWindow* win = (__bridge NSWindow*)_Hwnd;

	IsWindowed = !IsWindowed;
	DX8Wrapper_IsWindowed = IsWindowed;

	if (!IsWindowed) {
		// Switch to Fullscreen
		s_windowedWidth = ResolutionWidth;
		s_windowedHeight = ResolutionHeight;

		NSApp.presentationOptions = NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar;
		[win setStyleMask:NSWindowStyleMaskBorderless];
		
		NSScreen* screen = [win screen] ?: [NSScreen mainScreen];
		ResolutionWidth = (int)screen.frame.size.width;
		ResolutionHeight = (int)screen.frame.size.height;
	} else {
		// Switch to Windowed
		NSApp.presentationOptions = NSApplicationPresentationDefault;
		[win setStyleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable];
		
		ResolutionWidth = s_windowedWidth;
		ResolutionHeight = s_windowedHeight;
	}

	Resize_And_Position_Window();
	Reset_Device();

	CGFloat bsf = win.backingScaleFactor;
	MacOS_ApplyDisplayResolution(ResolutionWidth * bsf, ResolutionHeight * bsf, IsWindowed);

	return true;
}

extern "C" bool MacOS_ToggleFullscreen() {
	WW3D::Toggle_Windowed();
	return true;
}

bool DX8Wrapper::Set_Device_Resolution(int width, int height, int bits, int windowed, bool resize_window)
{
	if (D3DDevice != nullptr) {
		if (width != -1) {
			_PresentParameters.BackBufferWidth = ResolutionWidth = width;
		}
		if (height != -1) {
			_PresentParameters.BackBufferHeight = ResolutionHeight = height;
		}
		// Mirrors Windows dx8wrapper.cpp line 1367-1369
		if (resize_window) {
			Resize_And_Position_Window();
		}
		return Reset_Device();
	}
	return false;
}

// ── Resize_And_Position_Window ──
// Mirrors Windows dx8wrapper.cpp lines 960-1035.
// Windows version: GetClientRect → AdjustWindowRect → SetWindowPos.
// macOS equivalent: resize NSWindow content area → update CAMetalLayer → update MetalDevice8.

void DX8Wrapper::Resize_And_Position_Window()
{
	if (!_Hwnd) return;

	NSWindow* win = (__bridge NSWindow*)_Hwnd;
	NSView* contentView = win.contentView;
	CGFloat bsf = win.backingScaleFactor;

	printf("[DX8Wrapper] Resize_And_Position_Window: IsWindowed=%d, requested %dx%d\n",
	       IsWindowed, ResolutionWidth, ResolutionHeight);
	fflush(stdout);

	NSScreen* screen = [win screen] ?: [NSScreen mainScreen];

	if (!IsWindowed) {
		[win setFrame:screen.frame display:YES animate:NO];
		[win setLevel:NSMainMenuWindowLevel + 1];
	} else {
		[win setLevel:NSNormalWindowLevel];
		NSRect contentRect = NSMakeRect(0, 0, ResolutionWidth, ResolutionHeight);
		NSRect newFrame = [win frameRectForContentRect:contentRect];
		NSRect visibleFrame = screen.visibleFrame;

		// Clamp newFrame to visibleFrame to prevent it from going under the dock
		if (newFrame.size.width > visibleFrame.size.width) {
			newFrame.size.width = visibleFrame.size.width;
		}
		if (newFrame.size.height > visibleFrame.size.height) {
			newFrame.size.height = visibleFrame.size.height;
		}

		newFrame.origin.x = (visibleFrame.size.width - newFrame.size.width) / 2 + visibleFrame.origin.x;
		newFrame.origin.y = NSMaxY(visibleFrame) - newFrame.size.height;

		[win setFrame:newFrame display:YES animate:NO];
	}

	// 2. Update CAMetalLayer drawable size
	if (contentView.layer && [contentView.layer isKindOfClass:[CAMetalLayer class]]) {
		CAMetalLayer* layer = (CAMetalLayer*)contentView.layer;
		layer.contentsScale = bsf;
		layer.contentsGravity = kCAGravityResizeAspect; 
		layer.drawableSize = CGSizeMake(win.contentView.bounds.size.width * bsf, win.contentView.bounds.size.height * bsf);
	}

	// 3. Update MetalDevice8 screen dimensions + depth texture + viewport
	MacOS_UpdateMetalDeviceScreenSize(win.contentView.bounds.size.width * bsf, win.contentView.bounds.size.height * bsf);
}

// ── Scene / Frame (copied from dx8wrapper.cpp lines 1816-1984, DX8WebBrowser removed) ──

void DX8Wrapper::Begin_Scene()
{
	DX8_THREAD_ASSERT();
	DX8CALL(BeginScene());
}

void DX8Wrapper::End_Scene(bool flip_frames)
{
	DX8_THREAD_ASSERT();
	DX8CALL(EndScene());

	if (flip_frames) {
		DX8_Assert();
		HRESULT hr;
		{
			WWPROFILE("DX8Device::Present()");
			hr=_Get_D3D_Device8()->Present(nullptr, nullptr, nullptr, nullptr);
		}

		number_of_DX8_calls++;

		if (SUCCEEDED(hr)) {
			IsDeviceLost=false;
			FrameCount++;
		}
		else {
			IsDeviceLost=true;
		}

		if (hr==D3DERR_DEVICELOST) {
			hr=_Get_D3D_Device8()->TestCooperativeLevel();
			if (hr==D3DERR_DEVICENOTRESET) {
				WWDEBUG_SAY(("DX8Wrapper::End_Scene is resetting the device."));
				Reset_Device();
			}
			else {
				ThreadClass::Sleep_Ms(200);
			}
		}
		else {
			DX8_ErrorCode(hr);
		}
	}

	Set_Vertex_Buffer(nullptr);
	Set_Index_Buffer(nullptr,0);
	for (int i=0;i<CurrentCaps->Get_Max_Textures_Per_Pass();++i) Set_Texture(i,nullptr);
	Set_Material(nullptr);
}

void DX8Wrapper::Flip_To_Primary() {}

void DX8Wrapper::Clear(bool clear_color, bool clear_z_stencil, const Vector3 &color, float dest_alpha, float z, unsigned int stencil)
{
	DX8_THREAD_ASSERT();

	bool has_stencil=false;
	IDirect3DSurface8* depthbuffer;

	_Get_D3D_Device8()->GetDepthStencilSurface(&depthbuffer);
	number_of_DX8_calls++;

	if (depthbuffer)
	{
		D3DSURFACE_DESC desc;
		depthbuffer->GetDesc(&desc);
		has_stencil=
		(
			desc.Format==D3DFMT_D15S1 ||
			desc.Format==D3DFMT_D24S8 ||
			desc.Format==D3DFMT_D24X4S4
		);

		depthbuffer->Release();
	}

	DWORD flags = 0;
	if (clear_color) flags |= D3DCLEAR_TARGET;
	if (clear_z_stencil) flags |= D3DCLEAR_ZBUFFER;
	if (clear_z_stencil && has_stencil) flags |= D3DCLEAR_STENCIL;
	if (flags)
	{
		DX8CALL(Clear(0, nullptr, flags, Convert_Color(color,dest_alpha), z, stencil));
	}
}

void DX8Wrapper::Set_Viewport(CONST D3DVIEWPORT8* pViewport)
{
	DX8_THREAD_ASSERT();
	DX8CALL(SetViewport(pViewport));
}

// ── Vertex / Index Buffers (copied from dx8wrapper.cpp lines 1994-2079) ──

void DX8Wrapper::Set_Vertex_Buffer(const VertexBufferClass* vb, unsigned stream)
{
	render_state.vba_offset=0;
	render_state.vba_count=0;
	if (render_state.vertex_buffers[stream]) {
		render_state.vertex_buffers[stream]->Release_Engine_Ref();
	}
	REF_PTR_SET(render_state.vertex_buffers[stream],const_cast<VertexBufferClass*>(vb));
	if (vb) {
		vb->Add_Engine_Ref();
		render_state.vertex_buffer_types[stream]=vb->Type();
	}
	else {
		render_state.vertex_buffer_types[stream]=BUFFER_TYPE_INVALID;
	}
	render_state_changed|=VERTEX_BUFFER_CHANGED;
}

void DX8Wrapper::Set_Index_Buffer(const IndexBufferClass* ib,unsigned short index_base_offset)
{
	render_state.iba_offset=0;
	if (render_state.index_buffer) {
		render_state.index_buffer->Release_Engine_Ref();
	}
	REF_PTR_SET(render_state.index_buffer,const_cast<IndexBufferClass*>(ib));
	render_state.index_base_offset=index_base_offset;
	if (ib) {
		ib->Add_Engine_Ref();
		render_state.index_buffer_type=ib->Type();
	}
	else {
		render_state.index_buffer_type=BUFFER_TYPE_INVALID;
	}
	render_state_changed|=INDEX_BUFFER_CHANGED;
}

void DX8Wrapper::Set_Vertex_Buffer(const DynamicVBAccessClass& vba_)
{
	for (int i=1;i<MAX_VERTEX_STREAMS;++i) {
		DX8Wrapper::Set_Vertex_Buffer(nullptr, i);
	}

	if (render_state.vertex_buffers[0]) render_state.vertex_buffers[0]->Release_Engine_Ref();
	DynamicVBAccessClass& vba=const_cast<DynamicVBAccessClass&>(vba_);
	render_state.vertex_buffer_types[0]=vba.Get_Type();
	render_state.vba_offset=vba.VertexBufferOffset;
	render_state.vba_count=vba.Get_Vertex_Count();
	REF_PTR_SET(render_state.vertex_buffers[0],vba.VertexBuffer);
	render_state.vertex_buffers[0]->Add_Engine_Ref();
	render_state_changed|=VERTEX_BUFFER_CHANGED;
	render_state_changed|=INDEX_BUFFER_CHANGED;
}

void DX8Wrapper::Set_Index_Buffer(const DynamicIBAccessClass& iba_,unsigned short index_base_offset)
{
	if (render_state.index_buffer) render_state.index_buffer->Release_Engine_Ref();

	DynamicIBAccessClass& iba=const_cast<DynamicIBAccessClass&>(iba_);
	render_state.index_base_offset=index_base_offset;
	render_state.index_buffer_type=iba.Get_Type();
	render_state.iba_offset=iba.IndexBufferOffset;
	REF_PTR_SET(render_state.index_buffer,iba.IndexBuffer);
	render_state.index_buffer->Add_Engine_Ref();
	render_state_changed|=INDEX_BUFFER_CHANGED;
}

// ── Draw calls (copied from dx8wrapper.cpp lines 2088-2358) ──

void DX8Wrapper::Draw_Sorting_IB_VB(
	unsigned primitive_type,
	unsigned short start_index, unsigned short polygon_count,
	unsigned short min_vertex_index,
	unsigned short vertex_count)
{
	WWASSERT(render_state.vertex_buffer_types[0]==BUFFER_TYPE_SORTING || render_state.vertex_buffer_types[0]==BUFFER_TYPE_DYNAMIC_SORTING);
	WWASSERT(render_state.index_buffer_type==BUFFER_TYPE_SORTING || render_state.index_buffer_type==BUFFER_TYPE_DYNAMIC_SORTING);

	DynamicVBAccessClass dyn_vb_access(BUFFER_TYPE_DYNAMIC_DX8,dynamic_fvf_type,vertex_count);
	{
		DynamicVBAccessClass::WriteLockClass lock(&dyn_vb_access);
		VertexFormatXYZNDUV2* src = static_cast<SortingVertexBufferClass*>(render_state.vertex_buffers[0])->VertexBuffer;
		VertexFormatXYZNDUV2* dest= lock.Get_Formatted_Vertex_Array();
		src += render_state.vba_offset + render_state.index_base_offset + min_vertex_index;
		unsigned  size = dyn_vb_access.FVF_Info().Get_FVF_Size()*vertex_count/sizeof(unsigned);
		unsigned *dest_u =(unsigned*) dest;
		unsigned *src_u = (unsigned*) src;

		for (unsigned i=0;i<size;++i) {
			*dest_u++=*src_u++;
		}
	}

	DX8CALL(SetStreamSource(
		0,
		static_cast<DX8VertexBufferClass*>(dyn_vb_access.VertexBuffer)->Get_DX8_Vertex_Buffer(),
		dyn_vb_access.FVF_Info().Get_FVF_Size()));
	unsigned fvf=dyn_vb_access.FVF_Info().Get_FVF();
	if (fvf!=0) {
		DX8CALL(SetVertexShader(fvf));
	}
	DX8_RECORD_VERTEX_BUFFER_CHANGE();

	unsigned index_count=0;
	switch (primitive_type) {
	case D3DPT_TRIANGLELIST: index_count=polygon_count*3; break;
	case D3DPT_TRIANGLESTRIP: index_count=polygon_count+2; break;
	case D3DPT_TRIANGLEFAN: index_count=polygon_count+2; break;
	default: WWASSERT(0); break;
	}

	DynamicIBAccessClass dyn_ib_access(BUFFER_TYPE_DYNAMIC_DX8,index_count);
	{
		DynamicIBAccessClass::WriteLockClass lock(&dyn_ib_access);
		unsigned short* dest=lock.Get_Index_Array();
		unsigned short* src=nullptr;
		src=static_cast<SortingIndexBufferClass*>(render_state.index_buffer)->index_buffer;
		src+=render_state.iba_offset+start_index;

		for (unsigned short i=0;i<index_count;++i) {
			unsigned short index=*src++;
			index-=min_vertex_index;
			WWASSERT(index<vertex_count);
			*dest++=index;
		}
	}

	DX8CALL(SetIndices(
		static_cast<DX8IndexBufferClass*>(dyn_ib_access.IndexBuffer)->Get_DX8_Index_Buffer(),
		dyn_vb_access.VertexBufferOffset));
	DX8_RECORD_INDEX_BUFFER_CHANGE();

	DX8_RECORD_DRAW_CALLS();
	DX8CALL(DrawIndexedPrimitive(
		D3DPT_TRIANGLELIST,
		0,
		vertex_count,
		dyn_ib_access.IndexBufferOffset,
		polygon_count));

	DX8_RECORD_RENDER(polygon_count,vertex_count,render_state.shader);
}

void DX8Wrapper::Draw(
	unsigned primitive_type,
	unsigned short start_index, unsigned short polygon_count,
	unsigned short min_vertex_index, unsigned short vertex_count)
{
	if (DrawPolygonLowBoundLimit && DrawPolygonLowBoundLimit>=polygon_count) return;

	DX8_THREAD_ASSERT();

	Apply_Render_State_Changes();

	if (!_Is_Triangle_Draw_Enabled()) return;

	if (vertex_count<3) {
		min_vertex_index=0;
		switch (render_state.vertex_buffer_types[0]) {
		case BUFFER_TYPE_DX8:
		case BUFFER_TYPE_SORTING:
			vertex_count=render_state.vertex_buffers[0]->Get_Vertex_Count()-render_state.index_base_offset-render_state.vba_offset-min_vertex_index;
			break;
		case BUFFER_TYPE_DYNAMIC_DX8:
		case BUFFER_TYPE_DYNAMIC_SORTING:
			vertex_count=render_state.vba_count;
			break;
		}
	}

	switch (render_state.vertex_buffer_types[0]) {
	case BUFFER_TYPE_DX8:
	case BUFFER_TYPE_DYNAMIC_DX8:
		switch (render_state.index_buffer_type) {
		case BUFFER_TYPE_DX8:
		case BUFFER_TYPE_DYNAMIC_DX8:
			{
				DX8_RECORD_RENDER(polygon_count,vertex_count,render_state.shader);
				DX8_RECORD_DRAW_CALLS();
				DX8CALL(DrawIndexedPrimitive(
					(D3DPRIMITIVETYPE)primitive_type,
					min_vertex_index,
					vertex_count,
					start_index+render_state.iba_offset,
					polygon_count));
			}
			break;
		case BUFFER_TYPE_SORTING:
		case BUFFER_TYPE_DYNAMIC_SORTING:
			WWASSERT_PRINT(0,"VB and IB must of same type (sorting or dx8)");
			break;
		case BUFFER_TYPE_INVALID:
			WWASSERT(0);
			break;
		}
		break;
	case BUFFER_TYPE_SORTING:
	case BUFFER_TYPE_DYNAMIC_SORTING:
		switch (render_state.index_buffer_type) {
		case BUFFER_TYPE_DX8:
		case BUFFER_TYPE_DYNAMIC_DX8:
			WWASSERT_PRINT(0,"VB and IB must of same type (sorting or dx8)");
			break;
		case BUFFER_TYPE_SORTING:
		case BUFFER_TYPE_DYNAMIC_SORTING:
			Draw_Sorting_IB_VB(primitive_type,start_index,polygon_count,min_vertex_index,vertex_count);
			break;
		case BUFFER_TYPE_INVALID:
			WWASSERT(0);
			break;
		}
		break;
	case BUFFER_TYPE_INVALID:
		WWASSERT(0);
		break;
	}
}

void DX8Wrapper::Draw_Triangles(
	unsigned buffer_type,
	unsigned short start_index, unsigned short polygon_count,
	unsigned short min_vertex_index, unsigned short vertex_count)
{
	if (buffer_type==BUFFER_TYPE_SORTING || buffer_type==BUFFER_TYPE_DYNAMIC_SORTING) {
		SortingRendererClass::Insert_Triangles(start_index,polygon_count,min_vertex_index,vertex_count);
	}
	else {
		Draw(D3DPT_TRIANGLELIST,start_index,polygon_count,min_vertex_index,vertex_count);
	}
}

void DX8Wrapper::Draw_Triangles(
	unsigned short start_index, unsigned short polygon_count,
	unsigned short min_vertex_index, unsigned short vertex_count)
{
	Draw(D3DPT_TRIANGLELIST,start_index,polygon_count,min_vertex_index,vertex_count);
}

void DX8Wrapper::Draw_Strip(
	unsigned short start_index, unsigned short polygon_count,
	unsigned short min_vertex_index, unsigned short num_vertices)
{
	Draw(D3DPT_TRIANGLESTRIP,start_index,polygon_count,min_vertex_index,num_vertices);
}

// ── Apply_Render_State_Changes (copied from dx8wrapper.cpp lines 2366-2509) ──

void DX8Wrapper::Apply_Render_State_Changes()
{
	if (!render_state_changed) return;
	if (render_state_changed&SHADER_CHANGED) {
		render_state.shader.Apply();
	}

	unsigned mask=TEXTURE0_CHANGED;
	int i=0;
	for (;i<CurrentCaps->Get_Max_Textures_Per_Pass();++i,mask<<=1)
	{
		if (render_state_changed&mask)
		{
			if (render_state.Textures[i])
			{
				render_state.Textures[i]->Apply(i);
			}
			else
			{
				TextureBaseClass::Apply_Null(i);
			}
		}
	}

	if (render_state_changed&MATERIAL_CHANGED)
	{
		VertexMaterialClass* material=const_cast<VertexMaterialClass*>(render_state.material);
		if (material)
		{
			material->Apply();
		}
		else VertexMaterialClass::Apply_Null();
	}

	if (render_state_changed&LIGHTS_CHANGED)
	{
		unsigned mask=LIGHT0_CHANGED;
		for (unsigned index=0;index<4;++index,mask<<=1) {
			if (render_state_changed&mask) {
				if (render_state.LightEnable[index]) {
					Set_DX8_Light(index,&render_state.Lights[index]);
				}
				else {
					Set_DX8_Light(index,nullptr);
				}
			}
		}
	}

	if (render_state_changed&WORLD_CHANGED) {
		_Set_DX8_Transform(D3DTS_WORLD,render_state.world);
	}
	if (render_state_changed&VIEW_CHANGED) {
		_Set_DX8_Transform(D3DTS_VIEW,render_state.view);
	}
	if (render_state_changed&VERTEX_BUFFER_CHANGED) {
		for (i=0;i<MAX_VERTEX_STREAMS;++i) {
			if (render_state.vertex_buffers[i]) {
				switch (render_state.vertex_buffer_types[i]) {
				case BUFFER_TYPE_DX8:
				case BUFFER_TYPE_DYNAMIC_DX8:
					DX8CALL(SetStreamSource(
						i,
						static_cast<DX8VertexBufferClass*>(render_state.vertex_buffers[i])->Get_DX8_Vertex_Buffer(),
						render_state.vertex_buffers[i]->FVF_Info().Get_FVF_Size()));
					DX8_RECORD_VERTEX_BUFFER_CHANGE();
					{
						unsigned fvf=render_state.vertex_buffers[i]->FVF_Info().Get_FVF();
						if (fvf!=0) {
							Set_Vertex_Shader(fvf);
						}
					}
					break;
				case BUFFER_TYPE_SORTING:
				case BUFFER_TYPE_DYNAMIC_SORTING:
					break;
				default:
					WWASSERT(0);
				}
			} else {
				DX8CALL(SetStreamSource(i,nullptr,0));
				DX8_RECORD_VERTEX_BUFFER_CHANGE();
			}
		}
	}
	if (render_state_changed&INDEX_BUFFER_CHANGED) {
		if (render_state.index_buffer) {
			switch (render_state.index_buffer_type) {
			case BUFFER_TYPE_DX8:
			case BUFFER_TYPE_DYNAMIC_DX8:
				DX8CALL(SetIndices(
					static_cast<DX8IndexBufferClass*>(render_state.index_buffer)->Get_DX8_Index_Buffer(),
					render_state.index_base_offset+render_state.vba_offset));
				DX8_RECORD_INDEX_BUFFER_CHANGE();
				break;
			case BUFFER_TYPE_SORTING:
			case BUFFER_TYPE_DYNAMIC_SORTING:
				break;
			default:
				WWASSERT(0);
			}
		}
		else {
			DX8CALL(SetIndices(
				nullptr,
				0));
			DX8_RECORD_INDEX_BUFFER_CHANGE();
		}
	}

	render_state_changed&=((unsigned)WORLD_IDENTITY|(unsigned)VIEW_IDENTITY);
}

// ── Apply_Default_State (mirrors dx8wrapper.cpp lines 3882-4027) ──
void DX8Wrapper::Apply_Default_State()
{
	// only set states used in game
	Set_DX8_Render_State(D3DRS_ZENABLE, TRUE);
	Set_DX8_Render_State(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	Set_DX8_Render_State(D3DRS_ZWRITEENABLE, TRUE);
	Set_DX8_Render_State(D3DRS_ALPHATESTENABLE, FALSE);
	Set_DX8_Render_State(D3DRS_SRCBLEND, D3DBLEND_ONE);
	Set_DX8_Render_State(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	Set_DX8_Render_State(D3DRS_CULLMODE, D3DCULL_CW);
	Set_DX8_Render_State(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	Set_DX8_Render_State(D3DRS_ALPHAREF, 0);
	Set_DX8_Render_State(D3DRS_ALPHAFUNC, D3DCMP_LESSEQUAL);
	Set_DX8_Render_State(D3DRS_DITHERENABLE, FALSE);
	Set_DX8_Render_State(D3DRS_ALPHABLENDENABLE, FALSE);
	Set_DX8_Render_State(D3DRS_FOGENABLE, FALSE);
	Set_DX8_Render_State(D3DRS_SPECULARENABLE, FALSE);
	Set_DX8_Render_State(D3DRS_ZBIAS, 0);
	Set_DX8_Render_State(D3DRS_STENCILENABLE, FALSE);
	Set_DX8_Render_State(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
	Set_DX8_Render_State(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
	Set_DX8_Render_State(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
	Set_DX8_Render_State(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
	Set_DX8_Render_State(D3DRS_STENCILREF, 0);
	Set_DX8_Render_State(D3DRS_STENCILMASK, 0xffffffff);
	Set_DX8_Render_State(D3DRS_STENCILWRITEMASK, 0xffffffff);
	Set_DX8_Render_State(D3DRS_TEXTUREFACTOR, 0);
	Set_DX8_Render_State(D3DRS_CLIPPING, TRUE);
	Set_DX8_Render_State(D3DRS_LIGHTING, FALSE);
	Set_DX8_Render_State(D3DRS_COLORVERTEX, TRUE);
	Set_DX8_Render_State(D3DRS_COLORWRITEENABLE, 0x0000000f);
	Set_DX8_Render_State(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	Set_DX8_Render_State(D3DRS_SOFTWAREVERTEXPROCESSING, FALSE);

	// disable TSS stages
	int i;
	for (i=0; i<CurrentCaps->Get_Max_Textures_Per_Pass(); i++)
	{
		Set_DX8_Texture_Stage_State(i, D3DTSS_COLOROP, D3DTOP_DISABLE);
		Set_DX8_Texture_Stage_State(i, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		Set_DX8_Texture_Stage_State(i, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		Set_DX8_Texture_Stage_State(i, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		Set_DX8_Texture_Stage_State(i, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		Set_DX8_Texture_Stage_State(i, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		Set_DX8_Texture_Stage_State(i, D3DTSS_TEXCOORDINDEX, i);
		Set_DX8_Texture_Stage_State(i, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
		Set_DX8_Texture_Stage_State(i, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
		Set_DX8_Texture_Stage_State(i, D3DTSS_BORDERCOLOR, 0);
		Set_DX8_Texture_Stage_State(i, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
		Set_Texture(i, nullptr);
	}

	VertexMaterialClass::Apply_Null();

	for (unsigned index=0; index<4; ++index) {
		Set_DX8_Light(index, nullptr);
	}

	// set up simple default TSS
	Vector4 vconst[MAX_VERTEX_SHADER_CONSTANTS];
	memset(vconst, 0, sizeof(Vector4)*MAX_VERTEX_SHADER_CONSTANTS);
	Set_Vertex_Shader_Constant(0, vconst, MAX_VERTEX_SHADER_CONSTANTS);

	Vector4 pconst[MAX_PIXEL_SHADER_CONSTANTS];
	memset(pconst, 0, sizeof(Vector4)*MAX_PIXEL_SHADER_CONSTANTS);
	Set_Pixel_Shader_Constant(0, pconst, MAX_PIXEL_SHADER_CONSTANTS);

	Set_Vertex_Shader(DX8_FVF_XYZNDUV2);
	Set_Pixel_Shader(0);

	ShaderClass::Invalidate();
}

// ── Render state / Material ──
// Set_Render_State, Set_DX8_Material, Set_DX8_Render_State, Set_DX8_Texture_Stage_State,
// Set_DX8_Texture, Set_DX8_Clip_Plane, Set_Shader, Set_Transform, Get_Render_State,
// Release_Render_State — all defined WWINLINE in dx8wrapper.h

void DX8Wrapper::Set_Polygon_Mode(int mode) {}

// ── Lights / Fog ──
// Set_DX8_Light, Set_Fog, Set_Ambient, Set_DX8_ZBias, Set_Projection_Transform_With_Z_Bias,
// Get_Transform, Is_World_Identity, Is_View_Identity,
// Convert_Color (x4), Clamp_Color, Convert_Color_Clamp, Set_Alpha, _Copy_DX8_Rects
// — all defined WWINLINE in dx8wrapper.h

// ── Set_World_Identity (mirrors dx8wrapper.cpp lines 3862-3868) ──
void DX8Wrapper::Set_World_Identity()
{
	if (render_state_changed&(unsigned)WORLD_IDENTITY)
		return;
	// D3DMatrixIdentity equivalent
	memset(&render_state.world, 0, sizeof(render_state.world));
	render_state.world._11 = 1.0f;
	render_state.world._22 = 1.0f;
	render_state.world._33 = 1.0f;
	render_state.world._44 = 1.0f;
	render_state_changed|=(unsigned)WORLD_CHANGED|(unsigned)WORLD_IDENTITY;
}

// ── Set_View_Identity (mirrors dx8wrapper.cpp lines 3870-3876) ──
void DX8Wrapper::Set_View_Identity()
{
	if (render_state_changed&(unsigned)VIEW_IDENTITY)
		return;
	memset(&render_state.view, 0, sizeof(render_state.view));
	render_state.view._11 = 1.0f;
	render_state.view._22 = 1.0f;
	render_state.view._33 = 1.0f;
	render_state.view._44 = 1.0f;
	render_state_changed|=(unsigned)VIEW_CHANGED|(unsigned)VIEW_IDENTITY;
}

// ── Set_Light (mirrors dx8wrapper.cpp lines 3116-3126) ──
void DX8Wrapper::Set_Light(unsigned int index, const _D3DLIGHT8* light)
{
	if (light) {
		render_state.Lights[index] = *light;
		render_state.LightEnable[index] = true;
	} else {
		render_state.LightEnable[index] = false;
	}
	render_state_changed |= (LIGHT0_CHANGED << index);
}

void DX8_Assert() {}
void Log_DX8_ErrorCode(unsigned int code) {}
void Non_Fatal_Log_DX8_ErrorCode(unsigned res, const char* file, int line) {}
// — all defined WWINLINE in dx8wrapper.h

void DX8Wrapper::Set_Light_Environment(LightEnvironmentClass* light_env) 
{ 
	Light_Environment = light_env; 

	if (light_env)
	{
		int light_count = light_env->Get_Light_Count();
		unsigned int color=Convert_Color(light_env->Get_Equivalent_Ambient(),0.0f);
		if (RenderStates[D3DRS_AMBIENT]!=color)
		{
			Set_DX8_Render_State(D3DRS_AMBIENT,color);
			render_state_changed|=MATERIAL_CHANGED;
		}

		_D3DLIGHT8 light;
		int l=0;
		for (;l<light_count;++l) {
			::ZeroMemory(&light, sizeof(_D3DLIGHT8));

			light.Type=D3DLIGHT_DIRECTIONAL;
			(Vector3&)light.Diffuse=light_env->Get_Light_Diffuse(l);
			Vector3 dir=-light_env->Get_Light_Direction(l);
			light.Direction=(const D3DVECTOR&)(dir);

			if (l==0) {
				light.Specular.r = light.Specular.g = light.Specular.b = 1.0f;
			}

			if (light_env->isPointLight(l)) {
				light.Type = D3DLIGHT_POINT;
				(Vector3&)light.Diffuse=light_env->getPointDiffuse(l);
				(Vector3&)light.Ambient=light_env->getPointAmbient(l);
				light.Position = (const D3DVECTOR&)light_env->getPointCenter(l);
				light.Range = light_env->getPointOrad(l);

				double a,b;
				b = light_env->getPointOrad(l);
				a = light_env->getPointIrad(l);

				light.Attenuation0=1.0f;
				if (fabs(a-b)<1e-5 || a < 1e-5)
					light.Attenuation1=0.0f;
				else
					light.Attenuation1=(float) 0.1/a;

				light.Attenuation2=8.0f/(b*b);
			}

			Set_Light(l,&light);
		}

		for (;l<4;++l) {
			Set_Light(l,nullptr);
		}
	} else {
		Set_DX8_Render_State(D3DRS_AMBIENT,0);
		for (int l=0; l<4; ++l) {
			Set_Light(l,nullptr);
		}
	}
}
// ── Set_Gamma (mirrors dx8wrapper.cpp lines 3801-3848) ──
void DX8Wrapper::Set_Gamma(float gamma, float bright, float contrast, bool calibrate, bool uselimit)
{
	gamma = Bound(gamma, 0.6f, 6.0f);
	bright = Bound(bright, -0.5f, 0.5f);
	contrast = Bound(contrast, 0.5f, 2.0f);
	float oo_gamma = 1.0f / gamma;

	D3DGAMMARAMP ramp;
	float limit;

	if (uselimit) {
		limit = (contrast - 1) / 2 * contrast;
	} else {
		limit = 0.0f;
	}

	for (int i = 0; i < 256; i++) {
		float in, out;
		in = i / 256.0f;
		float x = in - limit;
		x = Bound(x, 0.0f, 1.0f);
		x = powf(x, oo_gamma);
		out = contrast * x + bright;
		out = Bound(out, 0.0f, 1.0f);
		ramp.red[i] = (WORD)(out * 65535);
		ramp.green[i] = (WORD)(out * 65535);
		ramp.blue[i] = (WORD)(out * 65535);
	}

	_Get_D3D_Device8()->SetGammaRamp(0, &ramp);
}

// ── Texture creation (mirrors dx8wrapper.cpp lines 2511-2940) ──

IDirect3DTexture8* DX8Wrapper::_Create_DX8_Texture(unsigned int width, unsigned int height, WW3DFormat format, MipCountType mip_level_count, D3DPOOL pool, bool rendertarget)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();
	IDirect3DTexture8* texture = nullptr;

	WWASSERT(format!=D3DFMT_P8);

	if (rendertarget) {
		unsigned ret = D3DXCreateTexture(
			_Get_D3D_Device8(), width, height, mip_level_count,
			D3DUSAGE_RENDERTARGET, WW3DFormat_To_D3DFormat(format), pool, &texture);

		if (ret == D3DERR_NOTAVAILABLE) {
			Non_Fatal_Log_DX8_ErrorCode(ret, __FILE__, __LINE__);
			return nullptr;
		}

		if (ret == D3DERR_OUTOFVIDEOMEMORY) {
			TextureClass::Invalidate_Old_Unused_Textures(5000);
			WW3D::_Invalidate_Mesh_Cache();

			ret = D3DXCreateTexture(
				_Get_D3D_Device8(), width, height, mip_level_count,
				D3DUSAGE_RENDERTARGET, WW3DFormat_To_D3DFormat(format), pool, &texture);

			if (ret == D3DERR_OUTOFVIDEOMEMORY) {
				Non_Fatal_Log_DX8_ErrorCode(ret, __FILE__, __LINE__);
				return nullptr;
			}
		}

		DX8_ErrorCode(ret);
		return texture;
	}

	unsigned ret = D3DXCreateTexture(
		_Get_D3D_Device8(), width, height, mip_level_count,
		0, WW3DFormat_To_D3DFormat(format), pool, &texture);

	if (ret == D3DERR_OUTOFVIDEOMEMORY) {
		TextureClass::Invalidate_Old_Unused_Textures(5000);
		WW3D::_Invalidate_Mesh_Cache();

		ret = D3DXCreateTexture(
			_Get_D3D_Device8(), width, height, mip_level_count,
			0, WW3DFormat_To_D3DFormat(format), pool, &texture);
	}
	DX8_ErrorCode(ret);
	return texture;
}

IDirect3DTexture8* DX8Wrapper::_Create_DX8_Texture(const char* filename, MipCountType mip_level_count)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();
	IDirect3DTexture8* texture = nullptr;

	unsigned result = D3DXCreateTextureFromFileExA(
		_Get_D3D_Device8(), filename,
		D3DX_DEFAULT, D3DX_DEFAULT, mip_level_count,
		0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
		D3DX_FILTER_BOX, D3DX_FILTER_BOX, 0,
		nullptr, nullptr, &texture);

	if (result != D3D_OK) {
		return MissingTexture::_Get_Missing_Texture();
	}

	D3DSURFACE_DESC desc;
	texture->GetLevelDesc(0, &desc);
	if (desc.Format == D3DFMT_P8) {
		texture->Release();
		return MissingTexture::_Get_Missing_Texture();
	}
	return texture;
}

HRESULT WINAPI D3DXLoadSurfaceFromSurface(
    IDirect3DSurface8* pDestSurface, const void* pDestPalette, const RECT* pDestRect,
    IDirect3DSurface8* pSrcSurface, const void* pSrcPalette, const RECT* pSrcRect,
    DWORD Filter, DWORD ColorKey)
{
    D3DLOCKED_RECT srcLR, destLR;
    if (pSrcSurface->LockRect(&srcLR, pSrcRect, 0) == D3D_OK) {
        if (pDestSurface->LockRect(&destLR, pDestRect, 0) == D3D_OK) {
            D3DSURFACE_DESC srcDesc, destDesc;
            pSrcSurface->GetDesc(&srcDesc);
            pDestSurface->GetDesc(&destDesc);
            
            unsigned int bpp = 4;
            if (destDesc.Format == D3DFMT_A8R8G8B8 || destDesc.Format == D3DFMT_X8R8G8B8) bpp = 4;
            else if (destDesc.Format == D3DFMT_R5G6B5 || destDesc.Format == D3DFMT_A1R5G5B5 || destDesc.Format == D3DFMT_A4R4G4B4 || destDesc.Format == D3DFMT_X1R5G5B5) bpp = 2;
            else if (destDesc.Format == D3DFMT_A8 || destDesc.Format == D3DFMT_L8 || destDesc.Format == D3DFMT_P8) bpp = 1;
            
            unsigned int copyWidth = pDestRect ? (pDestRect->right - pDestRect->left) : destDesc.Width;
            unsigned int copyHeight = pDestRect ? (pDestRect->bottom - pDestRect->top) : destDesc.Height;
            
            if (pSrcRect) {
                copyWidth = std::min<unsigned int>(copyWidth, pSrcRect->right - pSrcRect->left);
                copyHeight = std::min<unsigned int>(copyHeight, pSrcRect->bottom - pSrcRect->top);
            }
            
            // Note: Since MetalSurface8 natively ignores pRect in LockRect (returning base pointer),
            // we must manually offset our starting read/write pointers here just in case.
            // D3D allows LockRect to return an offset pointer, but we do it manually to be safe.
            unsigned int dstOffX = pDestRect ? pDestRect->left : 0;
            unsigned int dstOffY = pDestRect ? pDestRect->top : 0;
            unsigned int srcOffX = pSrcRect ? pSrcRect->left : 0;
            unsigned int srcOffY = pSrcRect ? pSrcRect->top : 0;
            
            for (unsigned int y = 0; y < copyHeight; ++y) {
                memcpy((char*)destLR.pBits + (dstOffY + y) * destLR.Pitch + (dstOffX * bpp),
                       (char*)srcLR.pBits + (srcOffY + y) * srcLR.Pitch + (srcOffX * bpp), copyWidth * bpp);
            }
            pDestSurface->UnlockRect();
        }
        pSrcSurface->UnlockRect();
    }
    return D3D_OK;
}

HRESULT WINAPI D3DXFilterTexture(
    IDirect3DTexture8* pTexture, const void* pPalette, UINT SrcLevel, DWORD Filter)
{
    if (!pTexture) return E_POINTER;
    
    // We strictly use MetalTexture8 in this port
    MetalTexture8* mtlTex = static_cast<MetalTexture8*>(pTexture);
    id<MTLTexture> tex = mtlTex->GetMTLTexture();
    
    if (!tex || tex.mipmapLevelCount <= 1) {
        return D3D_OK; // No mipmaps to generate
    }
    
    id<MTLDevice> device = tex.device;
    id<MTLCommandQueue> queue = [device newCommandQueue];
    if (!queue) return E_FAIL;
    
    id<MTLCommandBuffer> cmdBuf = [queue commandBuffer];
    if (!cmdBuf) return E_FAIL;
    
    id<MTLBlitCommandEncoder> blit = [cmdBuf blitCommandEncoder];
    [blit generateMipmapsForTexture:tex];
    [blit endEncoding];
    
    [cmdBuf commit];
    [cmdBuf waitUntilCompleted];
    
    return D3D_OK;
}

IDirect3DTexture8* DX8Wrapper::_Create_DX8_Texture(IDirect3DSurface8* surface, MipCountType mip_level_count)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();

	D3DSURFACE_DESC surface_desc;
	::ZeroMemory(&surface_desc, sizeof(D3DSURFACE_DESC));
	surface->GetDesc(&surface_desc);

	WW3DFormat format = D3DFormat_To_WW3DFormat(surface_desc.Format);
	IDirect3DTexture8* texture = _Create_DX8_Texture(surface_desc.Width, surface_desc.Height, format, mip_level_count);

	IDirect3DSurface8* tex_surface = nullptr;
	texture->GetSurfaceLevel(0, &tex_surface);
	DX8_ErrorCode(D3DXLoadSurfaceFromSurface(tex_surface, nullptr, nullptr, surface, nullptr, nullptr, D3DX_FILTER_BOX, 0));
	tex_surface->Release();

	if (mip_level_count != MIP_LEVELS_1) {
		DX8_ErrorCode(D3DXFilterTexture(texture, nullptr, 0, D3DX_FILTER_BOX));
	}

	return texture;
}

void DX8Wrapper::_Update_Texture(TextureClass* system, TextureClass* video)
{
	WWASSERT(system);
	WWASSERT(video);
	WWASSERT(system->Get_Pool() == TextureClass::POOL_SYSTEMMEM);
	WWASSERT(video->Get_Pool() == TextureClass::POOL_DEFAULT);
	DX8CALL(UpdateTexture(system->Peek_D3D_Base_Texture(), video->Peek_D3D_Base_Texture()));
}

IDirect3DTexture8* DX8Wrapper::_Create_DX8_ZTexture(unsigned int width, unsigned int height, WW3DZFormat zformat, MipCountType mip_level_count, _D3DPOOL pool)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();
	IDirect3DTexture8* texture = nullptr;

	D3DFORMAT zfmt = WW3DZFormat_To_D3DFormat(zformat);

	unsigned ret = _Get_D3D_Device8()->CreateTexture(
		width, height, mip_level_count, D3DUSAGE_DEPTHSTENCIL, zfmt, pool, &texture);

	if (ret == D3DERR_NOTAVAILABLE) {
		Non_Fatal_Log_DX8_ErrorCode(ret, __FILE__, __LINE__);
		return nullptr;
	}

	if (ret == D3DERR_OUTOFVIDEOMEMORY) {
		TextureClass::Invalidate_Old_Unused_Textures(5000);
		WW3D::_Invalidate_Mesh_Cache();

		ret = _Get_D3D_Device8()->CreateTexture(
			width, height, mip_level_count, D3DUSAGE_DEPTHSTENCIL, zfmt, pool, &texture);

		if (ret == D3DERR_OUTOFVIDEOMEMORY) {
			Non_Fatal_Log_DX8_ErrorCode(ret, __FILE__, __LINE__);
			return nullptr;
		}
	}

	DX8_ErrorCode(ret);
	if (texture) texture->AddRef();
	return texture;
}

IDirect3DCubeTexture8* DX8Wrapper::_Create_DX8_Cube_Texture(unsigned int width, unsigned int height, WW3DFormat format, MipCountType mip_level_count, _D3DPOOL pool, bool rendertarget)
{
	WWASSERT(width == height);
	DX8_THREAD_ASSERT();
	DX8_Assert();
	IDirect3DCubeTexture8* texture = nullptr;

	WWASSERT(format != D3DFMT_P8);

	DWORD usage = rendertarget ? D3DUSAGE_RENDERTARGET : 0;

	unsigned ret = D3DXCreateCubeTexture(
		_Get_D3D_Device8(), width, mip_level_count,
		usage, WW3DFormat_To_D3DFormat(format), pool, &texture);

	if (ret == D3DERR_NOTAVAILABLE) {
		Non_Fatal_Log_DX8_ErrorCode(ret, __FILE__, __LINE__);
		return nullptr;
	}

	if (ret == D3DERR_OUTOFVIDEOMEMORY) {
		TextureClass::Invalidate_Old_Unused_Textures(5000);
		WW3D::_Invalidate_Mesh_Cache();

		ret = D3DXCreateCubeTexture(
			_Get_D3D_Device8(), width, mip_level_count,
			usage, WW3DFormat_To_D3DFormat(format), pool, &texture);

		if (ret == D3DERR_OUTOFVIDEOMEMORY) {
			Non_Fatal_Log_DX8_ErrorCode(ret, __FILE__, __LINE__);
			return nullptr;
		}
	}
	DX8_ErrorCode(ret);
	return texture;
}

IDirect3DVolumeTexture8* DX8Wrapper::_Create_DX8_Volume_Texture(unsigned int width, unsigned int height, unsigned int depth, WW3DFormat format, MipCountType mip_level_count, _D3DPOOL pool)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();
	IDirect3DVolumeTexture8* texture = nullptr;

	unsigned ret = _Get_D3D_Device8()->CreateVolumeTexture(
		width, height, depth, mip_level_count, 0,
		WW3DFormat_To_D3DFormat(format), pool, &texture);

	DX8_ErrorCode(ret);
	return texture;
}

// ── Surface / Front-Back buffer (mirrors dx8wrapper.cpp lines 3011-3314) ──

IDirect3DSurface8* DX8Wrapper::_Create_DX8_Surface(unsigned int width, unsigned int height, WW3DFormat format)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();
	IDirect3DSurface8* surface = nullptr;

	WWASSERT(format != D3DFMT_P8);

	HRESULT hr = _Get_D3D_Device8()->CreateImageSurface(width, height, WW3DFormat_To_D3DFormat(format), &surface);
	number_of_DX8_calls++;

	if (FAILED(hr)) {
		Non_Fatal_Log_DX8_ErrorCode(hr, __FILE__, __LINE__);
		return nullptr;
	}
	return surface;
}

IDirect3DSurface8* DX8Wrapper::_Create_DX8_Surface(const char* filename)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();

	IDirect3DSurface8* surface = nullptr;

	{
		file_auto_ptr myfile(_TheFileFactory, filename);
		if (!myfile->Is_Available()) {
			char compressed_name[200];
			strlcpy(compressed_name, filename, sizeof(compressed_name));
			char* ext = strstr(compressed_name, ".");
			if (ext && (strlen(ext) == 4) &&
				((ext[1] == 't') || (ext[1] == 'T')) &&
				((ext[2] == 'g') || (ext[2] == 'G')) &&
				((ext[3] == 'a') || (ext[3] == 'A'))) {
				ext[1] = 'd'; ext[2] = 'd'; ext[3] = 's';
			}
			file_auto_ptr myfile2(_TheFileFactory, compressed_name);
			if (!myfile2->Is_Available()) {
				return MissingTexture::_Create_Missing_Surface();
			}
		}
	}

	StringClass filename_string(filename, true);
	surface = TextureLoader::Load_Surface_Immediate(filename_string, WW3D_FORMAT_UNKNOWN, true);
	return surface;
}

IDirect3DSurface8* DX8Wrapper::_Get_DX8_Front_Buffer()
{
	DX8_THREAD_ASSERT();
	D3DDISPLAYMODE mode;
	DX8CALL(GetDisplayMode(&mode));

	IDirect3DSurface8* fb = nullptr;
	DX8CALL(CreateImageSurface(mode.Width, mode.Height, D3DFMT_A8R8G8B8, &fb));
	DX8CALL(GetFrontBuffer(fb));
	return fb;
}

SurfaceClass* DX8Wrapper::_Get_DX8_Back_Buffer(unsigned int num)
{
	DX8_THREAD_ASSERT();

	IDirect3DSurface8* bb = nullptr;
	SurfaceClass* surf = nullptr;
	DX8CALL(GetBackBuffer(num, D3DBACKBUFFER_TYPE_MONO, &bb));
	if (bb) {
		surf = NEW_REF(SurfaceClass, (bb));
		bb->Release();
	}
	return surf;
}

void DX8Wrapper::Flush_DX8_Resource_Manager(unsigned int bytes)
{
	DX8_Assert();
	DX8CALL(ResourceManagerDiscardBytes(bytes));
}

unsigned int DX8Wrapper::Get_Free_Texture_RAM()
{
	DX8_Assert();
	return _Get_D3D_Device8()->GetAvailableTextureMem();
}

// ── Render target (mirrors dx8wrapper.cpp lines 3318-3790) ──

IDirect3DSwapChain8* DX8Wrapper::Create_Additional_Swap_Chain(HWND render_window)
{
	DX8_Assert();

	D3DPRESENT_PARAMETERS params = { 0 };
	params.BackBufferFormat = _PresentParameters.BackBufferFormat;
	params.BackBufferCount = 1;
	params.MultiSampleType = D3DMULTISAMPLE_NONE;
	params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	params.hDeviceWindow = render_window;
	params.Windowed = TRUE;
	params.EnableAutoDepthStencil = TRUE;
	params.AutoDepthStencilFormat = _PresentParameters.AutoDepthStencilFormat;
	params.Flags = 0;
	params.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	params.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

	IDirect3DSwapChain8* swap_chain = nullptr;
	DX8CALL(CreateAdditionalSwapChain(&params, &swap_chain));
	return swap_chain;
}

TextureClass* DX8Wrapper::Create_Render_Target(int width, int height, WW3DFormat format)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();
	number_of_DX8_calls++;

	if (format == WW3D_FORMAT_UNKNOWN) {
		D3DDISPLAYMODE mode;
		DX8CALL(GetDisplayMode(&mode));
		format = D3DFormat_To_WW3DFormat(mode.Format);
	}

	if (!Get_Current_Caps()->Support_Render_To_Texture_Format(format)) {
		return nullptr;
	}

	const D3DCAPS8& dx8caps = Get_Current_Caps()->Get_DX8_Caps();
	float poweroftwosize = width;
	if (height > 0 && height < width) {
		poweroftwosize = height;
	}
	poweroftwosize = ::Find_POT(poweroftwosize);

	if (poweroftwosize > dx8caps.MaxTextureWidth) {
		poweroftwosize = dx8caps.MaxTextureWidth;
	}
	if (poweroftwosize > dx8caps.MaxTextureHeight) {
		poweroftwosize = dx8caps.MaxTextureHeight;
	}

	width = height = poweroftwosize;

	TextureClass* tex = NEW_REF(TextureClass, (width, height, format, MIP_LEVELS_1, TextureClass::POOL_DEFAULT, true));

	if (tex->Peek_D3D_Base_Texture() == nullptr) {
		REF_PTR_RELEASE(tex);
	}

	return tex;
}

void DX8Wrapper::Create_Render_Target(int width, int height, WW3DFormat format, WW3DZFormat zformat, TextureClass** target, ZTextureClass** depth_buffer)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();
	number_of_DX8_calls++;

	if (format == WW3D_FORMAT_UNKNOWN) {
		*target = nullptr;
		*depth_buffer = nullptr;
		return;
	}

	if (!Get_Current_Caps()->Support_Render_To_Texture_Format(format) ||
		!Get_Current_Caps()->Support_Depth_Stencil_Format(zformat)) {
		return;
	}

	const D3DCAPS8& dx8caps = Get_Current_Caps()->Get_DX8_Caps();
	float poweroftwosize = width;
	if (height > 0 && height < width) {
		poweroftwosize = height;
	}
	poweroftwosize = ::Find_POT(poweroftwosize);

	if (poweroftwosize > dx8caps.MaxTextureWidth) {
		poweroftwosize = dx8caps.MaxTextureWidth;
	}
	if (poweroftwosize > dx8caps.MaxTextureHeight) {
		poweroftwosize = dx8caps.MaxTextureHeight;
	}

	width = height = poweroftwosize;

	TextureClass* tex = NEW_REF(TextureClass, (width, height, format, MIP_LEVELS_1, TextureClass::POOL_DEFAULT, true));

	if (tex->Peek_D3D_Base_Texture() == nullptr) {
		REF_PTR_RELEASE(tex);
	}

	*target = tex;

	*depth_buffer = NEW_REF(ZTextureClass, (width, height, zformat, MIP_LEVELS_1, TextureClass::POOL_DEFAULT));
}

void DX8Wrapper::Set_Render_Target(IDirect3DSurface8* render_target, bool use_default_depth_buffer)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();

	if (render_target == nullptr || render_target == DefaultRenderTarget) {
		if (DefaultRenderTarget != nullptr) {
			DX8CALL(SetRenderTarget(DefaultRenderTarget, DefaultDepthBuffer));
			DefaultRenderTarget->Release();
			DefaultRenderTarget = nullptr;
			if (DefaultDepthBuffer) {
				DefaultDepthBuffer->Release();
				DefaultDepthBuffer = nullptr;
			}
		}

		if (CurrentRenderTarget != nullptr) {
			CurrentRenderTarget->Release();
			CurrentRenderTarget = nullptr;
		}
		if (CurrentDepthBuffer != nullptr) {
			CurrentDepthBuffer->Release();
			CurrentDepthBuffer = nullptr;
		}
	} else if (render_target != CurrentRenderTarget) {
		if (DefaultDepthBuffer == nullptr) {
			DX8CALL(GetDepthStencilSurface(&DefaultDepthBuffer));
		}
		if (DefaultRenderTarget == nullptr) {
			DX8CALL(GetRenderTarget(&DefaultRenderTarget));
		}

		if (CurrentRenderTarget != nullptr) {
			CurrentRenderTarget->Release();
			CurrentRenderTarget = nullptr;
		}
		if (CurrentDepthBuffer != nullptr) {
			CurrentDepthBuffer->Release();
			CurrentDepthBuffer = nullptr;
		}

		CurrentRenderTarget = render_target;
		if (CurrentRenderTarget != nullptr) {
			CurrentRenderTarget->AddRef();
			if (use_default_depth_buffer) {
				DX8CALL(SetRenderTarget(CurrentRenderTarget, DefaultDepthBuffer));
			} else {
				DX8CALL(SetRenderTarget(CurrentRenderTarget, nullptr));
			}
		}
	}

	IsRenderToTexture = false;
}

void DX8Wrapper::Set_Render_Target(IDirect3DSurface8* render_target, IDirect3DSurface8* depth_buffer)
{
	DX8_THREAD_ASSERT();
	DX8_Assert();

	if (render_target == nullptr || render_target == DefaultRenderTarget) {
		if (DefaultRenderTarget != nullptr) {
			DX8CALL(SetRenderTarget(DefaultRenderTarget, DefaultDepthBuffer));
			DefaultRenderTarget->Release();
			DefaultRenderTarget = nullptr;
			if (DefaultDepthBuffer) {
				DefaultDepthBuffer->Release();
				DefaultDepthBuffer = nullptr;
			}
		}

		if (CurrentRenderTarget != nullptr) {
			CurrentRenderTarget->Release();
			CurrentRenderTarget = nullptr;
		}
		if (CurrentDepthBuffer != nullptr) {
			CurrentDepthBuffer->Release();
			CurrentDepthBuffer = nullptr;
		}
	} else if (render_target != CurrentRenderTarget) {
		if (DefaultDepthBuffer == nullptr) {
			DX8CALL(GetDepthStencilSurface(&DefaultDepthBuffer));
		}
		if (DefaultRenderTarget == nullptr) {
			DX8CALL(GetRenderTarget(&DefaultRenderTarget));
		}

		if (CurrentRenderTarget != nullptr) {
			CurrentRenderTarget->Release();
			CurrentRenderTarget = nullptr;
		}
		if (CurrentDepthBuffer != nullptr) {
			CurrentDepthBuffer->Release();
			CurrentDepthBuffer = nullptr;
		}

		CurrentRenderTarget = render_target;
		CurrentDepthBuffer = depth_buffer;
		if (CurrentRenderTarget != nullptr) {
			CurrentRenderTarget->AddRef();
			CurrentDepthBuffer->AddRef();
			DX8CALL(SetRenderTarget(CurrentRenderTarget, CurrentDepthBuffer));
		}
	}

	IsRenderToTexture = true;
}

void DX8Wrapper::Set_Render_Target(IDirect3DSwapChain8* swap_chain)
{
	DX8_THREAD_ASSERT();
	WWASSERT(swap_chain != nullptr);

	LPDIRECT3DSURFACE8 render_target = nullptr;
	swap_chain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &render_target);
	Set_Render_Target(render_target, true);

	if (render_target != nullptr) {
		render_target->Release();
		render_target = nullptr;
	}

	IsRenderToTexture = false;
}

void DX8Wrapper::Set_Render_Target_With_Z(TextureClass* texture, ZTextureClass* ztexture)
{
	WWASSERT(texture != nullptr);
	IDirect3DSurface8* d3d_surf = texture->Get_D3D_Surface_Level();
	WWASSERT(d3d_surf != nullptr);

	IDirect3DSurface8* d3d_zbuf = nullptr;
	if (ztexture != nullptr) {
		d3d_zbuf = ztexture->Get_D3D_Surface_Level();
		WWASSERT(d3d_zbuf != nullptr);
		Set_Render_Target(d3d_surf, d3d_zbuf);
		d3d_zbuf->Release();
	} else {
		Set_Render_Target(d3d_surf, true);
	}
	d3d_surf->Release();

	IsRenderToTexture = true;
}

// ── Statistics ──

// ── Statistics (mirrors dx8wrapper.cpp lines 1745-1796) ──
void DX8Wrapper::Reset_Statistics()
{
	matrix_changes = 0;
	material_changes = 0;
	vertex_buffer_changes = 0;
	index_buffer_changes = 0;
	light_changes = 0;
	texture_changes = 0;
	render_state_changes = 0;
	texture_stage_state_changes = 0;
	draw_calls = 0;

	number_of_DX8_calls = 0;
	last_frame_matrix_changes = 0;
	last_frame_material_changes = 0;
	last_frame_vertex_buffer_changes = 0;
	last_frame_index_buffer_changes = 0;
	last_frame_light_changes = 0;
	last_frame_texture_changes = 0;
	last_frame_render_state_changes = 0;
	last_frame_texture_stage_state_changes = 0;
	last_frame_number_of_DX8_calls = 0;
	last_frame_draw_calls = 0;
}

void DX8Wrapper::Begin_Statistics()
{
	matrix_changes = 0;
	material_changes = 0;
	vertex_buffer_changes = 0;
	index_buffer_changes = 0;
	light_changes = 0;
	texture_changes = 0;
	render_state_changes = 0;
	texture_stage_state_changes = 0;
	number_of_DX8_calls = 0;
	draw_calls = 0;
}

void DX8Wrapper::End_Statistics()
{
	last_frame_matrix_changes = matrix_changes;
	last_frame_material_changes = material_changes;
	last_frame_vertex_buffer_changes = vertex_buffer_changes;
	last_frame_index_buffer_changes = index_buffer_changes;
	last_frame_light_changes = light_changes;
	last_frame_texture_changes = texture_changes;
	last_frame_render_state_changes = render_state_changes;
	last_frame_texture_stage_state_changes = texture_stage_state_changes;
	last_frame_number_of_DX8_calls = number_of_DX8_calls;
	last_frame_draw_calls = draw_calls;
}
unsigned DX8Wrapper::Get_Last_Frame_Matrix_Changes() { return last_frame_matrix_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Material_Changes() { return last_frame_material_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Vertex_Buffer_Changes() { return last_frame_vertex_buffer_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Index_Buffer_Changes() { return last_frame_index_buffer_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Light_Changes() { return last_frame_light_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Texture_Changes() { return last_frame_texture_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Render_State_Changes() { return last_frame_render_state_changes; }
unsigned DX8Wrapper::Get_Last_Frame_Texture_Stage_State_Changes() { return last_frame_texture_stage_state_changes; }
unsigned DX8Wrapper::Get_Last_Frame_DX8_Calls() { return last_frame_number_of_DX8_calls; }
unsigned DX8Wrapper::Get_Last_Frame_Draw_Calls() { return last_frame_draw_calls; }
unsigned long DX8Wrapper::Get_FrameCount() { return FrameCount; }

// ── Device queries ──

int DX8Wrapper::Get_Render_Device_Count() { return 1; }
int DX8Wrapper::Get_Render_Device() { return 0; }
const RenderDeviceDescClass& DX8Wrapper::Get_Render_Device_Desc(int deviceidx) {
	if ((deviceidx == -1) && (CurRenderDevice == -1)) {
		CurRenderDevice = 0;
	}
	if (deviceidx == -1) {
		deviceidx = CurRenderDevice;
	}
	if (deviceidx >= 0 && deviceidx < _RenderDeviceDescriptionTable.Count()) {
		return _RenderDeviceDescriptionTable[deviceidx];
	}
	static RenderDeviceDescClass emptyDesc;
	return emptyDesc;
}
const char* DX8Wrapper::Get_Render_Device_Name(int device_index) { return "Metal"; }
void DX8Wrapper::Get_Device_Resolution(int& set_w, int& set_h, int& set_bits, bool& set_windowed) {
	set_w = ResolutionWidth; set_h = ResolutionHeight; set_bits = BitDepth; set_windowed = IsWindowed;
}
void DX8Wrapper::Get_Render_Target_Resolution(int& set_w, int& set_h, int& set_bits, bool& set_windowed) {
	set_w = ResolutionWidth; set_h = ResolutionHeight; set_bits = BitDepth; set_windowed = IsWindowed;
}
WW3DFormat DX8Wrapper::getBackBufferFormat() { return WW3D_FORMAT_A8R8G8B8; }
bool DX8Wrapper::Has_Stencil() { return false; }
void DX8Wrapper::Set_Swap_Interval(int swap) {}
int DX8Wrapper::Get_Swap_Interval() { return 1; }

// ── Registry ──

bool DX8Wrapper::Registry_Save_Render_Device(const char* sub_key) { return false; }
bool DX8Wrapper::Registry_Save_Render_Device(const char* sub_key, int device, int width, int height, int depth, bool windowed, int texture_depth) { return false; }
bool DX8Wrapper::Registry_Load_Render_Device(const char* sub_key, bool resize_window) { return false; }
bool DX8Wrapper::Registry_Load_Render_Device(const char* sub_key, char* device, int device_len, int& width, int& height, int& depth, int& windowed, int& texture_depth) { return false; }

// ── Color mode helpers ──

bool DX8Wrapper::Find_Color_And_Z_Mode(int resx, int resy, int bitdepth, D3DFORMAT* set_colorbuffer, D3DFORMAT* set_backbuffer, D3DFORMAT* set_zmode) { return true; }
bool DX8Wrapper::Find_Color_Mode(D3DFORMAT colorbuffer, int resx, int resy, UINT* mode) { return true; }
bool DX8Wrapper::Find_Z_Mode(D3DFORMAT colorbuffer, D3DFORMAT backbuffer, D3DFORMAT* zmode) { return true; }
bool DX8Wrapper::Test_Z_Mode(D3DFORMAT colorbuffer, D3DFORMAT backbuffer, D3DFORMAT zmode) { return true; }

// ── Format name ──

void DX8Wrapper::Get_Format_Name(unsigned int format, StringClass* tex_format) {}

// ── Debug name getters ──

const char* DX8Wrapper::Get_DX8_Render_State_Name(D3DRENDERSTATETYPE state) { return ""; }
const char* DX8Wrapper::Get_DX8_Texture_Stage_State_Name(D3DTEXTURESTAGESTATETYPE state) { return ""; }
void DX8Wrapper::Get_DX8_Render_State_Value_Name(StringClass& name, D3DRENDERSTATETYPE state, unsigned value) {}
void DX8Wrapper::Get_DX8_Texture_Stage_State_Value_Name(StringClass& name, D3DTEXTURESTAGESTATETYPE state, unsigned value) {}
const char* DX8Wrapper::Get_DX8_Texture_Address_Name(unsigned value) { return ""; }
const char* DX8Wrapper::Get_DX8_Texture_Filter_Name(unsigned value) { return ""; }
const char* DX8Wrapper::Get_DX8_Texture_Arg_Name(unsigned value) { return ""; }
const char* DX8Wrapper::Get_DX8_Texture_Op_Name(unsigned value) { return ""; }
const char* DX8Wrapper::Get_DX8_Texture_Transform_Flag_Name(unsigned value) { return ""; }
const char* DX8Wrapper::Get_DX8_ZBuffer_Type_Name(unsigned value) { return ""; }
const char* DX8Wrapper::Get_DX8_Fill_Mode_Name(unsigned value) { return ""; }
const char* DX8Wrapper::Get_DX8_Shade_Mode_Name(unsigned value) { return ""; }
const char* DX8Wrapper::Get_DX8_Blend_Name(unsigned value) { return ""; }
const char* DX8Wrapper::Get_DX8_Cull_Mode_Name(unsigned value) { return ""; }
const char* DX8Wrapper::Get_DX8_Cmp_Func_Name(unsigned value) { return ""; }
const char* DX8Wrapper::Get_DX8_Fog_Mode_Name(unsigned value) { return ""; }
const char* DX8Wrapper::Get_DX8_Stencil_Op_Name(unsigned value) { return ""; }
const char* DX8Wrapper::Get_DX8_Material_Source_Name(unsigned value) { return ""; }
const char* DX8Wrapper::Get_DX8_Vertex_Blend_Flag_Name(unsigned value) { return ""; }
const char* DX8Wrapper::Get_DX8_Patch_Edge_Style_Name(unsigned value) { return ""; }
const char* DX8Wrapper::Get_DX8_Debug_Monitor_Token_Name(unsigned value) { return ""; }
const char* DX8Wrapper::Get_DX8_Blend_Op_Name(unsigned value) { return ""; }
