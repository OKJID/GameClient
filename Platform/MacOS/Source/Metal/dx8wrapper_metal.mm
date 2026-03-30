// dx8wrapper_metal.mm — Metal implementation of DX8Wrapper static methods
//
// This file replaces dx8wrapper.cpp on macOS (which is guarded by #ifndef __APPLE__).
// All static fields and methods of DX8Wrapper class are defined here.

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <AppKit/AppKit.h>

#include "dx8wrapper.h"
#include "dx8caps.h"
#include "dx8texman.h"
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

#include "MetalDevice8.h"
#include "MetalInterface8.h"
#include "MetalSurface8.h"

// ── Constants (mirrors dx8wrapper.cpp lines 91-95) ──

const int DEFAULT_RESOLUTION_WIDTH = 640;
const int DEFAULT_RESOLUTION_HEIGHT = 480;
const int DEFAULT_BIT_DEPTH = 32;
const int DEFAULT_TEXTURE_BIT_DEPTH = 16;
const D3DMULTISAMPLE_TYPE DEFAULT_MSAA = D3DMULTISAMPLE_NONE;

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

// ── Init / Shutdown / Device ──

bool DX8Wrapper::Init(void* hwnd, bool lite) { return true; }
void DX8Wrapper::Shutdown() {}
void DX8Wrapper::Do_Onetime_Device_Dependent_Inits() {}
void DX8Wrapper::Do_Onetime_Device_Dependent_Shutdowns() {}
bool DX8Wrapper::Create_Device() { return true; }
void DX8Wrapper::Release_Device() {}
bool DX8Wrapper::Reset_Device(bool reload_assets) { return true; }
void DX8Wrapper::Enumerate_Devices() {}
void DX8Wrapper::Compute_Caps(WW3DFormat display_format) {}
void DX8Wrapper::Set_Default_Global_Render_States() {}
bool DX8Wrapper::Validate_Device() { return true; }
void DX8Wrapper::Invalidate_Cached_Render_States() {}

// ── Render device selection ──

bool DX8Wrapper::Set_Any_Render_Device() { return true; }
bool DX8Wrapper::Set_Render_Device(const char* dev_name, int width, int height, int bits, int windowed, bool resize_window) { return true; }
bool DX8Wrapper::Set_Render_Device(int dev, int resx, int resy, int bits, int windowed, bool resize_window, bool reset_device, bool restore_assets) { return true; }
bool DX8Wrapper::Set_Next_Render_Device() { return true; }
bool DX8Wrapper::Toggle_Windowed() { return true; }
bool DX8Wrapper::Set_Device_Resolution(int width, int height, int bits, int windowed, bool resize_window) { return true; }
void DX8Wrapper::Resize_And_Position_Window() {}

// ── Scene / Frame ──

void DX8Wrapper::Begin_Scene() {}
void DX8Wrapper::End_Scene(bool flip_frames) {}
void DX8Wrapper::Flip_To_Primary() {}
void DX8Wrapper::Clear(bool clear_color, bool clear_z_stencil, const Vector3& color, float dest_alpha, float z, unsigned int stencil) {}
void DX8Wrapper::Set_Viewport(CONST D3DVIEWPORT8* pViewport) {}

// ── Vertex / Index Buffers ──

void DX8Wrapper::Set_Vertex_Buffer(const VertexBufferClass* vb, unsigned stream) {}
void DX8Wrapper::Set_Index_Buffer(const IndexBufferClass* ib, unsigned short index_base_offset) {}
void DX8Wrapper::Set_Vertex_Buffer(const DynamicVBAccessClass& vba) {}
void DX8Wrapper::Set_Index_Buffer(const DynamicIBAccessClass& iba, unsigned short index_base_offset) {}

// ── Draw calls ──

void DX8Wrapper::Draw_Sorting_IB_VB(
	VertexBufferClass* vb, IndexBufferClass* ib,
	unsigned short num_verts, unsigned short num_indices,
	unsigned short min_vertex_index,
	unsigned short start_index) {}

