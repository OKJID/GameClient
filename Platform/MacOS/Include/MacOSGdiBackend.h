#pragma once

#include <windows.h>

class MacOSGdiBackend
{
public:
	virtual ~MacOSGdiBackend() = default;

	virtual HDC getDC(HWND hwnd) = 0;
	virtual int releaseDC(HWND hwnd, HDC hdc) = 0;
	virtual HGDIOBJ selectObject(HDC hdc, HGDIOBJ object) = 0;
	virtual BOOL deleteObject(HGDIOBJ object) = 0;
	virtual COLORREF setBkColor(HDC hdc, COLORREF color) = 0;
	virtual COLORREF setTextColor(HDC hdc, COLORREF color) = 0;
	virtual int setBkMode(HDC hdc, int mode) = 0;
};

void MacOS_SetGdiBackend(MacOSGdiBackend* backend);
