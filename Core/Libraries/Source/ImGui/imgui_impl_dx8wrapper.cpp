#include "imgui_impl_dx8wrapper.h"

#include "imgui.h"

#include <algorithm>

#include "WWMath/vector2i.h"
#include "WW3D2/dx8wrapper.h"
#include "WW3D2/dx8vertexbuffer.h"
#include "WW3D2/dx8indexbuffer.h"
#include "WW3D2/shader.h"
#include "WW3D2/surfaceclass.h"
#include "WW3D2/texture.h"
#include "WW3D2/vertmaterial.h"
#include "WW3D2/ww3d.h"

namespace
{

struct ClipRect
{
	float left;
	float top;
	float right;
	float bottom;
};

ShaderClass makeImGuiShader()
{
	ShaderClass shader = ShaderClass::_PresetAlpha2DShader;
	shader.Set_Primary_Gradient(ShaderClass::GRADIENT_MODULATE);
	shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);
	return shader;
}

TextureClass* textureFromId(ImTextureID textureId)
{
	return reinterpret_cast<TextureClass*>(static_cast<intptr_t>(textureId));
}

struct PixelBlock
{
	const unsigned char* pixels;
	int pitch;
	int x;
	int y;
	int width;
	int height;
	int bytesPerPixel;
};

void uploadBlock(TextureClass* texture, const PixelBlock& block)
{
	SurfaceClass* surface = texture->Get_Surface_Level(0);
	int pitch = 0;
	const Vector2i min(block.x, block.y);
	const Vector2i max(block.x + block.width, block.y + block.height);
	unsigned char* bits = static_cast<unsigned char*>(surface->Lock(&pitch, min, max));
	if (bits != nullptr)
	{
		for (int row = 0; row < block.height; ++row)
		{
			memcpy(bits + pitch * row, block.pixels + block.pitch * row, block.width * block.bytesPerPixel);
		}
		surface->Unlock();
	}
	REF_PTR_RELEASE(surface);
}

TextureClass* newTexture(int width, int height)
{
	return NEW_REF(TextureClass, (width, height, WW3D_FORMAT_A8R8G8B8, MIP_LEVELS_1, TextureClass::POOL_MANAGED, false, false));
}

void uploadRect(ImTextureData* tex, const ImTextureRect& rect)
{
	const PixelBlock block = {
		static_cast<const unsigned char*>(tex->GetPixelsAt(rect.x, rect.y)),
		tex->GetPitch(),
		rect.x,
		rect.y,
		rect.w,
		rect.h,
		tex->BytesPerPixel,
	};
	uploadBlock(textureFromId(tex->TexID), block);
}

void createTexture(ImTextureData* tex)
{
	TextureClass* texture = newTexture(tex->Width, tex->Height);
	tex->SetTexID(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(texture)));

	ImTextureRect wholeTexture = { 0, 0, static_cast<unsigned short>(tex->Width), static_cast<unsigned short>(tex->Height) };
	uploadRect(tex, wholeTexture);
	tex->SetStatus(ImTextureStatus_OK);
}

void updateTexture(ImTextureData* tex)
{
	for (const ImTextureRect& rect : tex->Updates)
	{
		uploadRect(tex, rect);
	}
	tex->SetStatus(ImTextureStatus_OK);
}

void destroyTexture(ImTextureData* tex)
{
	TextureClass* texture = textureFromId(tex->TexID);
	REF_PTR_RELEASE(texture);
	tex->SetTexID(ImTextureID_Invalid);
	tex->SetStatus(ImTextureStatus_Destroyed);
}

void processTexture(ImTextureData* tex)
{
	switch (tex->Status)
	{
	case ImTextureStatus_WantCreate:
		createTexture(tex);
		return;
	case ImTextureStatus_WantUpdates:
		updateTexture(tex);
		return;
	case ImTextureStatus_WantDestroy:
		destroyTexture(tex);
		return;
	default:
		return;
	}
}

void processTextures(ImDrawData* drawData)
{
	if (drawData->Textures == nullptr)
	{
		return;
	}

	for (ImTextureData* tex : *drawData->Textures)
	{
		processTexture(tex);
	}
}

