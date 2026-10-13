#include "Win32PropertySheet.h"

#include "Win32Dialog.h"
#include "Win32Resources.h"

#include <algorithm>
#include <iterator>

namespace
{

const int kMarginUnits = 7;
const int kButtonWidthUnits = 50;
const int kButtonHeightUnits = 14;
const int kButtonGapUnits = 4;
const DWORD kButtonStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
const UINT kSheetButtons[] = { IDOK, IDCANCEL, ID_APPLY_NOW, IDHELP };

std::string sheetCaption(DWORD flags, const std::string& title)
{
	return (flags & PSH_PROPTITLE) != 0 ? title + " Properties" : title;
}

Win32DialogTemplate sheetTemplate(const PROPSHEETHEADERA& header)
{
	Win32DialogTemplate dialog;
	dialog.style = DS_MODALFRAME | DS_3DLOOK | DS_SETFONT | WS_POPUP | WS_CAPTION | WS_SYSMENU;
	dialog.style |= (header.dwFlags & PSH_MODELESS) != 0 ? WS_VISIBLE : 0;
	dialog.exStyle = WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT;
	dialog.caption = sheetCaption(header.dwFlags, Win32PropertySheets::resourceText(header.pszCaption));
	dialog.fontName = "MS Shell Dlg";
	dialog.fontSize = 8;
	dialog.items.push_back({ WC_TABCONTROLA, "", 0, Win32PropertySheets::kTabControlId, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS, 0, 0, 0, 0, 0 });
	dialog.items.push_back({ "Button", "OK", 0, IDOK, kButtonStyle | BS_DEFPUSHBUTTON, 0, 0, 0, 0, 0 });
	dialog.items.push_back({ "Button", "Cancel", 0, IDCANCEL, kButtonStyle | BS_PUSHBUTTON, 0, 0, 0, 0, 0 });
	if ((header.dwFlags & PSH_NOAPPLYNOW) == 0)
	{
		dialog.items.push_back({ "Button", "&Apply", 0, ID_APPLY_NOW, kButtonStyle | BS_PUSHBUTTON | WS_DISABLED, 0, 0, 0, 0, 0 });
	}
	if ((header.dwFlags & PSH_HASHELP) != 0)
	{
		dialog.items.push_back({ "Button", "Help", 0, IDHELP, kButtonStyle | BS_PUSHBUTTON, 0, 0, 0, 0, 0 });
	}
	return dialog;
}

Win32PropertySheetState* sheetStateOf(HWND sheet)
{
	return reinterpret_cast<Win32PropertySheetState*>(GetWindowLongPtrA(sheet, DWLP_USER));
}

INT_PTR reply(HWND sheet, LRESULT result)
{
	SetWindowLongPtrA(sheet, DWLP_MSGRESULT, result);
	return TRUE;
}

RECT placeTab(Win32PropertySheetState& state, SIZE margin)
{
	const SIZE pageSize = Win32PropertySheets::largestPageSize(state);
	RECT tabRect = { 0, 0, pageSize.cx, pageSize.cy };
	SendMessageA(state.tab, TCM_ADJUSTRECT, TRUE, reinterpret_cast<LPARAM>(&tabRect));
	OffsetRect(&tabRect, margin.cx - tabRect.left, margin.cy - tabRect.top);
	MoveWindow(state.tab, tabRect.left, tabRect.top, tabRect.right - tabRect.left, tabRect.bottom - tabRect.top, FALSE);

	state.pageRect = tabRect;
	SendMessageA(state.tab, TCM_ADJUSTRECT, FALSE, reinterpret_cast<LPARAM>(&state.pageRect));
	return tabRect;
}

int placeButtons(HWND sheet, const RECT& tabRect, SIZE margin)
{
	const RECT button = Win32Dialogs::templateRectInPixels(0, 0, kButtonWidthUnits, kButtonHeightUnits);
	const int gap = Win32Dialogs::templateRectInPixels(0, 0, kButtonGapUnits, 0).right;
	const int top = tabRect.bottom + margin.cy;
	int right = tabRect.right;
	for (auto it = std::rbegin(kSheetButtons); it != std::rend(kSheetButtons); ++it)
	{
		const HWND control = GetDlgItem(sheet, static_cast<int>(*it));
		if (control == nullptr)
		{
			continue;
		}
		MoveWindow(control, right - button.right, top, button.right, button.bottom, FALSE);
		right -= button.right + gap;
	}
	return top + button.bottom;
}

void layoutSheet(HWND sheet, Win32PropertySheetState& state)
{
	const RECT marginRect = Win32Dialogs::templateRectInPixels(0, 0, kMarginUnits, kMarginUnits);
	const SIZE margin = { marginRect.right, marginRect.bottom };
	const RECT tabRect = placeTab(state, margin);
	const int buttonsBottom = placeButtons(sheet, tabRect, margin);

	RECT window = { 0, 0, tabRect.right + margin.cx, buttonsBottom + margin.cy };
	AdjustWindowRectEx(&window, static_cast<DWORD>(GetWindowLongA(sheet, GWL_STYLE)), FALSE, static_cast<DWORD>(GetWindowLongA(sheet, GWL_EXSTYLE)));
	const int width = window.right - window.left;
	const int height = window.bottom - window.top;
	const int left = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
	const int top = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
	SetWindowPos(sheet, nullptr, left, top, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

void activateStartPage(HWND sheet, Win32PropertySheetState& state)
{
	const int count = static_cast<int>(state.pages.size());
	const int start = std::clamp(static_cast<int>(state.header.nStartPage), 0, std::max(count - 1, 0));
	for (int offset = 0; offset < count; ++offset)
	{
		if (Win32PropertySheets::activatePage(sheet, state, (start + offset) % count))
		{
			return;
		}
	}
}

INT_PTR initializeSheet(HWND sheet, Win32PropertySheetState* state)
{
	SetWindowLongPtrA(sheet, DWLP_USER, reinterpret_cast<LONG_PTR>(state));
	state->tab = GetDlgItem(sheet, static_cast<int>(Win32PropertySheets::kTabControlId));
	for (size_t index = 0; index < state->pages.size(); ++index)
	{
		Win32PropertySheets::insertTab(state->tab, static_cast<int>(index), state->pages[index]->page);
	}
	layoutSheet(sheet, *state);
	activateStartPage(sheet, *state);
	if ((state->header.dwFlags & PSH_USECALLBACK) != 0 && state->header.pfnCallback != nullptr)
	{
		state->header.pfnCallback(sheet, PSCB_INITIALIZED, 0);
	}
	return FALSE;
}

void refreshApplyButton(HWND sheet, const Win32PropertySheetState& state)
{
	const bool hasChanges = std::any_of(state.pages.begin(), state.pages.end(),
		[](const std::unique_ptr<Win32PropertySheetPage>& sheetPage) { return sheetPage->isChanged; });
	EnableWindow(GetDlgItem(sheet, ID_APPLY_NOW), hasChanges ? TRUE : FALSE);
}

bool applyPages(HWND sheet, Win32PropertySheetState& state, BOOL isClosing)
{
	if (!Win32PropertySheets::killActivePage(sheet, state))
	{
		return false;
	}
	for (size_t index = 0; index < state.pages.size(); ++index)
	{
		const HWND page = state.pages[index]->hwnd;
		const LRESULT result = page != nullptr ? Win32PropertySheets::notifyPage(sheet, page, PSN_APPLY, isClosing) : PSNRET_NOERROR;
		if (result == PSNRET_INVALID || result == PSNRET_INVALID_NOCHANGEPAGE)
		{
			const int target = result == PSNRET_INVALID ? static_cast<int>(index) : state.activeIndex;
			Win32PropertySheets::activatePage(sheet, state, target);
			return false;
		}
	}

	state.hasApplied = true;
	for (std::unique_ptr<Win32PropertySheetPage>& sheetPage : state.pages)
	{
		sheetPage->isChanged = false;
	}
	refreshApplyButton(sheet, state);
	return true;
}

void cancelSheet(HWND sheet, Win32PropertySheetState& state)
{
	const HWND active = state.activeIndex >= 0 ? state.pages[static_cast<size_t>(state.activeIndex)]->hwnd : nullptr;
	if (active != nullptr && Win32PropertySheets::notifyPage(sheet, active, PSN_QUERYCANCEL, 0) != 0)
	{
		return;
	}
	for (std::unique_ptr<Win32PropertySheetPage>& sheetPage : state.pages)
	{
		if (sheetPage->hwnd != nullptr)
		{
			Win32PropertySheets::notifyPage(sheet, sheetPage->hwnd, PSN_RESET, 0);
		}
	}
	Win32PropertySheets::destroyPages(state);
}

INT_PTR handleCommand(HWND sheet, Win32PropertySheetState& state, UINT id)
{
	switch (id)
	{
	case IDOK:
		if (applyPages(sheet, state, TRUE))
		{
			Win32PropertySheets::destroyPages(state);
		}
		return TRUE;
	case IDCANCEL:
		cancelSheet(sheet, state);
		return TRUE;
	case ID_APPLY_NOW:
		if (applyPages(sheet, state, FALSE))
		{
			Win32PropertySheets::activatePage(sheet, state, state.activeIndex);
		}
		return TRUE;
	case IDHELP:
		if (state.activeIndex >= 0)
		{
			Win32PropertySheets::notifyPage(sheet, state.pages[static_cast<size_t>(state.activeIndex)]->hwnd, PSN_HELP, 0);
		}
		return TRUE;
	default:
		return FALSE;
	}
}

INT_PTR handleTabNotify(HWND sheet, Win32PropertySheetState& state, const NMHDR* header)
{
	if (header == nullptr || header->hwndFrom != state.tab)
	{
		return FALSE;
	}
	if (header->code == TCN_SELCHANGING)
	{
		return reply(sheet, Win32PropertySheets::killActivePage(sheet, state) ? FALSE : TRUE);
	}
	if (header->code != TCN_SELCHANGE)
	{
		return FALSE;
	}
	const int selected = static_cast<int>(SendMessageA(state.tab, TCM_GETCURSEL, 0, 0));
	if (!Win32PropertySheets::activatePage(sheet, state, selected))
	{
		Win32PropertySheets::activatePage(sheet, state, state.activeIndex);
	}
	return reply(sheet, 0);
}

bool selectPage(HWND sheet, Win32PropertySheetState& state, int index)
{
	if (index == state.activeIndex)
	{
		return true;
	}
	if (!Win32PropertySheets::killActivePage(sheet, state))
	{
		return false;
	}
	if (Win32PropertySheets::activatePage(sheet, state, index))
	{
		return true;
	}
	Win32PropertySheets::activatePage(sheet, state, state.activeIndex);
	return false;
}

LRESULT markChanged(HWND sheet, Win32PropertySheetState& state, HWND page, bool isChanged)
{
	const int index = Win32PropertySheets::pageIndexOf(state, page);
	if (index < 0)
	{
		return 0;
	}
	state.pages[static_cast<size_t>(index)]->isChanged = isChanged;
	refreshApplyButton(sheet, state);
	return 0;
}

LRESULT querySiblings(const Win32PropertySheetState& state, WPARAM wParam, LPARAM lParam)
{
	for (const std::unique_ptr<Win32PropertySheetPage>& sheetPage : state.pages)
	{
		const LRESULT result = sheetPage->hwnd != nullptr ? SendMessageA(sheetPage->hwnd, PSM_QUERYSIBLINGS, wParam, lParam) : 0;
		if (result != 0)
		{
			return result;
		}
	}
	return 0;
}

LRESULT pressButton(HWND sheet, WPARAM button)
{
	static const UINT buttonIds[] = { ID_WIZBACK, ID_WIZNEXT, ID_WIZFINISH, IDOK, ID_APPLY_NOW, IDCANCEL, IDHELP };
	if (button >= sizeof(buttonIds) / sizeof(buttonIds[0]))
	{
		return FALSE;
	}
	const UINT id = buttonIds[button];
	const HWND control = GetDlgItem(sheet, static_cast<int>(id));
	if (control == nullptr || !IsWindowEnabled(control))
	{
		return FALSE;
	}
	SendMessageA(sheet, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), reinterpret_cast<LPARAM>(control));
	return TRUE;
}

LRESULT addPage(Win32PropertySheetState& state, HPROPSHEETPAGE handle)
{
	if (handle == nullptr)
	{
		return FALSE;
	}
	Win32PropertySheets::appendPage(state, handle->page, handle);
	return TRUE;
}

LRESULT removePage(HWND sheet, Win32PropertySheetState& state, WPARAM index, HPROPSHEETPAGE handle)
{
	const int target = handle != nullptr ? Win32PropertySheets::pageIndexOf(state, handle) : static_cast<int>(index);
	return Win32PropertySheets::removePage(sheet, state, target) ? TRUE : FALSE;
}

LRESULT translateSheetMessage(HWND sheet, Win32PropertySheetState& state, MSG* message)
{
	const bool isControlDown = GetKeyState(VK_CONTROL) < 0;
	const bool isPageKey = message->message == WM_KEYDOWN && isControlDown
		&& (message->wParam == VK_TAB || message->wParam == VK_PRIOR || message->wParam == VK_NEXT);
	const int count = static_cast<int>(state.pages.size());
	if (!isPageKey || count == 0 || (message->hwnd != sheet && !IsChild(sheet, message->hwnd)))
	{
		return IsDialogMessageA(sheet, message);
	}

	const bool isBackward = message->wParam == VK_PRIOR || (message->wParam == VK_TAB && GetKeyState(VK_SHIFT) < 0);
	const int next = (state.activeIndex + (isBackward ? count - 1 : 1)) % count;
	selectPage(sheet, state, next);
	return TRUE;
}

LRESULT handleSheetMessage(HWND sheet, Win32PropertySheetState& state, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case PSM_SETCURSEL:
	{
		const HPROPSHEETPAGE handle = reinterpret_cast<HPROPSHEETPAGE>(lParam);
		const int index = handle != nullptr ? Win32PropertySheets::pageIndexOf(state, handle) : static_cast<int>(wParam);
		return selectPage(sheet, state, index) ? TRUE : FALSE;
	}
	case PSM_REMOVEPAGE:
		return removePage(sheet, state, wParam, reinterpret_cast<HPROPSHEETPAGE>(lParam));
	case PSM_ADDPAGE:
		return addPage(state, reinterpret_cast<HPROPSHEETPAGE>(lParam));
	case PSM_CHANGED:
		return markChanged(sheet, state, reinterpret_cast<HWND>(wParam), true);
	case PSM_UNCHANGED:
		return markChanged(sheet, state, reinterpret_cast<HWND>(wParam), false);
	case PSM_CANCELTOCLOSE:
		EnableWindow(GetDlgItem(sheet, IDCANCEL), FALSE);
		SetDlgItemTextA(sheet, IDOK, "Close");
		return 0;
	case PSM_QUERYSIBLINGS:
		return querySiblings(state, wParam, lParam);
	case PSM_APPLY:
		return applyPages(sheet, state, FALSE) ? TRUE : FALSE;
	case PSM_SETTITLEA:
		SetWindowTextA(sheet, sheetCaption(static_cast<DWORD>(wParam), Win32PropertySheets::resourceText(reinterpret_cast<LPCSTR>(lParam))).c_str());
		return 0;
	case PSM_PRESSBUTTON:
		return pressButton(sheet, wParam);
	case PSM_GETTABCONTROL:
		return reinterpret_cast<LRESULT>(state.tab);
	case PSM_ISDIALOGMESSAGE:
		return translateSheetMessage(sheet, state, reinterpret_cast<MSG*>(lParam));
	case PSM_GETCURRENTPAGEHWND:
		return state.activeIndex >= 0 ? reinterpret_cast<LRESULT>(state.pages[static_cast<size_t>(state.activeIndex)]->hwnd) : 0;
	default:
		return 0;
	}
}

void releaseSheet(HWND sheet, Win32PropertySheetState* state)
{
	Win32PropertySheets::destroyPages(*state);
	for (std::unique_ptr<Win32PropertySheetPage>& sheetPage : state->pages)
	{
		DestroyPropertySheetPage(sheetPage->handle);
	}
	SetWindowLongPtrA(sheet, DWLP_USER, 0);
	if (state->isOwnedByWindow)
	{
		delete state;
	}
}

INT_PTR CALLBACK propertySheetProc(HWND sheet, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_INITDIALOG)
	{
		return initializeSheet(sheet, reinterpret_cast<Win32PropertySheetState*>(lParam));
	}
	Win32PropertySheetState* state = sheetStateOf(sheet);
	if (state == nullptr)
	{
		return FALSE;
	}

