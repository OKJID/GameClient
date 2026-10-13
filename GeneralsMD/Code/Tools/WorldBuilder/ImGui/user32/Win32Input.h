#pragma once

#include "Win32Api.h"

#include "imgui.h"

namespace Win32Input
{
ImGuiKey imGuiKeyFor(int virtualKey);
int virtualKeyFor(ImGuiKey key);
bool isVirtualKeyDown(int virtualKey);
WPARAM mouseKeyFlags();
}