void setupRenderState()
{
	VertexMaterialClass* material = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_DIFFUSE);
	DX8Wrapper::Set_Material(material);
	REF_PTR_RELEASE(material);

	ShaderClass::Invalidate();
	DX8Wrapper::Set_Shader(makeImGuiShader());
	DX8Wrapper::Set_World_Identity();
	DX8Wrapper::Set_View_Identity();
}

void writeVertices(const ImDrawList* drawList, DynamicVBAccessClass& vertexBuffer)
{
	DynamicVBAccessClass::WriteLockClass lock(&vertexBuffer);
	const FVFInfoClass& fvf = vertexBuffer.FVF_Info();
	unsigned char* destination = reinterpret_cast<unsigned char*>(lock.Get_Formatted_Vertex_Array());

	for (const ImDrawVert& source : drawList->VtxBuffer)
	{
		*reinterpret_cast<Vector3*>(destination + fvf.Get_Location_Offset()) = Vector3(source.pos.x, source.pos.y, 0.0f);
		*reinterpret_cast<unsigned int*>(destination + fvf.Get_Diffuse_Offset()) = source.col;
		*reinterpret_cast<Vector2*>(destination + fvf.Get_Tex_Offset(0)) = Vector2(source.uv.x, source.uv.y);
		destination += fvf.Get_FVF_Size();
	}
}

void writeIndices(const ImDrawList* drawList, DynamicIBAccessClass& indexBuffer)
{
	DynamicIBAccessClass::WriteLockClass lock(&indexBuffer);
	memcpy(lock.Get_Index_Array(), drawList->IdxBuffer.Data, drawList->IdxBuffer.Size * sizeof(ImDrawIdx));
}

bool clipToDisplay(const ImDrawCmd& command, const ImDrawData* drawData, ClipRect& clip)
{
	const ImVec2 offset = drawData->DisplayPos;
	clip.left = std::max(command.ClipRect.x - offset.x, 0.0f);
	clip.top = std::max(command.ClipRect.y - offset.y, 0.0f);
	clip.right = std::min(command.ClipRect.z - offset.x, drawData->DisplaySize.x);
	clip.bottom = std::min(command.ClipRect.w - offset.y, drawData->DisplaySize.y);
	return clip.right > clip.left && clip.bottom > clip.top;
}

void applyClip(const ClipRect& clip, const ImVec2& displayPos, const ImVec2& framebufferScale)
{
	D3DVIEWPORT8 viewport;
	viewport.X = static_cast<DWORD>(clip.left * framebufferScale.x);
	viewport.Y = static_cast<DWORD>(clip.top * framebufferScale.y);
	viewport.Width = static_cast<DWORD>((clip.right - clip.left) * framebufferScale.x);
	viewport.Height = static_cast<DWORD>((clip.bottom - clip.top) * framebufferScale.y);
	viewport.MinZ = 0.0f;
	viewport.MaxZ = 1.0f;
	DX8Wrapper::Set_Viewport(&viewport);

	const float left = clip.left + displayPos.x;
	const float right = clip.right + displayPos.x;
	const float top = clip.top + displayPos.y;
	const float bottom = clip.bottom + displayPos.y;
	const Matrix4x4 projection(
		Vector4(2.0f / (right - left), 0.0f, 0.0f, (left + right) / (left - right)),
		Vector4(0.0f, 2.0f / (top - bottom), 0.0f, (top + bottom) / (bottom - top)),
		Vector4(0.0f, 0.0f, 1.0f, 0.0f),
		Vector4(0.0f, 0.0f, 0.0f, 1.0f));
	DX8Wrapper::Set_Transform(D3DTS_PROJECTION, projection);
}

ImVec2 renderTargetScale(const ImDrawData* drawData)
{
	int width = 0;
	int height = 0;
	int bits = 0;
	bool windowed = false;
	WW3D::Get_Render_Target_Resolution(width, height, bits, windowed);
	return ImVec2(width / drawData->DisplaySize.x, height / drawData->DisplaySize.y);
}

