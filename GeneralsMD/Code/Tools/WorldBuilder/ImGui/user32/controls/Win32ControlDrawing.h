#pragma once

#include "Win32Control.h"

#include "imgui.h"

#include <string>

namespace Win32ControlDrawing
{
std::string withoutMnemonic(const std::string& text);
ImVec2 screenPoint(const Win32DrawContext& context, int x, int y);
ImVec2 clientSize(const Win32DrawContext& context);
void placeCursor(const Win32DrawContext& context);
ImU32 color(ImGuiCol colorId);
void drawSunkenFrame(ImDrawList* drawList, ImVec2 min, ImVec2 max);
}
