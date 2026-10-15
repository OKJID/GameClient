#include "Win32Toolbar.h"

#include "Win32ControlDrawing.h"

#include <algorithm>

namespace
{

const int kDefaultSeparatorWidth = 8;

ImVec2 toScreen(const Win32DrawContext& context, int x, int y)
{
	return Win32ControlDrawing::screenPoint(context, x, y);
}

}

int Win32Toolbar::separatorWidth(const TBBUTTON& button) const
{
	return button.iBitmap > 0 ? button.iBitmap : kDefaultSeparatorWidth;
}

RECT Win32Toolbar::itemRect(int index) const
{
	int x = 0;
	int y = 0;
	RECT rect = {};
	for (int current = 0; current <= index; ++current)
	{
		const TBBUTTON& button = m_buttons[static_cast<size_t>(current)];
		if (!isVisible(button))
		{
			SetRectEmpty(&rect);
			continue;
		}
		const int width = isSeparator(button) ? separatorWidth(button) : m_buttonSize.cx;
		SetRect(&rect, x, y, x + width, y + m_buttonSize.cy);
		x += width;
		if ((button.fsState & TBSTATE_WRAP) != 0)
		{
			x = 0;
			y += m_buttonSize.cy;
		}
	}
	return rect;
}

int Win32Toolbar::itemAt(POINT point) const
{
	for (int index = 0; index < buttonCount(); ++index)
	{
		const RECT rect = itemRect(index);
		if (PtInRect(&rect, point))
		{
			return index;
		}
	}
	return -1;
}

SIZE Win32Toolbar::contentSize() const
{
	SIZE size = { 0, m_buttonSize.cy };
	for (int index = 0; index < buttonCount(); ++index)
	{
		const RECT rect = itemRect(index);
		size.cx = std::max<LONG>(size.cx, rect.right);
		size.cy = std::max<LONG>(size.cy, rect.bottom);
	}
	return size;
}

void Win32Toolbar::draw(const Win32DrawContext& context)
{
	for (int index = 0; index < buttonCount(); ++index)
	{
		drawButton(context, index);
	}
	drawToolTip(context);
}

void Win32Toolbar::drawButton(const Win32DrawContext& context, int index)
{
	const TBBUTTON& button = m_buttons[static_cast<size_t>(index)];
	const RECT rect = itemRect(index);
	if (IsRectEmpty(&rect))
	{
		return;
	}
	const ImVec2 min = toScreen(context, rect.left, rect.top);
	const ImVec2 max = toScreen(context, rect.right, rect.bottom);
	if (isSeparator(button))
	{
		const float x = (min.x + max.x) * 0.5f;
		context.drawList->AddLine(ImVec2(x, min.y + 2.0f), ImVec2(x, max.y - 2.0f), Win32ControlDrawing::color(ImGuiCol_Separator));
		return;
	}

	const bool isEnabled = context.isEnabled && (button.fsState & TBSTATE_ENABLED) != 0;
	const bool isPressed = (button.fsState & TBSTATE_PRESSED) != 0;
	const bool isChecked = (button.fsState & TBSTATE_CHECKED) != 0;
	const bool isHot = isEnabled && (m_pressedItem < 0 || m_pressedItem == index) && ImGui::IsMouseHoveringRect(min, max);
	const bool isFlat = (style() & TBSTYLE_FLAT) != 0;
	const float rounding = ImGui::GetStyle().FrameRounding;
	if (isPressed || isChecked || isHot || !isFlat)
	{
		const ImGuiCol background = isPressed || isChecked ? ImGuiCol_ButtonActive : (isHot ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
		context.drawList->AddRectFilled(min, max, Win32ControlDrawing::color(background), rounding);
		context.drawList->AddRect(min, max, Win32ControlDrawing::color(ImGuiCol_Border), rounding);
	}
	const float offset = isPressed ? 1.0f : 0.0f;
	drawImage(context, button, ImVec2(min.x + offset, min.y + offset), ImVec2(max.x + offset, max.y + offset));
}

void Win32Toolbar::drawImage(const Win32DrawContext& context, const TBBUTTON& button, ImVec2 min, ImVec2 max)
{
	const auto block = std::find_if(m_imageBlocks.begin(), m_imageBlocks.end(),
		[&](const ImageBlock& candidate) { return button.iBitmap >= candidate.firstIndex && button.iBitmap < candidate.firstIndex + candidate.count; });
	if (block == m_imageBlocks.end() || block->image->width <= 0 || block->image->height <= 0)
	{
		return;
	}
	const ImTextureID texture = block->image->textureForDrawing();
	if (texture == ImTextureID_Invalid)
	{
		return;
	}

	const float imageWidth = static_cast<float>(block->image->width);
	const float imageHeight = static_cast<float>(block->image->height);
	const float left = static_cast<float>((button.iBitmap - block->firstIndex) * m_bitmapSize.cx);
	const ImVec2 uvMin(left / imageWidth, 0.0f);
	const ImVec2 uvMax((left + static_cast<float>(m_bitmapSize.cx)) / imageWidth, static_cast<float>(m_bitmapSize.cy) / imageHeight);
	const ImVec2 size(static_cast<float>(m_bitmapSize.cx), static_cast<float>(m_bitmapSize.cy));
	const ImVec2 imageMin(min.x + (max.x - min.x - size.x) * 0.5f, min.y + (max.y - min.y - size.y) * 0.5f);
	const bool isEnabled = context.isEnabled && (button.fsState & TBSTATE_ENABLED) != 0;
	const ImU32 tint = isEnabled ? IM_COL32_WHITE : IM_COL32(255, 255, 255, 90);
	context.drawList->AddImage(ImTextureRef(texture), imageMin, ImVec2(imageMin.x + size.x, imageMin.y + size.y), uvMin, uvMax, tint);
}

void Win32Toolbar::drawToolTip(const Win32DrawContext& context)
{
	if (m_hotItem < 0 || m_hotItem >= buttonCount() || m_toolTipText.empty() || m_pressedItem >= 0)
	{
		return;
	}
	const RECT rect = itemRect(m_hotItem);
	if (!ImGui::IsMouseHoveringRect(toScreen(context, rect.left, rect.top), toScreen(context, rect.right, rect.bottom)))
	{
		return;
	}
	if (ImGui::GetTime() - m_hotSince < ImGui::GetStyle().HoverDelayNormal)
	{
		return;
	}
	ImGui::SetTooltip("%s", m_toolTipText.c_str());
}
