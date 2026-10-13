#pragma once

#include "Win32Window.h"

struct Win32DialogTemplate;

struct Win32DialogState
{
	DLGPROC dialogProc = nullptr;
	INT_PTR result = 0;
	bool hasEnded = false;
	UINT defaultId = 0;
	HWND savedFocus = nullptr;
	HFONT font = nullptr;
};

namespace Win32Dialogs
{
void registerDialogClass();
Win32DialogState* stateOf(HWND dialog);
HWND createFromTemplate(HINSTANCE instance, const Win32DialogTemplate& dialogTemplate, HWND parent, DLGPROC dialogProc, LPARAM initParam);
INT_PTR runModal(HINSTANCE instance, const Win32DialogTemplate& dialogTemplate, HWND parent, DLGPROC dialogProc, LPARAM initParam);
RECT templateRectInPixels(int x, int y, int cx, int cy);
}
