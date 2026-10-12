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

HDC BeginPaint(HWND hwnd, PAINTSTRUCT* paint)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->beginPaint(hwnd, paint) : nullptr;
}

BOOL EndPaint(HWND hwnd, const PAINTSTRUCT* paint)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->endPaint(hwnd, paint) : FALSE;
}

HGDIOBJ SelectObject(HDC hdc, HGDIOBJ object)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->selectObject(hdc, object) : nullptr;
}

BOOL DeleteObject(HGDIOBJ object)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->deleteObject(object) : FALSE;
}

HPEN CreatePen(int style, int width, COLORREF color)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->createPen(style, width, color) : nullptr;
}

HBRUSH CreateSolidBrush(COLORREF color)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->createSolidBrush(color) : nullptr;
}

HFONT CreateFontIndirect(const LOGFONT* font)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->createFontIndirect(font) : nullptr;
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

BOOL MoveToEx(HDC hdc, int x, int y, POINT* previous)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->moveTo(hdc, x, y, previous) : FALSE;
}

BOOL LineTo(HDC hdc, int x, int y)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->lineTo(hdc, x, y) : FALSE;
}

BOOL Polyline(HDC hdc, const POINT* points, int count)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->polyline(hdc, points, count) : FALSE;
}

BOOL TextOutA(HDC hdc, int x, int y, const char* text, int length)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->textOut(hdc, x, y, text, length) : FALSE;
}

int FrameRect(HDC hdc, const RECT* rect, HBRUSH brush)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->frameRect(hdc, rect, brush) : 0;
}

int FillRect(HDC hdc, const RECT* rect, HBRUSH brush)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->fillRect(hdc, rect, brush) : 0;
}

BOOL Ellipse(HDC hdc, int left, int top, int right, int bottom)
{
	return s_gdiBackend != nullptr ? s_gdiBackend->ellipse(hdc, left, top, right, bottom) : FALSE;
}