void renderCommand(const ImDrawCmd& command, const ImDrawList* drawList, const ImDrawData* drawData, const ImVec2& targetScale)
{
	if (command.UserCallback != nullptr)
	{
		command.UserCallback(drawList, &command);
		setupRenderState();
		return;
	}

	ClipRect clip;
	if (!clipToDisplay(command, drawData, clip))
	{
		return;
	}

	applyClip(clip, drawData->DisplayPos, targetScale);
	DX8Wrapper::Set_Texture(0, textureFromId(command.GetTexID()));
	DX8Wrapper::Draw_Triangles(
		static_cast<unsigned short>(command.IdxOffset),
		static_cast<unsigned short>(command.ElemCount / 3),
		0,
		static_cast<unsigned short>(drawList->VtxBuffer.Size));
}

void renderDrawList(const ImDrawList* drawList, const ImDrawData* drawData, const ImVec2& targetScale)
{
	if (drawList->VtxBuffer.Size == 0 || drawList->IdxBuffer.Size == 0)
	{
		return;
	}

	DynamicVBAccessClass vertexBuffer(BUFFER_TYPE_DYNAMIC_DX8, dynamic_fvf_type, static_cast<unsigned short>(drawList->VtxBuffer.Size));
	writeVertices(drawList, vertexBuffer);
	DynamicIBAccessClass indexBuffer(BUFFER_TYPE_DYNAMIC_DX8, static_cast<unsigned short>(drawList->IdxBuffer.Size));
	writeIndices(drawList, indexBuffer);

	DX8Wrapper::Set_Vertex_Buffer(vertexBuffer);
	DX8Wrapper::Set_Index_Buffer(indexBuffer, 0);

	for (const ImDrawCmd& command : drawList->CmdBuffer)
	{
		renderCommand(command, drawList, drawData, targetScale);
	}
}

}

bool ImGui_ImplDX8Wrapper_Init()
{
	ImGuiIO& io = ImGui::GetIO();
	io.BackendRendererName = "imgui_impl_dx8wrapper";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

	ImGuiPlatformIO& platformIo = ImGui::GetPlatformIO();
	platformIo.Renderer_TextureMaxWidth = 4096;
	platformIo.Renderer_TextureMaxHeight = 4096;
	return true;
}

void ImGui_ImplDX8Wrapper_Shutdown()
{
	for (ImTextureData* tex : ImGui::GetPlatformIO().Textures)
	{
		if (tex->TexID != ImTextureID_Invalid)
		{
			destroyTexture(tex);
		}
	}

	ImGuiIO& io = ImGui::GetIO();
	io.BackendRendererName = nullptr;
	io.BackendFlags &= ~ImGuiBackendFlags_RendererHasTextures;
}

void ImGui_ImplDX8Wrapper_NewFrame()
{
}

ImTextureID ImGui_ImplDX8Wrapper_CreateTexture(int width, int height, const void* bgraPixels, int pitch)
{
	TextureClass* texture = newTexture(width, height);
	const PixelBlock block = { static_cast<const unsigned char*>(bgraPixels), pitch, 0, 0, width, height, 4 };
	uploadBlock(texture, block);
	return static_cast<ImTextureID>(reinterpret_cast<intptr_t>(texture));
}

void ImGui_ImplDX8Wrapper_DestroyTexture(ImTextureID textureId)
{
	TextureClass* texture = textureFromId(textureId);
	REF_PTR_RELEASE(texture);
}

void ImGui_ImplDX8Wrapper_RenderDrawData(ImDrawData* drawData)
{
	if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
	{
		return;
	}

	processTextures(drawData);

	Matrix4x4 savedView;
	Matrix4x4 savedProjection;
	D3DVIEWPORT8 savedViewport;
	DX8Wrapper::Get_Transform(D3DTS_VIEW, savedView);
	DX8Wrapper::Get_Transform(D3DTS_PROJECTION, savedProjection);
	DX8Wrapper::_Get_D3D_Device8()->GetViewport(&savedViewport);

	setupRenderState();
	const ImVec2 targetScale = renderTargetScale(drawData);
	for (const ImDrawList* drawList : drawData->CmdLists)
	{
		renderDrawList(drawList, drawData, targetScale);
	}

	DX8Wrapper::Set_Viewport(&savedViewport);
	DX8Wrapper::Set_Transform(D3DTS_VIEW, savedView);
	DX8Wrapper::Set_Transform(D3DTS_PROJECTION, savedProjection);
}
