#pragma once

#include "Win32Api.h"

struct Win32MouseInput
{
	UINT message = WM_MOUSEMOVE;
	POINT screenPoint = {};
	WPARAM keyState = 0;
	int wheelDelta = 0;
	HWND rootWindow = nullptr;
};

namespace Win32InputQueue
{
void postMouse(const Win32MouseInput& input);
void postKeyboard(UINT message, WPARAM virtualKey, LPARAM keyData);
bool take(LPMSG message, HWND hwnd, UINT filterMin, UINT filterMax, bool remove);
}
