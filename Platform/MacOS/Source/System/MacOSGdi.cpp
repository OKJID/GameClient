// MacOSGdi.cpp — Win32 GDI and MessageBox entry points dispatched to an optional host backend

#include "MacOSGdiBackend.h"

#include <cstdio>

static MacOSGdiBackend* s_gdiBackend = nullptr;
static MacOSMessageBoxHandler s_messageBoxHandler = nullptr;

void MacOS_SetGdiBackend(MacOSGdiBackend* backend)
{
	s_gdiBackend = backend;
}

void MacOS_SetMessageBoxHandler(MacOSMessageBoxHandler handler)
{
	s_messageBoxHandler = handler;
}

int MessageBoxA(HWND hwnd, const char* text, const char* caption, UINT type)
{
	if (s_messageBoxHandler != nullptr)
	{
		return s_messageBoxHandler(hwnd, text, caption, type);
	}

	return printf("[MessageBox] %s: %s\n", caption, text);
}

HDC GetDC(HWND hwnd)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->getDC(hwnd) : nullptr;
}

int ReleaseDC(HWND hwnd, HDC hdc)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->releaseDC(hwnd, hdc) : 0;
}

HGDIOBJ SelectObject(HDC hdc, HGDIOBJ object)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->selectObject(hdc, object) : nullptr;
}

BOOL DeleteObject(HGDIOBJ object)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->deleteObject(object) : FALSE;
}

COLORREF SetBkColor(HDC hdc, COLORREF color)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->setBkColor(hdc, color) : 0;
}

COLORREF SetTextColor(HDC hdc, COLORREF color)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->setTextColor(hdc, color) : 0;
}

int SetBkMode(HDC hdc, int mode)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->setBkMode(hdc, mode) : 0;
}
