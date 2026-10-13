#include "Win32Control.h"

#include "Win32Class.h"

#include <cstdio>

LRESULT Win32Control::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	return defaultProc(message, wParam, lParam);
}

void Win32Control::queueInput(Win32Task action)
{
	Win32PostTask(hwnd(), std::move(action));
}

HWND Win32Control::parentHwnd() const
{
	return m_window.parent != nullptr ? m_window.parent->hwnd : nullptr;
}

const char* Win32Control::widgetId() const
{
	snprintf(m_widgetId, sizeof(m_widgetId), "##hwnd%p", static_cast<void*>(m_window.hwnd));
	return m_widgetId;
}

LRESULT Win32Control::defaultProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProcA(hwnd(), message, wParam, lParam);
}

LRESULT Win32Control::eraseWithSystemColor(HDC hdc, int colorIndex)
{
	RECT box;
	if (GetClipBox(hdc, &box) == ERROR)
	{
		return FALSE;
	}
	FillRect(hdc, &box, GetSysColorBrush(colorIndex));
	return TRUE;
}

void Win32Control::notifyCommand(WORD code)
{
	const HWND parent = parentHwnd();
	if (parent == nullptr)
	{
		return;
	}
	SendMessageA(parent, WM_COMMAND, MAKEWPARAM(controlId(), code), reinterpret_cast<LPARAM>(hwnd()));
}

LRESULT Win32Control::notifyParent(NMHDR& header, UINT code)
{
	const HWND parent = parentHwnd();
	if (parent == nullptr)
	{
		return 0;
	}

	header.hwndFrom = hwnd();
	header.idFrom = static_cast<UINT_PTR>(controlId());
	header.code = code;
	return SendMessageA(parent, WM_NOTIFY, static_cast<WPARAM>(controlId()), reinterpret_cast<LPARAM>(&header));
}

void Win32Control::takeFocus()
{
	if (GetFocus() != hwnd())
	{
		SetFocus(hwnd());
	}
}

LRESULT Win32ControlWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return 0;
	}

	if (message == WM_NCCREATE && window->control == nullptr && window->windowClass->controlFactory != nullptr)
	{
		window->control = window->windowClass->controlFactory(*window);
	}

	if (window->control == nullptr)
	{
		return DefWindowProcA(hwnd, message, wParam, lParam);
	}
	if (message == WM_PAINT)
	{
		window->controlReceivedPaint = true;
	}
	return window->control->handleMessage(message, wParam, lParam);
}
