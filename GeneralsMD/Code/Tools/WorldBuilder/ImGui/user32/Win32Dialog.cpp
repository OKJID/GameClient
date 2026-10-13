#include "Win32Dialog.h"

#include "Lib/BaseType.h"
#include "Common/Debug.h"

#include "Win32Class.h"
#include "Win32Metrics.h"
#include "Win32Resources.h"

#include <algorithm>
#include <cstdlib>

namespace
{

const char kDialogClass[] = WC_DIALOG_NAME;

Win32WindowManager& manager()
{
	return Win32WindowManager::instance();
}

Win32DialogState& ensureState(Win32Window* window)
{
	if (window->dialogState == nullptr)
	{
		window->dialogState = std::make_unique<Win32DialogState>();
	}
	return *window->dialogState;
}

bool isNavigable(const Win32Window* control)
{
	return control->isVisible() && control->isEnabled();
}

bool isTabStop(const Win32Window* control)
{
	return isNavigable(control) && (control->style & WS_TABSTOP) != 0;
}

bool isControlParent(const Win32Window* control)
{
	return (control->exStyle & WS_EX_CONTROLPARENT) != 0 && control->isVisible();
}

void collectTabOrder(Win32Window* container, std::vector<Win32Window*>& order)
{
	for (Win32Window* child : container->children)
	{
		if (isControlParent(child))
		{
			collectTabOrder(child, order);
			continue;
		}
		order.push_back(child);
	}
}

HWND firstTabStop(Win32Window* dialog)
{
	std::vector<Win32Window*> order;
	collectTabOrder(dialog, order);
	for (Win32Window* control : order)
	{
		if (isTabStop(control))
		{
			return control->hwnd;
		}
	}
	return nullptr;
}

HFONT createDialogFont(const Win32DialogTemplate& dialogTemplate)
{
	LOGFONTA logFont = {};
	logFont.lfHeight = -Win32Metrics::dialogBaseUnits().cy;
	logFont.lfWeight = FW_NORMAL;
	strlcpy(logFont.lfFaceName, dialogTemplate.fontName.c_str(), sizeof(logFont.lfFaceName));
	return CreateFontIndirectA(&logFont);
}

RECT nonClientInsets(DWORD style)
{
	RECT insets = {};
	if ((style & WS_CHILD) != 0)
	{
		return insets;
	}

	const int border = (style & (WS_BORDER | WS_DLGFRAME | WS_THICKFRAME)) != 0 ? Win32Metrics::frameBorder() : 0;
	const int caption = (style & WS_CAPTION) == WS_CAPTION ? Win32Metrics::captionHeight() : 0;
	SetRect(&insets, border, border + caption, border, border);
	return insets;
}

POINT dialogOrigin(const Win32DialogTemplate& dialogTemplate, HWND parent, SIZE windowSize)
{
	const RECT client = Win32Dialogs::templateRectInPixels(dialogTemplate.x, dialogTemplate.y, 0, 0);
	POINT origin = { client.left, client.top };
	if ((dialogTemplate.style & WS_CHILD) != 0)
	{
		return origin;
	}

	if ((dialogTemplate.style & DS_CENTER) != 0)
	{
		const SIZE host = manager().hostSize();
		origin.x = (host.cx - windowSize.cx) / 2;
		origin.y = (host.cy - windowSize.cy) / 2;
		return origin;
	}

	if (parent != nullptr && (dialogTemplate.style & DS_ABSALIGN) == 0)
	{
		ClientToScreen(parent, &origin);
	}
	return origin;
}

void createControls(HWND dialog, const Win32DialogTemplate& dialogTemplate, HINSTANCE instance, HFONT font)
{
	for (const Win32DialogItemTemplate& item : dialogTemplate.items)
	{
		const RECT rect = Win32Dialogs::templateRectInPixels(item.x, item.y, item.cx, item.cy);
		const LPCSTR name = item.textOrdinal != 0 ? MAKEINTRESOURCE(item.textOrdinal) : item.text.c_str();
		const HMENU id = reinterpret_cast<HMENU>(static_cast<UINT_PTR>(static_cast<WORD>(item.id)));
		const HWND control = CreateWindowExA(item.exStyle | WS_EX_NOPARENTNOTIFY, item.className.c_str(), name, item.style,
			rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, dialog, id, instance, nullptr);
		if (control == nullptr)
		{
			DEBUG_LOG(("Dialog control of class '%s' could not be created", item.className.c_str()));
			continue;
		}
		SendMessageA(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
	}
}

LRESULT callDialogProc(Win32Window* window, Win32DialogState& state, UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = false;
	if (state.dialogProc == nullptr)
	{
		return 0;
	}

	const INT_PTR result = state.dialogProc(window->hwnd, message, wParam, lParam);
	if (result == 0)
	{
		return 0;
	}

	handled = true;
	const bool returnsValueDirectly = message == WM_INITDIALOG || (message >= WM_CTLCOLORMSGBOX && message <= WM_CTLCOLORSTATIC);
	return returnsValueDirectly ? result : GetWindowLongPtrA(window->hwnd, DWLP_MSGRESULT);
}

void restoreFocus(Win32Window* window, Win32DialogState& state)
{
	const HWND target = IsWindow(state.savedFocus) && IsChild(window->hwnd, state.savedFocus) ? state.savedFocus : firstTabStop(window);
	if (target != nullptr)
	{
		SetFocus(target);
	}
}

LRESULT clickDefaultButton(HWND dialog, UINT id)
{
	const HWND button = GetDlgItem(dialog, static_cast<int>(id));
	if (button != nullptr && !IsWindowEnabled(button))
	{
		MessageBeep(0);
		return 0;
	}
	SendMessageA(dialog, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), reinterpret_cast<LPARAM>(button));
	return 0;
}

void focusDialogControl(HWND control)
{
	if (control == nullptr)
	{
		return;
	}
	SetFocus(control);
	if ((SendMessageA(control, WM_GETDLGCODE, 0, 0) & DLGC_HASSETSEL) != 0)
	{
		SendMessageA(control, EM_SETSEL, 0, -1);
	}
}

LRESULT moveToDialogControl(HWND dialog, WPARAM wParam, LPARAM lParam)
{
	const bool targetsHandle = LOWORD(lParam) != 0;
	const HWND target = targetsHandle ? reinterpret_cast<HWND>(wParam) : GetNextDlgTabItem(dialog, GetFocus(), wParam != 0 ? TRUE : FALSE);
	focusDialogControl(target);
	return 0;
}

LRESULT defaultIdOf(Win32Window* window, const Win32DialogState& state)
{
	if (state.defaultId != 0)
	{
		return MAKELONG(state.defaultId, DC_HASDEFID);
	}
	std::vector<Win32Window*> order;
	collectTabOrder(window, order);
	for (Win32Window* control : order)
	{
		if ((SendMessageA(control->hwnd, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON) != 0)
		{
			return MAKELONG(control->id, DC_HASDEFID);
		}
	}
	return 0;
}

LRESULT setDefaultId(Win32Window* window, Win32DialogState& state, UINT id)
{
	const HWND previous = GetDlgItem(window->hwnd, static_cast<int>(LOWORD(defaultIdOf(window, state))));
	if (previous != nullptr && (SendMessageA(previous, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON) != 0)
	{
		SendMessageA(previous, BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
	}
	state.defaultId = id;
	const HWND next = GetDlgItem(window->hwnd, static_cast<int>(id));
	if (next != nullptr && (SendMessageA(next, WM_GETDLGCODE, 0, 0) & DLGC_UNDEFPUSHBUTTON) != 0)
	{
		SendMessageA(next, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
	}
	return TRUE;
}

LRESULT defaultDialogMessage(Win32Window* window, Win32DialogState& state, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_CLOSE:
		return clickDefaultButton(window->hwnd, IDCANCEL);
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			state.savedFocus = GetFocus();
			return 0;
		}
		restoreFocus(window, state);
		return 0;
	case WM_SETFOCUS:
		restoreFocus(window, state);
		return 0;
	case DM_GETDEFID:
		return defaultIdOf(window, state);
	case DM_SETDEFID:
		return setDefaultId(window, state, static_cast<UINT>(wParam));
	case WM_NEXTDLGCTL:
		return moveToDialogControl(window->hwnd, wParam, lParam);
	case WM_ERASEBKGND:
	{
		RECT client;
		GetClientRect(window->hwnd, &client);
		FillRect(reinterpret_cast<HDC>(wParam), &client, GetSysColorBrush(COLOR_BTNFACE));
		return 1;
	}
	default:
		return DefWindowProcA(window->hwnd, message, wParam, lParam);
	}
}

HWND nextInTabOrder(Win32Window* dialog, HWND current, bool previous, bool (*accepts)(const Win32Window*))
{
	std::vector<Win32Window*> order;
	collectTabOrder(dialog, order);
	if (order.empty())
	{
		return nullptr;
	}
	if (previous)
	{
		std::reverse(order.begin(), order.end());
	}

	const auto start = std::find_if(order.begin(), order.end(), [&](Win32Window* control) { return control->hwnd == current; });
	const size_t startIndex = start == order.end() ? order.size() - 1 : static_cast<size_t>(start - order.begin());
	for (size_t step = 1; step <= order.size(); ++step)
	{
		Win32Window* candidate = order[(startIndex + step) % order.size()];
		if (accepts(candidate))
		{
			return candidate->hwnd;
		}
	}
	return nullptr;
}

std::vector<Win32Window*> groupOf(Win32Window* dialog, Win32Window* control)
{
	std::vector<Win32Window*> order;
	collectTabOrder(dialog, order);
	const auto position = std::find(order.begin(), order.end(), control);
	if (position == order.end())
	{
		return {};
	}

	auto groupStart = position;
	while (groupStart != order.begin() && ((*groupStart)->style & WS_GROUP) == 0)
	{
		--groupStart;
	}
	auto groupEnd = position + 1;
	while (groupEnd != order.end() && ((*groupEnd)->style & WS_GROUP) == 0)
	{
		++groupEnd;
	}
	return std::vector<Win32Window*>(groupStart, groupEnd);
}

bool handleNavigationKey(HWND dialog, const MSG* message, LRESULT dialogCode)
{
	const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	switch (message->wParam)
	{
	case VK_TAB:
		if ((dialogCode & (DLGC_WANTTAB | DLGC_WANTALLKEYS)) != 0)
		{
			return false;
		}
		focusDialogControl(GetNextDlgTabItem(dialog, message->hwnd, shift ? TRUE : FALSE));
		return true;
	case VK_LEFT:
	case VK_RIGHT:
	case VK_UP:
	case VK_DOWN:
	{
		if ((dialogCode & (DLGC_WANTARROWS | DLGC_WANTALLKEYS)) != 0)
		{
			return false;
		}
		const bool backwards = message->wParam == VK_LEFT || message->wParam == VK_UP;
		const HWND next = GetNextDlgGroupItem(dialog, message->hwnd, backwards ? TRUE : FALSE);
		SetFocus(next);
		if ((SendMessageA(next, WM_GETDLGCODE, 0, 0) & DLGC_RADIOBUTTON) != 0)
		{
			SendMessageA(next, BM_CLICK, 0, 0);
		}
		return true;
	}
	default:
		return false;
	}
}

bool handleCommandKey(HWND dialog, const MSG* message, LRESULT dialogCode)
{
	if (message->wParam == VK_ESCAPE)
	{
		clickDefaultButton(dialog, IDCANCEL);
		return true;
	}
	if (message->wParam != VK_RETURN || (dialogCode & DLGC_WANTALLKEYS) != 0)
	{
		return false;
	}

	const bool focusIsPushButton = (dialogCode & (DLGC_DEFPUSHBUTTON | DLGC_UNDEFPUSHBUTTON)) != 0;
	const LRESULT defaultId = SendMessageA(dialog, DM_GETDEFID, 0, 0);
	const UINT id = focusIsPushButton ? static_cast<UINT>(GetDlgCtrlID(message->hwnd)) : (HIWORD(defaultId) == DC_HASDEFID ? LOWORD(defaultId) : IDOK);
	clickDefaultButton(dialog, id);
	return true;
}

}

namespace Win32Dialogs
{

void registerDialogClass()
{
	if (Win32ClassRegistry::instance().find(kDialogClass) != nullptr)
	{
		return;
	}

	WNDCLASSA dialogClass = {};
	dialogClass.style = CS_SAVEBITS | CS_DBLCLKS | CS_GLOBALCLASS;
	dialogClass.lpfnWndProc = DefDlgProcA;
	dialogClass.cbWndExtra = static_cast<int>(sizeof(LONG_PTR) * 3);
	dialogClass.hCursor = LoadCursorA(nullptr, IDC_ARROW);
	dialogClass.lpszClassName = kDialogClass;
	RegisterClassA(&dialogClass);
}

Win32DialogState* stateOf(HWND dialog)
{
	Win32Window* window = Win32FindWindow(dialog);
	return window != nullptr ? window->dialogState.get() : nullptr;
}

RECT templateRectInPixels(int x, int y, int cx, int cy)
{
	RECT rect;
	rect.left = Win32Metrics::dialogUnitsToPixelsX(x);
	rect.top = Win32Metrics::dialogUnitsToPixelsY(y);
	rect.right = rect.left + Win32Metrics::dialogUnitsToPixelsX(cx);
	rect.bottom = rect.top + Win32Metrics::dialogUnitsToPixelsY(cy);
	return rect;
}

HWND createFromTemplate(HINSTANCE instance, const Win32DialogTemplate& dialogTemplate, HWND parent, DLGPROC dialogProc, LPARAM initParam)
{
	registerDialogClass();
	const RECT client = templateRectInPixels(0, 0, dialogTemplate.cx, dialogTemplate.cy);
	const RECT insets = nonClientInsets(dialogTemplate.style);
	const SIZE windowSize = { client.right + insets.left + insets.right, client.bottom + insets.top + insets.bottom };
	const POINT origin = dialogOrigin(dialogTemplate, parent, windowSize);
	const char* className = dialogTemplate.className.empty() ? kDialogClass : dialogTemplate.className.c_str();
	const HMENU menu = dialogTemplate.menuName.empty() ? nullptr : LoadMenuA(instance, dialogTemplate.menuName.c_str());

	const HWND dialog = CreateWindowExA(dialogTemplate.exStyle, className, dialogTemplate.caption.c_str(), dialogTemplate.style & ~WS_VISIBLE,
		origin.x, origin.y, windowSize.cx, windowSize.cy, parent, menu, instance, nullptr);
	Win32Window* window = Win32FindWindow(dialog);
	if (window == nullptr)
	{
		return nullptr;
	}

	Win32DialogState& state = ensureState(window);
	state.dialogProc = dialogProc;
	state.font = createDialogFont(dialogTemplate);
	SetWindowLongPtrA(dialog, DWLP_DLGPROC, reinterpret_cast<LONG_PTR>(dialogProc));
	SendMessageA(dialog, WM_SETFONT, reinterpret_cast<WPARAM>(state.font), FALSE);
	createControls(dialog, dialogTemplate, instance, state.font);

	const HWND focusCandidate = firstTabStop(window);
	if (SendMessageA(dialog, WM_INITDIALOG, reinterpret_cast<WPARAM>(focusCandidate), initParam) && IsWindow(focusCandidate))
	{
		SetFocus(focusCandidate);
	}
	if ((dialogTemplate.style & WS_VISIBLE) != 0 && IsWindow(dialog))
	{
		ShowWindow(dialog, SW_SHOWNORMAL);
	}
	return IsWindow(dialog) ? dialog : nullptr;
}

}

LRESULT DefDlgProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return 0;
	}

