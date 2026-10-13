#include "d3dx8.h"

#include "Win32Metrics.h"

#include "imgui.h"
#include "imgui_impl_dx8wrapper.h"

#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace
{

class D3DXImGuiFont : public ID3DXFont
{
public:
	explicit D3DXImGuiFont(const LOGFONTA& logFont) :
		m_logFont(logFont),
		m_drawList(ImGui::GetDrawListSharedData())
	{
	}

	ULONG AddRef() override
	{
		return ++m_references;
	}

	ULONG Release() override
	{
		const ULONG references = --m_references;
		if (references == 0)
		{
			delete this;
		}
		return references;
	}

	HRESULT Begin() override
	{
		m_isBatching = true;
		startDrawList();
		return D3D_OK;
	}

	INT DrawTextA(LPCSTR pString, INT Count, LPRECT pRect, DWORD Format, D3DCOLOR Color) override;

	HRESULT End() override
	{
		if (m_isBatching)
		{
			renderDrawList();
		}
		m_isBatching = false;
		return D3D_OK;
	}

	HRESULT OnLostDevice() override
	{
		return D3D_OK;
	}

	HRESULT OnResetDevice() override
	{
		return D3D_OK;
	}

private:
	static ImFont* font();
	static ImU32 colorOf(D3DCOLOR color);

	float pixelSize() const;
	void startDrawList();
	void renderDrawList();

	LOGFONTA m_logFont;
	ImDrawList m_drawList;
	ULONG m_references = 1;
	bool m_isBatching = false;
};

ImVec2 textPosition(const RECT& rect, ImVec2 textSize, DWORD format)
{
	ImVec2 position(static_cast<float>(rect.left), static_cast<float>(rect.top));
	if ((format & DT_CENTER) != 0)
	{
		position.x = (rect.left + rect.right - textSize.x) * 0.5f;
	}
	else if ((format & DT_RIGHT) != 0)
	{
		position.x = rect.right - textSize.x;
	}
	if ((format & DT_SINGLELINE) == 0)
	{
		return position;
	}
	if ((format & DT_VCENTER) != 0)
	{
		position.y = (rect.top + rect.bottom - textSize.y) * 0.5f;
	}
	else if ((format & DT_BOTTOM) != 0)
	{
		position.y = rect.bottom - textSize.y;
	}
	return position;
}

ImFont* D3DXImGuiFont::font()
{
	return Win32Metrics::systemFont();
}

ImU32 D3DXImGuiFont::colorOf(D3DCOLOR color)
{
	return IM_COL32((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (color >> 24) & 0xFF);
}

float D3DXImGuiFont::pixelSize() const
{
	if (m_logFont.lfHeight == 0)
	{
		return Win32Metrics::systemFontSize();
	}
	return static_cast<float>(std::abs(m_logFont.lfHeight));
}

void D3DXImGuiFont::startDrawList()
{
	m_drawList._ResetForNewFrame();
	m_drawList.PushClipRectFullScreen();
	m_drawList.PushTexture(font()->OwnerAtlas->TexRef);
}

void D3DXImGuiFont::renderDrawList()
{
	ImDrawData drawData;
	drawData.Valid = true;
	drawData.DisplayPos = ImVec2(0.0f, 0.0f);
	drawData.DisplaySize = ImGui::GetIO().DisplaySize;
	drawData.FramebufferScale = ImVec2(1.0f, 1.0f);
	drawData.Textures = &ImGui::GetPlatformIO().Textures;
	drawData.AddDrawList(&m_drawList);
	ImGui_ImplDX8Wrapper_RenderDrawData(&drawData);
}

INT D3DXImGuiFont::DrawTextA(LPCSTR pString, INT Count, LPRECT pRect, DWORD Format, D3DCOLOR Color)
{
	if (pString == nullptr || pRect == nullptr)
	{
		return 0;
	}
	const char* end = pString + (Count < 0 ? strlen(pString) : static_cast<size_t>(Count));
	const float size = pixelSize();
	const float wrapWidth = (Format & DT_WORDBREAK) != 0 ? static_cast<float>(pRect->right - pRect->left) : 0.0f;
	const ImVec2 textSize = font()->CalcTextSizeA(size, FLT_MAX, wrapWidth, pString, end);
	if ((Format & DT_CALCRECT) != 0)
	{
		pRect->right = pRect->left + static_cast<LONG>(std::ceil(textSize.x));
		pRect->bottom = pRect->top + static_cast<LONG>(std::ceil(textSize.y));
		return static_cast<INT>(std::ceil(textSize.y));
	}

	if (!m_isBatching)
	{
		startDrawList();
	}
	const ImVec4 clip(static_cast<float>(pRect->left), static_cast<float>(pRect->top), static_cast<float>(pRect->right), static_cast<float>(pRect->bottom));
	const ImVec4* clipRect = (Format & DT_NOCLIP) != 0 ? nullptr : &clip;
	m_drawList.AddText(font(), size, textPosition(*pRect, textSize, Format), colorOf(Color), pString, end, wrapWidth, clipRect);
	if (!m_isBatching)
	{
		renderDrawList();
	}
	return static_cast<INT>(std::ceil(textSize.y));
}

}

HRESULT WINAPI D3DXCreateFont(IDirect3DDevice8* pDevice, HFONT hFont, LPD3DXFONT* ppFont)
{
	if (pDevice == nullptr || ppFont == nullptr)
	{
		return D3DERR_INVALIDCALL;
	}
	LOGFONTA logFont = {};
	if (GetObjectA(hFont, sizeof(logFont), &logFont) == 0)
	{
		return D3DERR_INVALIDCALL;
	}
	*ppFont = new D3DXImGuiFont(logFont);
	return D3D_OK;
}