	switch (message)
	{
	case WM_NCDESTROY:
		releaseSheet(sheet, state);
		return FALSE;
	case WM_COMMAND:
		return handleCommand(sheet, *state, LOWORD(wParam));
	case WM_NOTIFY:
		return handleTabNotify(sheet, *state, reinterpret_cast<const NMHDR*>(lParam));
	default:
		break;
	}
	if (message < PSM_SETCURSEL || message > PSM_GETCURRENTPAGEHWND)
	{
		return FALSE;
	}
	return reply(sheet, handleSheetMessage(sheet, *state, message, wParam, lParam));
}

std::unique_ptr<Win32PropertySheetState> createSheetState(const PROPSHEETHEADERA& header)
{
	auto state = std::make_unique<Win32PropertySheetState>();
	state->header = header;
	for (UINT index = 0; index < header.nPages; ++index)
	{
		Win32PropertySheets::appendPage(*state, header.ppsp[index], nullptr);
	}
	return state;
}

HWND createSheet(const PROPSHEETHEADERA& header, HWND owner, Win32PropertySheetState* state)
{
	Win32DialogTemplate dialog = sheetTemplate(header);
	if ((header.dwFlags & PSH_USECALLBACK) != 0 && header.pfnCallback != nullptr)
	{
		header.pfnCallback(nullptr, PSCB_PRECREATE, reinterpret_cast<LPARAM>(&dialog));
	}
	return Win32Dialogs::createFromTemplate(header.hInstance, dialog, owner, propertySheetProc, reinterpret_cast<LPARAM>(state));
}

