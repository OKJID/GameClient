#include "Win32ColorPicker.h"

#include "Win32ControlDrawing.h"

#include <algorithm>
#include <cstdio>

namespace
{

const int kSwatchesPerRow = 8;
const int kSwatchRows = 2;

ImVec4 toImVec4(COLORREF color)
{
	return ImVec4(GetRValue(color) / 255.0f, GetGValue(color) / 255.0f, GetBValue(color) / 255.0f, 1.0f);
}

COLORREF toColorRef(const float* rgb)
{
	const auto channel = [](float value) { return static_cast<BYTE>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f); };
	return RGB(channel(rgb[0]), channel(rgb[1]), channel(rgb[2]));
}

}

LRESULT Win32ColorPicker::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case CPM_SETCOLOR:
		m_color = static_cast<COLORREF>(lParam);
		return 0;
	case CPM_GETCOLOR:
		return static_cast<LRESULT>(m_color);
	case CPM_SETCUSTOMCOLORS:
	{
		const COLORREF* colors = reinterpret_cast<const COLORREF*>(lParam);
		if (colors != nullptr)
		{
			std::copy(colors, colors + kCustomColorCount, m_customColors.begin());
		}
		return 0;
	}
	default:
		return Win32Control::handleMessage(message, wParam, lParam);
	}
}

void Win32ColorPicker::draw(const Win32DrawContext& context)
{
	const ImGuiStyle& style = ImGui::GetStyle();
	const float swatchSize = ImGui::GetFrameHeight();
	const float swatchesHeight = (swatchSize + style.ItemSpacing.y) * kSwatchRows;
	const ImVec2 size = Win32ControlDrawing::clientSize(context);
	const float pickerWidth = std::max(1.0f, std::min(size.x, size.y - swatchesHeight));

	Win32ControlDrawing::placeCursor(context);
	float rgb[3] = { GetRValue(m_color) / 255.0f, GetGValue(m_color) / 255.0f, GetBValue(m_color) / 255.0f };
	ImGui::SetNextItemWidth(pickerWidth);
	const ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB;
	if (ImGui::ColorPicker3("##color", rgb, flags))
	{
		const COLORREF picked = toColorRef(rgb);
		queueInput([this, picked]() { m_color = picked; });
	}
	drawCustomColors(swatchSize);
}

void Win32ColorPicker::drawCustomColors(float swatchSize)
{
	for (int index = 0; index < kCustomColorCount; ++index)
	{
		if (index % kSwatchesPerRow != 0)
		{
			ImGui::SameLine();
		}
		char label[24];
		snprintf(label, sizeof(label), "##custom%d", index);
		const COLORREF custom = m_customColors[static_cast<size_t>(index)];
		if (ImGui::ColorButton(label, toImVec4(custom), ImGuiColorEditFlags_NoAlpha, ImVec2(swatchSize, swatchSize)))
		{
			queueInput([this, custom]() { m_color = custom; });
		}
	}
}