	Win32DialogState& state = ensureState(window);
	bool handled = false;
	const LRESULT result = callDialogProc(window, state, message, wParam, lParam, handled);
	if (handled || !IsWindow(hwnd))
	{
		return result;
	}
	if (message == WM_NCDESTROY && state.font != nullptr)
	{
		DeleteObject(state.font);
		state.font = nullptr;
	}
	return defaultDialogMessage(window, state, message, wParam, lParam);
}

HWND CreateDialogParamA(HINSTANCE instance, LPCSTR templateName, HWND parent, DLGPROC dialogProc, LPARAM initParam)
{
	UINT id = 0;
	if (!Win32Resources::instance().resolveName(templateName, id))
	{
		return nullptr;
	}
	const Win32DialogTemplate* dialogTemplate = Win32Resources::instance().dialog(id);
	return dialogTemplate != nullptr ? Win32Dialogs::createFromTemplate(instance, *dialogTemplate, parent, dialogProc, initParam) : nullptr;
}

INT_PTR DialogBoxParamA(HINSTANCE instance, LPCSTR templateName, HWND parent, DLGPROC dialogProc, LPARAM initParam)
{
	UINT id = 0;
	const Win32DialogTemplate* dialogTemplate = Win32Resources::instance().resolveName(templateName, id) ? Win32Resources::instance().dialog(id) : nullptr;
	return dialogTemplate != nullptr ? Win32Dialogs::runModal(instance, *dialogTemplate, parent, dialogProc, initParam) : -1;
}

