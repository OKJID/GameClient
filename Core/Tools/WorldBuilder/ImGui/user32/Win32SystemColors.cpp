#include "Win32SystemColors.h"

#include "imgui.h"

namespace
{

ImVec4 colorVector(int index)
{
	const COLORREF color = Win32SystemColors::color(index);
	return ImVec4(GetRValue(color) / 255.0f, GetGValue(color) / 255.0f, GetBValue(color) / 255.0f, 1.0f);
}

ImVec4 blend(int fromIndex, int toIndex, float amount)
{
	const ImVec4 from = colorVector(fromIndex);
	const ImVec4 to = colorVector(toIndex);
	return ImVec4(from.x + (to.x - from.x) * amount, from.y + (to.y - from.y) * amount, from.z + (to.z - from.z) * amount, 1.0f);
}

ImVec4 translucent(int index, float alpha)
{
	ImVec4 color = colorVector(index);
	color.w = alpha;
	return color;
}

void applyWindowColors(ImVec4* colors)
{
	colors[ImGuiCol_Text] = colorVector(COLOR_BTNTEXT);
	colors[ImGuiCol_TextDisabled] = colorVector(COLOR_GRAYTEXT);
	colors[ImGuiCol_WindowBg] = colorVector(COLOR_BTNFACE);
	colors[ImGuiCol_ChildBg] = colorVector(COLOR_BTNFACE);
	colors[ImGuiCol_PopupBg] = colorVector(COLOR_MENU);
	colors[ImGuiCol_Border] = colorVector(COLOR_BTNSHADOW);
	colors[ImGuiCol_BorderShadow] = translucent(COLOR_BTNHIGHLIGHT, 0.0f);
	colors[ImGuiCol_TitleBg] = colorVector(COLOR_INACTIVECAPTION);
	colors[ImGuiCol_TitleBgActive] = colorVector(COLOR_ACTIVECAPTION);
	colors[ImGuiCol_TitleBgCollapsed] = colorVector(COLOR_INACTIVECAPTION);
	colors[ImGuiCol_MenuBarBg] = colorVector(COLOR_MENUBAR);
	colors[ImGuiCol_ScrollbarBg] = blend(COLOR_SCROLLBAR, COLOR_BTNHIGHLIGHT, 0.5f);
	colors[ImGuiCol_ScrollbarGrab] = blend(COLOR_BTNFACE, COLOR_BTNSHADOW, 0.35f);
	colors[ImGuiCol_ScrollbarGrabHovered] = blend(COLOR_BTNFACE, COLOR_BTNSHADOW, 0.5f);
	colors[ImGuiCol_ScrollbarGrabActive] = colorVector(COLOR_BTNSHADOW);
	colors[ImGuiCol_Separator] = colorVector(COLOR_BTNSHADOW);
	colors[ImGuiCol_SeparatorHovered] = colorVector(COLOR_BTNSHADOW);
	colors[ImGuiCol_SeparatorActive] = colorVector(COLOR_3DDKSHADOW);
	colors[ImGuiCol_ResizeGrip] = translucent(COLOR_BTNSHADOW, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = translucent(COLOR_BTNSHADOW, 0.6f);
	colors[ImGuiCol_ResizeGripActive] = colorVector(COLOR_BTNSHADOW);
}

void applyControlColors(ImVec4* colors)
{
	colors[ImGuiCol_FrameBg] = colorVector(COLOR_WINDOW);
	colors[ImGuiCol_FrameBgHovered] = colorVector(COLOR_WINDOW);
	colors[ImGuiCol_FrameBgActive] = colorVector(COLOR_WINDOW);
	colors[ImGuiCol_CheckMark] = colorVector(COLOR_WINDOWTEXT);
	colors[ImGuiCol_SliderGrab] = colorVector(COLOR_BTNFACE);
	colors[ImGuiCol_SliderGrabActive] = colorVector(COLOR_BTNSHADOW);
	colors[ImGuiCol_Button] = colorVector(COLOR_BTNFACE);
	colors[ImGuiCol_ButtonHovered] = colorVector(COLOR_BTNFACE);
	colors[ImGuiCol_ButtonActive] = blend(COLOR_BTNFACE, COLOR_BTNSHADOW, 0.5f);
	colors[ImGuiCol_Header] = colorVector(COLOR_GRADIENTACTIVECAPTION);
	colors[ImGuiCol_HeaderHovered] = colorVector(COLOR_GRADIENTACTIVECAPTION);
	colors[ImGuiCol_HeaderActive] = colorVector(COLOR_GRADIENTACTIVECAPTION);
	colors[ImGuiCol_TextSelectedBg] = colorVector(COLOR_GRADIENTACTIVECAPTION);
	colors[ImGuiCol_PlotHistogram] = colorVector(COLOR_HIGHLIGHT);
	colors[ImGuiCol_PlotHistogramHovered] = colorVector(COLOR_HIGHLIGHT);
	colors[ImGuiCol_NavCursor] = colorVector(COLOR_WINDOWTEXT);
}

void applyTabAndTableColors(ImVec4* colors)
{
	colors[ImGuiCol_Tab] = blend(COLOR_BTNFACE, COLOR_BTNSHADOW, 0.2f);
	colors[ImGuiCol_TabHovered] = colorVector(COLOR_BTNFACE);
	colors[ImGuiCol_TabSelected] = colorVector(COLOR_BTNFACE);
	colors[ImGuiCol_TabSelectedOverline] = colorVector(COLOR_HIGHLIGHT);
	colors[ImGuiCol_TabDimmed] = blend(COLOR_BTNFACE, COLOR_BTNSHADOW, 0.2f);
	colors[ImGuiCol_TabDimmedSelected] = colorVector(COLOR_BTNFACE);
	colors[ImGuiCol_TabDimmedSelectedOverline] = colorVector(COLOR_BTNSHADOW);
	colors[ImGuiCol_TableHeaderBg] = colorVector(COLOR_BTNFACE);
	colors[ImGuiCol_TableBorderStrong] = colorVector(COLOR_BTNSHADOW);
	colors[ImGuiCol_TableBorderLight] = colorVector(COLOR_3DLIGHT);
	colors[ImGuiCol_TableRowBg] = translucent(COLOR_WINDOW, 0.0f);
	colors[ImGuiCol_TableRowBgAlt] = translucent(COLOR_BTNSHADOW, 0.08f);
}

void applyClassicShapes(ImGuiStyle& style)
{
	style.WindowRounding = 0.0f;
	style.ChildRounding = 0.0f;
	style.FrameRounding = 0.0f;
	style.PopupRounding = 0.0f;
	style.ScrollbarRounding = 0.0f;
	style.GrabRounding = 0.0f;
	style.TabRounding = 0.0f;
	style.WindowBorderSize = 1.0f;
	style.PopupBorderSize = 1.0f;
}

}

COLORREF Win32SystemColors::color(int index)
{
	switch (index)
	{
	case COLOR_BACKGROUND:
		return RGB(58, 110, 165);
	case COLOR_ACTIVECAPTION:
	case COLOR_HIGHLIGHT:
	case COLOR_MENUHILIGHT:
		return RGB(10, 36, 106);
	case COLOR_INACTIVECAPTION:
	case COLOR_APPWORKSPACE:
	case COLOR_BTNSHADOW:
	case COLOR_GRAYTEXT:
		return RGB(128, 128, 128);
	case COLOR_WINDOW:
	case COLOR_CAPTIONTEXT:
	case COLOR_HIGHLIGHTTEXT:
	case COLOR_BTNHIGHLIGHT:
		return RGB(255, 255, 255);
	case COLOR_WINDOWFRAME:
	case COLOR_MENUTEXT:
	case COLOR_WINDOWTEXT:
	case COLOR_BTNTEXT:
	case COLOR_INFOTEXT:
		return RGB(0, 0, 0);
	case COLOR_3DDKSHADOW:
		return RGB(64, 64, 64);
	case COLOR_INFOBK:
		return RGB(255, 255, 225);
	case COLOR_HOTLIGHT:
		return RGB(0, 0, 128);
	case COLOR_GRADIENTACTIVECAPTION:
		return RGB(166, 202, 240);
	case COLOR_GRADIENTINACTIVECAPTION:
		return RGB(192, 192, 192);
	default:
		return RGB(212, 208, 200);
	}
}

void Win32SystemColors::applyToStyle(ImGuiStyle& style)
{
	ImGui::StyleColorsLight(&style);
	applyWindowColors(style.Colors);
	applyControlColors(style.Colors);
	applyTabAndTableColors(style.Colors);
	applyClassicShapes(style);
}
