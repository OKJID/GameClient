#include "Win32Static.h"

#include "Win32ControlDrawing.h"
#include "Win32Gdi.h"

namespace
{

Win32ImagePtr imageOf(HANDLE handle, UINT type)
{
	if (type == IMAGE_BITMAP)
	{
		Win32GdiObject* object = Win32Gdi::object(handle);
		return object != nullptr && object->type == Win32GdiObjectType::Bitmap ? static_cast<Win32Bitmap*>(object)->image : nullptr;
	}
	Win32Icon* icon = Win32Gdi::icon(static_cast<HICON>(handle));
	return icon != nullptr ? icon->image : nullptr;
}

ImU32 frameColor(UINT staticType)
{
	switch (staticType)
	{
	case SS_BLACKFRAME:
	case SS_BLACKRECT:
		return Win32Gdi::toImColor(GetSysColor(COLOR_WINDOWFRAME));
	case SS_WHITEFRAME:
	case SS_WHITERECT:
		return Win32Gdi::toImColor(GetSysColor(COLOR_WINDOW));
	default:
		return Win32Gdi::toImColor(GetSysColor(COLOR_BTNSHADOW));
	}
}

}

void Win32Static::loadImageFromName(const CREATESTRUCTA* create)
{
	if (!isImage() || create == nullptr || create->lpszName == nullptr || !IS_INTRESOURCE(create->lpszName))
	{
		return;
	}
	const UINT type = staticType() == SS_BITMAP ? IMAGE_BITMAP : IMAGE_ICON;
	setImage(type, LoadImageA(create->hInstance, create->lpszName, type, 0, 0, LR_DEFAULTCOLOR));
}

HANDLE Win32Static::setImage(UINT type, HANDLE image)
{
	const HANDLE previous = m_image;
	m_image = image;
	m_imageType = type;
	InvalidateRect(hwnd(), nullptr, TRUE);
	return previous;
}

LRESULT Win32Static::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		loadImageFromName(reinterpret_cast<const CREATESTRUCTA*>(lParam));
		return 0;
	case STM_SETIMAGE:
		return reinterpret_cast<LRESULT>(setImage(static_cast<UINT>(wParam), reinterpret_cast<HANDLE>(lParam)));
	case STM_GETIMAGE:
		return reinterpret_cast<LRESULT>(m_image);
	case STM_SETICON:
		return reinterpret_cast<LRESULT>(setImage(IMAGE_ICON, reinterpret_cast<HANDLE>(wParam)));
	case STM_GETICON:
		return reinterpret_cast<LRESULT>(m_imageType == IMAGE_ICON ? m_image : nullptr);
	case WM_NCHITTEST:
		return (style() & SS_NOTIFY) != 0 ? HTCLIENT : HTTRANSPARENT;
	case WM_GETDLGCODE:
		return DLGC_STATIC;
	case WM_LBUTTONUP:
		if ((style() & SS_NOTIFY) != 0)
		{
			notifyCommand(STN_CLICKED);
		}
		return 0;
	case WM_LBUTTONDBLCLK:
		if ((style() & SS_NOTIFY) != 0)
		{
			notifyCommand(STN_DBLCLK);
		}
		return 0;
	default:
		return Win32Control::handleMessage(message, wParam, lParam);
	}
}

void Win32Static::drawText(const Win32DrawContext& context)
{
	const std::string label = (style() & SS_NOPREFIX) != 0 ? text() : Win32ControlDrawing::withoutMnemonic(text());
	if (label.empty())
	{
		return;
	}

	const ImVec2 origin = Win32ControlDrawing::screenPoint(context, 0, 0);
	const ImVec2 size = Win32ControlDrawing::clientSize(context);
	const bool wraps = staticType() != SS_LEFTNOWORDWRAP && staticType() != SS_SIMPLE;
	const ImVec2 textSize = ImGui::CalcTextSize(label.c_str(), nullptr, false, wraps ? size.x : -1.0f);
	float x = origin.x;
	if (staticType() == SS_CENTER)
	{
		x += (size.x - textSize.x) * 0.5f;
	}
	if (staticType() == SS_RIGHT)
	{
		x += size.x - textSize.x;
	}
	const float y = (style() & SS_CENTERIMAGE) != 0 ? origin.y + (size.y - textSize.y) * 0.5f : origin.y;
	const ImU32 color = Win32ControlDrawing::color(context.isEnabled ? ImGuiCol_Text : ImGuiCol_TextDisabled);
	context.drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(x, y), color, label.c_str(), nullptr, wraps ? size.x : 0.0f);
}

void Win32Static::drawImage(const Win32DrawContext& context)
{
	Win32ImagePtr image = imageOf(m_image, m_imageType);
	const ImTextureID texture = image != nullptr ? image->textureForDrawing() : ImTextureID_Invalid;
	if (texture == ImTextureID_Invalid)
	{
		return;
	}

	const ImVec2 origin = Win32ControlDrawing::screenPoint(context, 0, 0);
	const ImVec2 size = Win32ControlDrawing::clientSize(context);
	const ImVec2 imageSize(static_cast<float>(image->width), static_cast<float>(image->height));
	const bool centers = (style() & SS_CENTERIMAGE) != 0;
	const ImVec2 min = centers ? ImVec2(origin.x + (size.x - imageSize.x) * 0.5f, origin.y + (size.y - imageSize.y) * 0.5f) : origin;
	context.drawList->AddImage(ImTextureRef(texture), min, ImVec2(min.x + imageSize.x, min.y + imageSize.y));
}

void Win32Static::drawFrame(const Win32DrawContext& context)
{
	const ImVec2 min = Win32ControlDrawing::screenPoint(context, 0, 0);
	const ImVec2 size = Win32ControlDrawing::clientSize(context);
	const ImVec2 max(min.x + size.x, min.y + size.y);
	const UINT type = staticType();
	const ImU32 color = frameColor(type);
	switch (type)
	{
	case SS_BLACKRECT:
	case SS_GRAYRECT:
	case SS_WHITERECT:
		context.drawList->AddRectFilled(min, max, color);
		return;
	case SS_ETCHEDHORZ:
		context.drawList->AddLine(min, ImVec2(max.x, min.y), color);
		return;
	case SS_ETCHEDVERT:
		context.drawList->AddLine(min, ImVec2(min.x, max.y), color);
		return;
	default:
		context.drawList->AddRect(min, max, color);
		return;
	}
}

void Win32Static::draw(const Win32DrawContext& context)
{
	const UINT type = staticType();
	const bool isFrame = (type >= SS_BLACKRECT && type <= SS_WHITEFRAME) || (type >= SS_ETCHEDHORZ && type <= SS_ETCHEDFRAME);
	if (isFrame)
	{
		drawFrame(context);
	}
	else if (isImage())
	{
		drawImage(context);
	}
	else
	{
		drawText(context);
	}

	if ((style() & SS_SUNKEN) != 0)
	{
		const ImVec2 min = Win32ControlDrawing::screenPoint(context, 0, 0);
		const ImVec2 size = Win32ControlDrawing::clientSize(context);
		context.drawList->AddRect(min, ImVec2(min.x + size.x, min.y + size.y), Win32ControlDrawing::color(ImGuiCol_Border));
	}
}
