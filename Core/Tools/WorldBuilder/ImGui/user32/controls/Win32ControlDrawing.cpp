#include "Win32ControlDrawing.h"

namespace Win32ControlDrawing
{

std::string withoutMnemonic(const std::string& text)
{
	std::string result;
	result.reserve(text.size());
	for (size_t index = 0; index < text.size(); ++index)
	{
		if (text[index] != '&')
		{
			result += text[index];
			continue;
		}
		if (index + 1 < text.size() && text[index + 1] == '&')
		{
			result += '&';
			++index;
		}
	}
	return result;
}

ImVec2 screenPoint(const Win32DrawContext& context, int x, int y)
{
	return ImVec2(static_cast<float>(context.clientOrigin.x + x), static_cast<float>(context.clientOrigin.y + y));
}

ImVec2 clientSize(const Win32DrawContext& context)
{
	return ImVec2(static_cast<float>(context.clientSize.cx), static_cast<float>(context.clientSize.cy));
}

void placeCursor(const Win32DrawContext& context)
{
	ImGui::SetCursorScreenPos(screenPoint(context, 0, 0));
}

ImU32 color(ImGuiCol colorId)
{
	return ImGui::GetColorU32(colorId);
}

void drawSunkenFrame(ImDrawList* drawList, ImVec2 min, ImVec2 max)
{
	drawList->AddRectFilled(min, max, color(ImGuiCol_FrameBg));
	drawList->AddRect(min, max, color(ImGuiCol_Border));
}

}