INT_PTR Win32Dialogs::runModal(HINSTANCE instance, const Win32DialogTemplate& dialogTemplate, HWND parent, DLGPROC dialogProc, LPARAM initParam)
{
	Win32Window* owner = Win32TopLevelOf(Win32FindWindow(parent));
	const HWND ownerHandle = owner != nullptr ? owner->hwnd : nullptr;
	const bool disabledOwner = ownerHandle != nullptr && IsWindowEnabled(ownerHandle);
	if (disabledOwner)
	{
		EnableWindow(ownerHandle, FALSE);
	}

	const HWND dialog = createFromTemplate(instance, dialogTemplate, ownerHandle, dialogProc, initParam);
	Win32DialogState* state = stateOf(dialog);
	if (state != nullptr && !state->hasEnded)
	{
		ShowWindow(dialog, SW_SHOWNORMAL);
	}

	MSG message;
	while (state != nullptr && IsWindow(dialog) && !state->hasEnded && GetMessageA(&message, nullptr, 0, 0))
	{
		if (!IsDialogMessageA(dialog, &message))
		{
			TranslateMessage(&message);
			DispatchMessageA(&message);
		}
	}
	if (message.message == WM_QUIT)
	{
		PostQuitMessage(static_cast<int>(message.wParam));
	}

	const INT_PTR result = state != nullptr ? state->result : -1;
	if (disabledOwner)
	{
		EnableWindow(ownerHandle, TRUE);
		SetActiveWindow(ownerHandle);
	}
	DestroyWindow(dialog);
	return result;
}

