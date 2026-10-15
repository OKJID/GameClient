#include "Win32ColorPicker.h"
#include "Win32CommonDialogs.h"
#include "Win32Dialog.h"
#include "Win32Resources.h"

namespace
{

const UINT kPickerId = 710;
const UINT kAddCustomColorId = 712;
const DWORD kButtonStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP;

struct ColorDialogState
{
	CHOOSECOLORA* chooseColor = nullptr;
	int nextCustomColor = 0;
};

Win32DialogTemplate colorDialogTemplate()
{
	Win32DialogTemplate dialog;
	dialog.style = DS_MODALFRAME | DS_SETFONT | WS_POPUP | WS_CAPTION | WS_SYSMENU;
	dialog.cx = 236;
	dialog.cy = 222;
	dialog.caption = "Color";
	dialog.fontName = "MS Shell Dlg";
	dialog.fontSize = 8;
	dialog.items = {
		{ WIN32_COLORPICKER_CLASS, "", 0, kPickerId, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 7, 7, 222, 186 },
		{ "Button", "&Add to Custom Colors", 0, kAddCustomColorId, kButtonStyle | BS_PUSHBUTTON, 0, 7, 200, 100, 14 },
		{ "Button", "OK", 0, IDOK, kButtonStyle | BS_DEFPUSHBUTTON, 0, 124, 200, 50, 14 },
		{ "Button", "Cancel", 0, IDCANCEL, kButtonStyle | BS_PUSHBUTTON, 0, 179, 200, 50, 14 },
	};
	return dialog;
}

ColorDialogState* stateOf(HWND dialog)
{
	return reinterpret_cast<ColorDialogState*>(GetWindowLongPtrA(dialog, DWLP_USER));
}

void initializeDialog(HWND dialog, ColorDialogState* state)
{
	SetWindowLongPtrA(dialog, DWLP_USER, reinterpret_cast<LONG_PTR>(state));
	const CHOOSECOLORA& chooseColor = *state->chooseColor;
	const HWND picker = GetDlgItem(dialog, kPickerId);
	SendMessageA(picker, CPM_SETCUSTOMCOLORS, 0, reinterpret_cast<LPARAM>(chooseColor.lpCustColors));
	SendMessageA(picker, CPM_SETCOLOR, 0, static_cast<LPARAM>((chooseColor.Flags & CC_RGBINIT) != 0 ? chooseColor.rgbResult : 0));
}

void addCustomColor(HWND dialog, ColorDialogState* state)
{
	COLORREF* customColors = state->chooseColor->lpCustColors;
	if (customColors == nullptr)
	{
		return;
	}
	const HWND picker = GetDlgItem(dialog, kPickerId);
	customColors[state->nextCustomColor] = static_cast<COLORREF>(SendMessageA(picker, CPM_GETCOLOR, 0, 0));
	state->nextCustomColor = (state->nextCustomColor + 1) % Win32ColorPicker::kCustomColorCount;
	SendMessageA(picker, CPM_SETCUSTOMCOLORS, 0, reinterpret_cast<LPARAM>(customColors));
}

INT_PTR handleCommand(HWND dialog, UINT commandId)
{
	ColorDialogState* state = stateOf(dialog);
	switch (commandId)
	{
	case IDOK:
		state->chooseColor->rgbResult = static_cast<COLORREF>(SendDlgItemMessageA(dialog, kPickerId, CPM_GETCOLOR, 0, 0));
		EndDialog(dialog, IDOK);
		return TRUE;
	case IDCANCEL:
		EndDialog(dialog, IDCANCEL);
		return TRUE;
	case kAddCustomColorId:
		addCustomColor(dialog, state);
		return TRUE;
	default:
		return FALSE;
	}
}

INT_PTR CALLBACK colorDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_INITDIALOG)
	{
		initializeDialog(dialog, reinterpret_cast<ColorDialogState*>(lParam));
		return TRUE;
	}
	if (message == WM_COMMAND)
	{
		return handleCommand(dialog, LOWORD(wParam));
	}
	return FALSE;
}

}

BOOL ChooseColorA(LPCHOOSECOLOR chooseColor)
{
	Win32SetCommonDialogError(0);
	if (chooseColor == nullptr || chooseColor->lStructSize != sizeof(CHOOSECOLORA))
	{
		Win32SetCommonDialogError(CDERR_STRUCTSIZE);
		return FALSE;
	}
	ColorDialogState state;
	state.chooseColor = chooseColor;
	const INT_PTR result = Win32Dialogs::runModal(nullptr, colorDialogTemplate(), chooseColor->hwndOwner, colorDialogProc, reinterpret_cast<LPARAM>(&state));
	if (result == -1)
	{
		Win32SetCommonDialogError(CDERR_DIALOGFAILURE);
	}
	return result == IDOK ? TRUE : FALSE;
}