void DX8Wrapper::Draw(
	unsigned short start_index, unsigned short polygon_count,
	unsigned short min_vertex_index, unsigned short num_vertices) {}

void DX8Wrapper::Draw_Triangles(
	unsigned short start_index, unsigned short polygon_count,
	unsigned short min_vertex_index, unsigned short num_vertices) {}

void DX8Wrapper::Draw_Triangles(
	unsigned short startVertex, unsigned short numVertices) {}

void DX8Wrapper::Draw_Strip(
	unsigned short start_index, unsigned short polygon_count,
	unsigned short min_vertex_index, unsigned short num_vertices) {}

void DX8Wrapper::Apply_Render_State_Changes() {}
void DX8Wrapper::Apply_Default_State() {}

// ── Render state / Material ──

void DX8Wrapper::Get_Render_State(RenderStateStruct& state) { state = render_state; }
void DX8Wrapper::Set_Render_State(const RenderStateStruct& state) { render_state = state; }
void DX8Wrapper::Release_Render_State() {}
void DX8Wrapper::Set_DX8_Material(const D3DMATERIAL8* mat) {}
void DX8Wrapper::Set_DX8_Render_State(D3DRENDERSTATETYPE state, unsigned value) { RenderStates[state] = value; }
void DX8Wrapper::Set_DX8_Texture_Stage_State(unsigned stage, D3DTEXTURESTAGESTATETYPE state, unsigned value) { TextureStageStates[stage][state] = value; }
void DX8Wrapper::Set_DX8_Texture(unsigned int stage, IDirect3DBaseTexture8* texture) { Textures[stage] = texture; }
void DX8Wrapper::Set_DX8_Clip_Plane(DWORD Index, CONST float* pPlane) {}
void DX8Wrapper::Set_Shader(const ShaderClass& shader) { render_state.Shaders[0] = shader; }
void DX8Wrapper::Set_Polygon_Mode(int mode) {}

// ── Transforms ──

void DX8Wrapper::Set_Transform(D3DTRANSFORMSTATETYPE transform, const Matrix4x4& m) {}
void DX8Wrapper::Set_Transform(D3DTRANSFORMSTATETYPE transform, const Matrix3D& m) {}
void DX8Wrapper::Get_Transform(D3DTRANSFORMSTATETYPE transform, Matrix4x4& m) {}
void DX8Wrapper::Set_World_Identity() { world_identity = true; }
void DX8Wrapper::Set_View_Identity() {}
bool DX8Wrapper::Is_World_Identity() { return world_identity; }
bool DX8Wrapper::Is_View_Identity() { return false; }
void DX8Wrapper::Set_DX8_ZBias(int zbias) { ZBias = zbias; }
void DX8Wrapper::Set_Projection_Transform_With_Z_Bias(const Matrix4x4& matrix, float znear, float zfar) {}

// ── Lights / Fog ──

void DX8Wrapper::Set_DX8_Light(int index, D3DLIGHT8* light) {}
void DX8Wrapper::Set_Light_Environment(LightEnvironmentClass* light_env) { Light_Environment = light_env; }
void DX8Wrapper::Set_Fog(bool enable, const Vector3& color, float start, float end) { FogEnable = enable; }
void DX8Wrapper::Set_Ambient(const Vector3& color) { Ambient_Color = color; }
void DX8Wrapper::Set_Gamma(float gamma, float bright, float contrast, bool calibrate, bool uselimit) {}

// ── Texture creation ──

IDirect3DTexture8* DX8Wrapper::_Create_DX8_Texture(unsigned int width, unsigned int height, WW3DFormat format, MipCountType mip_level_count, D3DPOOL pool, bool rendertarget) { return nullptr; }
IDirect3DTexture8* DX8Wrapper::_Create_DX8_Texture(const char* filename, MipCountType mip_level_count) { return nullptr; }
IDirect3DTexture8* DX8Wrapper::_Create_DX8_Texture(IDirect3DSurface8* surface, MipCountType mip_level_count) { return nullptr; }
void DX8Wrapper::_Update_Texture(TextureClass* system, TextureClass* video) {}

