#pragma once

#include <windows.h>

class MacOSGdiBackend
{
public:
	virtual ~MacOSGdiBackend() = default;

	virtual HDC getDC(HWND hwnd) = 0;
	virtual int releaseDC(HWND hwnd, HDC hdc) = 0;
	virtual HDC beginPaint(HWND hwnd, PAINTSTRUCT* paint) = 0;
	virtual BOOL endPaint(HWND hwnd, const PAINTSTRUCT* paint) = 0;

	virtual HGDIOBJ selectObject(HDC hdc, HGDIOBJ object) = 0;
	virtual BOOL deleteObject(HGDIOBJ object) = 0;
	virtual HPEN createPen(int style, int width, COLORREF color) = 0;
	virtual HBRUSH createSolidBrush(COLORREF color) = 0;
	virtual HFONT createFontIndirect(const LOGFONT* font) = 0;

	virtual COLORREF setBkColor(HDC hdc, COLORREF color) = 0;
	virtual COLORREF setTextColor(HDC hdc, COLORREF color) = 0;
	virtual int setBkMode(HDC hdc, int mode) = 0;

	virtual BOOL moveTo(HDC hdc, int x, int y, POINT* previous) = 0;
	virtual BOOL lineTo(HDC hdc, int x, int y) = 0;
	virtual BOOL polyline(HDC hdc, const POINT* points, int count) = 0;
	virtual BOOL textOut(HDC hdc, int x, int y, const char* text, int length) = 0;
	virtual int frameRect(HDC hdc, const RECT* rect, HBRUSH brush) = 0;
	virtual int fillRect(HDC hdc, const RECT* rect, HBRUSH brush) = 0;
	virtual BOOL ellipse(HDC hdc, int left, int top, int right, int bottom) = 0;
};

void MacOS_SetGdiBackend(MacOSGdiBackend* backend);