BOOL EndDialog(HWND dialog, INT_PTR result)
{
	Win32DialogState* state = Win32Dialogs::stateOf(dialog);
	if (state == nullptr)
	{
		return FALSE;
	}
	state->result = result;
	state->hasEnded = true;
	ShowWindow(dialog, SW_HIDE);
	return TRUE;
}

BOOL IsDialogMessageA(HWND dialog, LPMSG message)
{
	if (message->hwnd != dialog && !IsChild(dialog, message->hwnd))
	{
		return FALSE;
	}

	if (message->message == WM_KEYDOWN)
	{
		const LRESULT dialogCode = SendMessageA(message->hwnd, WM_GETDLGCODE, message->wParam, reinterpret_cast<LPARAM>(message));
		if (handleNavigationKey(dialog, message, dialogCode) || handleCommandKey(dialog, message, dialogCode))
		{
			return TRUE;
		}
	}

	TranslateMessage(message);
	DispatchMessageA(message);
	return TRUE;
}

BOOL MapDialogRect(HWND, LPRECT rect)
{
	if (rect == nullptr)
	{
		return FALSE;
	}
	*rect = Win32Dialogs::templateRectInPixels(rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top);
	return TRUE;
}

HWND GetDlgItem(HWND dialog, int id)
{
	Win32Window* window = Win32FindWindow(dialog);
	if (window == nullptr)
	{
		return nullptr;
	}
	for (Win32Window* child : window->children)
	{
		if (static_cast<int>(child->id) == id || static_cast<WORD>(child->id) == static_cast<WORD>(id))
		{
			return child->hwnd;
		}
	}
	return nullptr;
}