// ── Surface / Front-Back buffer ──

IDirect3DSurface8* DX8Wrapper::_Create_DX8_Surface(unsigned int width, unsigned int height, WW3DFormat format) { return nullptr; }
IDirect3DSurface8* DX8Wrapper::_Create_DX8_Surface(const char* filename) { return nullptr; }
IDirect3DSurface8* DX8Wrapper::_Get_DX8_Front_Buffer() { return nullptr; }
SurfaceClass* DX8Wrapper::_Get_DX8_Back_Buffer(unsigned int num) { return nullptr; }

void DX8Wrapper::_Copy_DX8_Rects(IDirect3DSurface8* pSourceSurface, CONST RECT* pSourceRectsArray, UINT cRects, IDirect3DSurface8* pDestinationSurface, CONST POINT* pDestPointsArray) {}
void DX8Wrapper::Flush_DX8_Resource_Manager(unsigned int bytes) {}
unsigned int DX8Wrapper::Get_Free_Texture_RAM() { return 256 * 1024 * 1024; }

// ── Render target ──

IDirect3DSwapChain8* DX8Wrapper::Create_Additional_Swap_Chain(HWND render_window) { return nullptr; }
TextureClass* DX8Wrapper::Create_Render_Target(int width, int height, WW3DFormat format) { return nullptr; }
void DX8Wrapper::Create_Render_Target(int width, int height, WW3DFormat format, WW3DZFormat zformat, TextureClass** target, ZTextureClass** depth_buffer) {}
void DX8Wrapper::Set_Render_Target(IDirect3DSurface8* render_target, bool use_default_depth_buffer) {}
void DX8Wrapper::Set_Render_Target(IDirect3DSurface8* render_target, IDirect3DSurface8* depth_buffer) {}
void DX8Wrapper::Set_Render_Target(IDirect3DSwapChain8* swap_chain) {}
void DX8Wrapper::Set_Render_Target_With_Z(TextureClass* texture, ZTextureClass* ztexture) {}

// ── Statistics ──

void DX8Wrapper::Reset_Statistics() {}
void DX8Wrapper::Begin_Statistics() {}
void DX8Wrapper::End_Statistics() {}
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
	static RenderDeviceDescClass desc;
	return desc;
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

// ── Format name / Get_Format_Name ──

void DX8Wrapper::Get_Format_Name(unsigned int format, StringClass* tex_format) {}

// ── Utilities ──

Vector4 DX8Wrapper::Convert_Color(unsigned color) {
	return Vector4(
		((color >> 16) & 0xFF) / 255.0f,
		((color >> 8) & 0xFF) / 255.0f,
		(color & 0xFF) / 255.0f,
		((color >> 24) & 0xFF) / 255.0f);
}
unsigned int DX8Wrapper::Convert_Color(const Vector4& color) {
	return D3DCOLOR_ARGB(
		(int)(color.W * 255), (int)(color.X * 255),
		(int)(color.Y * 255), (int)(color.Z * 255));
}
unsigned int DX8Wrapper::Convert_Color(const Vector3& color, const float alpha) {
	return D3DCOLOR_ARGB(
		(int)(alpha * 255), (int)(color.X * 255),
		(int)(color.Y * 255), (int)(color.Z * 255));
}
void DX8Wrapper::Clamp_Color(Vector4& color) {
	if (color.X < 0) color.X = 0; if (color.X > 1) color.X = 1;
	if (color.Y < 0) color.Y = 0; if (color.Y > 1) color.Y = 1;
	if (color.Z < 0) color.Z = 0; if (color.Z > 1) color.Z = 1;
	if (color.W < 0) color.W = 0; if (color.W > 1) color.W = 1;
}
unsigned int DX8Wrapper::Convert_Color_Clamp(const Vector4& color) {
	Vector4 c = color; Clamp_Color(c); return Convert_Color(c);
}
void DX8Wrapper::Set_Alpha(const float alpha, unsigned int& color) {
	color = (color & 0x00FFFFFF) | ((unsigned int)(alpha * 255.0f) << 24);
}

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
