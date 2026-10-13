#pragma once

#include "Win32Api.h"

struct ImGuiStyle;

namespace Win32SystemColors
{
COLORREF color(int index);
void applyToStyle(ImGuiStyle& style);
}