int GetDlgCtrlID(HWND hwnd)
{
	const Win32Window* window = Win32FindWindow(hwnd);
	return window != nullptr && window->isChild() ? static_cast<int>(window->id) : 0;
}

UINT GetDlgItemTextA(HWND dialog, int id, LPSTR text, int maxCount)
{
	if (text != nullptr && maxCount > 0)
	{
		text[0] = '\0';
	}
	return static_cast<UINT>(GetWindowTextA(GetDlgItem(dialog, id), text, maxCount));
}

BOOL SetDlgItemTextA(HWND dialog, int id, LPCSTR text)
{
	return SetWindowTextA(GetDlgItem(dialog, id), text);
}

UINT GetDlgItemInt(HWND dialog, int id, BOOL* translated, BOOL isSigned)
{
	char buffer[64] = {};
	GetDlgItemTextA(dialog, id, buffer, sizeof(buffer));
	char* end = nullptr;
	const long value = isSigned ? std::strtol(buffer, &end, 10) : static_cast<long>(std::strtoul(buffer, &end, 10));
	const bool isValid = end != buffer && *end == '\0';
	if (translated != nullptr)
	{
		*translated = isValid ? TRUE : FALSE;
	}
	return isValid ? static_cast<UINT>(value) : 0;
}

BOOL SetDlgItemInt(HWND dialog, int id, UINT value, BOOL isSigned)
{
	char buffer[32];
	snprintf(buffer, sizeof(buffer), isSigned ? "%d" : "%u", isSigned ? static_cast<int>(value) : value);
	return SetDlgItemTextA(dialog, id, buffer);
}

