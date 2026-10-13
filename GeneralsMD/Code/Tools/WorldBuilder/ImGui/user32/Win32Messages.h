#pragma once

#include "Win32Window.h"

#include <functional>

#define WM_WB_TASK 0x03F0

class Win32FramePump
{
public:
	virtual ~Win32FramePump() = default;

	virtual void runFrame() = 0;
};

typedef std::function<void()> Win32Task;

void Win32SetFramePump(Win32FramePump* pump);
DWORD Win32CurrentTime();
bool Win32MatchesMessageFilter(const MSG& message, HWND hwnd, UINT filterMin, UINT filterMax);

void Win32PostTask(HWND hwnd, Win32Task task);
void Win32RemoveWindowMessages(Win32Window* window);
void Win32KillWindowTimers(Win32Window* window);
LRESULT Win32CallCbtHooks(int code, WPARAM wParam, LPARAM lParam);
void Win32InvalidateTree(Win32Window* window, bool erase);
