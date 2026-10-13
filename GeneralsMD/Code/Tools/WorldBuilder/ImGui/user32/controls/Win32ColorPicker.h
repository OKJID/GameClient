#pragma once

#include "Win32Control.h"

#include <array>

#define WIN32_COLORPICKER_CLASS "Win32ColorPicker"
#define CPM_SETCOLOR (WM_USER + 1)
#define CPM_GETCOLOR (WM_USER + 2)
#define CPM_SETCUSTOMCOLORS (WM_USER + 3)

class Win32ColorPicker : public Win32Control
{
public:
	static const int kCustomColorCount = 16;

	explicit Win32ColorPicker(Win32Window& window) : Win32Control(window) {}

	LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
	void draw(const Win32DrawContext& context) override;
	bool drawsOwnFocus() const override { return false; }

private:
	void drawCustomColors(float swatchSize);

	COLORREF m_color = 0;
	std::array<COLORREF, kCustomColorCount> m_customColors = {};
};