BOOL CheckDlgButton(HWND dialog, int id, UINT check)
{
	const HWND button = GetDlgItem(dialog, id);
	if (button == nullptr)
	{
		return FALSE;
	}
	SendMessageA(button, BM_SETCHECK, check, 0);
	return TRUE;
}

UINT IsDlgButtonChecked(HWND dialog, int id)
{
	return static_cast<UINT>(SendMessageA(GetDlgItem(dialog, id), BM_GETCHECK, 0, 0));
}

BOOL CheckRadioButton(HWND dialog, int firstId, int lastId, int checkId)
{
	for (int id = firstId; id <= lastId; ++id)
	{
		CheckDlgButton(dialog, id, id == checkId ? BST_CHECKED : BST_UNCHECKED);
	}
	return TRUE;
}

LRESULT SendDlgItemMessageA(HWND dialog, int id, UINT message, WPARAM wParam, LPARAM lParam)
{
	return SendMessageA(GetDlgItem(dialog, id), message, wParam, lParam);
}

HWND GetNextDlgTabItem(HWND dialog, HWND control, BOOL previous)
{
	Win32Window* window = Win32FindWindow(dialog);
	return window != nullptr ? nextInTabOrder(window, control, previous != FALSE, isTabStop) : nullptr;
}

HWND GetNextDlgGroupItem(HWND dialog, HWND control, BOOL previous)
{
	Win32Window* window = Win32FindWindow(dialog);
	Win32Window* current = Win32FindWindow(control);
	if (window == nullptr || current == nullptr)
	{
		return control;
	}

	std::vector<Win32Window*> group = groupOf(window, current);
	if (previous)
	{
		std::reverse(group.begin(), group.end());
	}
	const auto position = std::find(group.begin(), group.end(), current);
	for (size_t step = 1; step <= group.size(); ++step)
	{
		Win32Window* candidate = group[(static_cast<size_t>(position - group.begin()) + step) % group.size()];
		if (isNavigable(candidate))
		{
			return candidate->hwnd;
		}
	}
	return control;
}