INT_PTR runModalSheet(const PROPSHEETHEADERA& header, Win32PropertySheetState* state)
{
	Win32Window* owner = Win32TopLevelOf(Win32FindWindow(header.hwndParent));
	const HWND ownerHandle = owner != nullptr ? owner->hwnd : nullptr;
	const bool disabledOwner = ownerHandle != nullptr && IsWindowEnabled(ownerHandle);
	if (disabledOwner)
	{
		EnableWindow(ownerHandle, FALSE);
	}

	const HWND sheet = createSheet(header, ownerHandle, state);
	if (sheet != nullptr)
	{
		ShowWindow(sheet, SW_SHOWNORMAL);
	}
	MSG message = {};
	while (sheet != nullptr && IsWindow(sheet) && state->activeIndex >= 0 && GetMessageA(&message, nullptr, 0, 0))
	{
		if (!SendMessageA(sheet, PSM_ISDIALOGMESSAGE, 0, reinterpret_cast<LPARAM>(&message)))
		{
			TranslateMessage(&message);
			DispatchMessageA(&message);
		}
	}
	if (message.message == WM_QUIT)
	{
		PostQuitMessage(static_cast<int>(message.wParam));
	}

	const INT_PTR result = sheet == nullptr ? -1 : (state->hasApplied ? 1 : 0);
	if (disabledOwner)
	{
		EnableWindow(ownerHandle, TRUE);
		SetActiveWindow(ownerHandle);
	}
	DestroyWindow(sheet);
	return result;
}

}

INT_PTR PropertySheetA(LPCPROPSHEETHEADERA header)
{
	if (header == nullptr || (header->dwFlags & PSH_PROPSHEETPAGE) == 0 || (header->ppsp == nullptr && header->nPages > 0))
	{
		return -1;
	}

	std::unique_ptr<Win32PropertySheetState> state = createSheetState(*header);
	if ((header->dwFlags & PSH_MODELESS) == 0)
	{
		return runModalSheet(*header, state.get());
	}

	const HWND sheet = createSheet(*header, header->hwndParent, state.get());
	if (sheet == nullptr)
	{
		return -1;
	}
	state.release()->isOwnedByWindow = true;
	return reinterpret_cast<INT_PTR>(sheet);
}

HPROPSHEETPAGE CreatePropertySheetPageA(LPCPROPSHEETPAGEA page)
{
	if (page == nullptr)
	{
		return nullptr;
	}
	HPROPSHEETPAGE handle = new _PSP;
	handle->page = *page;
	return handle;
}

BOOL DestroyPropertySheetPage(HPROPSHEETPAGE page)
{
	if (page == nullptr)
	{
		return FALSE;
	}
	delete page;
	return TRUE;
}
