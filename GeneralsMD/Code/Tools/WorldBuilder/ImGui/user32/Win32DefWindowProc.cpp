#include "Win32Class.h"
#include "Win32Messages.h"
#include "Win32Window.h"

namespace
{

LRESULT setText(Win32Window* window, LPARAM lParam)
{
	const char* text = reinterpret_cast<const char*>(lParam);
	window->text = text != nullptr ? text : "";
	InvalidateRect(window->hwnd, nullptr, TRUE);
	return TRUE;
}

LRESULT eraseBackground(Win32Window* window, WPARAM wParam)
{
	HBRUSH background = window->windowClass != nullptr ? window->windowClass->background : nullptr;
	if (background == nullptr)
	{
		return 0;
	}

	if (reinterpret_cast<uintptr_t>(background) <= COLOR_INFOBK + 1)
	{
		background = GetSysColorBrush(static_cast<int>(reinterpret_cast<uintptr_t>(background)) - 1);
	}
	RECT client;
	GetClientRect(window->hwnd, &client);
	FillRect(reinterpret_cast<HDC>(wParam), &client, background);
	return 1;
}

LRESULT paintAndValidate(HWND hwnd)
{
	PAINTSTRUCT paint;
	BeginPaint(hwnd, &paint);
	EndPaint(hwnd, &paint);
	return 0;
}

LRESULT sendGeometryNotifications(Win32Window* window, LPARAM lParam)
{
	const WINDOWPOS* position = reinterpret_cast<const WINDOWPOS*>(lParam);
	RECT client;
	GetClientRect(window->hwnd, &client);
	if ((position->flags & SWP_NOCLIENTSIZE) == 0)
	{
		const WPARAM sizeType = (window->style & WS_MAXIMIZE) != 0 ? SIZE_MAXIMIZED : SIZE_RESTORED;
		SendMessageA(window->hwnd, WM_SIZE, sizeType, MAKELPARAM(client.right, client.bottom));
	}
	if ((position->flags & SWP_NOCLIENTMOVE) == 0)
	{
		const RECT clientInWindow = Win32WindowManager::instance().clientRectInWindow(window);
		SendMessageA(window->hwnd, WM_MOVE, 0, MAKELPARAM(window->rect.left + clientInWindow.left, window->rect.top + clientInWindow.top));
	}
	return 0;
}

LRESULT calculateClientArea(Win32Window* window, LPARAM lParam)
{
	RECT* rect = reinterpret_cast<RECT*>(lParam);
	const Win32NonClientMetrics metrics = Win32WindowManager::instance().nonClientMetrics(window);
	rect->left += metrics.borderWidth;
	rect->top += metrics.borderWidth + metrics.captionHeight + metrics.menuHeight;
	rect->right -= metrics.borderWidth;
	rect->bottom -= metrics.borderWidth;
	return 0;
}

LRESULT paintFrame(Win32Window* window)
{
	if (!window->isChild() || Win32WindowManager::instance().nonClientMetrics(window).borderWidth <= 0)
	{
		return 0;
	}
	const HDC hdc = GetWindowDC(window->hwnd);
	RECT frame;
	SetRect(&frame, 0, 0, window->width(), window->height());
	const bool hasEdge = (window->exStyle & (WS_EX_CLIENTEDGE | WS_EX_STATICEDGE)) != 0;
	FrameRect(hdc, &frame, GetSysColorBrush(hasEdge ? COLOR_BTNSHADOW : COLOR_WINDOWFRAME));
	ReleaseDC(window->hwnd, hdc);
	return 0;
}

LRESULT showForOwner(Win32Window* window, WPARAM show, LPARAM status)
{
	if (status == 0 || window->owner == nullptr || window->isVisible() == (show != 0))
	{
		return 0;
	}
	ShowWindow(window->hwnd, show != 0 ? SW_SHOWNOACTIVATE : SW_HIDE);
	return 0;
}

LRESULT setCursor(Win32Window* window, WPARAM wParam, LPARAM lParam)
{
	if (window->isChild() && window->parent != nullptr && SendMessageA(window->parent->hwnd, WM_SETCURSOR, wParam, lParam))
	{
		return TRUE;
	}
	if (LOWORD(lParam) != HTCLIENT || window->windowClass == nullptr || window->windowClass->cursor == nullptr)
	{
		return FALSE;
	}
	SetCursor(window->windowClass->cursor);
	return TRUE;
}

LRESULT forwardToParent(Win32Window* window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (!window->isChild() || window->parent == nullptr)
	{
		return 0;
	}
	return SendMessageA(window->parent->hwnd, message, wParam, lParam);
}

LRESULT activate(Win32Window* window, WPARAM wParam)
{
	if (LOWORD(wParam) == WA_INACTIVE)
	{
		return 0;
	}

	const HWND focus = GetFocus();
	if (focus != window->hwnd && !IsChild(window->hwnd, focus))
	{
		SetFocus(window->hwnd);
	}
	return 0;
}

LRESULT mouseActivate(Win32Window* window, WPARAM wParam, LPARAM lParam)
{
	const LRESULT parentResult = forwardToParent(window, WM_MOUSEACTIVATE, wParam, lParam);
	return parentResult != 0 ? parentResult : MA_ACTIVATE;
}

LRESULT controlColor(UINT message)
{
	const bool usesWindowColor = message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX;
	return reinterpret_cast<LRESULT>(GetSysColorBrush(usesWindowColor ? COLOR_WINDOW : COLOR_BTNFACE));
}

LRESULT cancelMode(Win32Window* window)
{
	if (GetCapture() == window->hwnd)
	{
		ReleaseCapture();
	}
	return 0;
}

LRESULT handleTextMessage(Win32Window* window, UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = true;
	switch (message)
	{
	case WM_SETTEXT:
		return setText(window, lParam);
	case WM_GETTEXT:
		return Win32CopyWindowText(window->text, reinterpret_cast<LPSTR>(lParam), static_cast<int>(wParam));
	case WM_GETTEXTLENGTH:
		return static_cast<LRESULT>(window->text.size());
	case WM_SETFONT:
		window->font = reinterpret_cast<HFONT>(wParam);
		return 0;
	case WM_GETFONT:
		return reinterpret_cast<LRESULT>(window->font);
	default:
		handled = false;
		return 0;
	}
}

}

LRESULT DefWindowProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return 0;
	}

	bool handled = false;
	const LRESULT textResult = handleTextMessage(window, message, wParam, lParam, handled);
	if (handled)
	{
		return textResult;
	}
	if (message >= WM_CTLCOLORMSGBOX && message <= WM_CTLCOLORSTATIC)
	{
		return controlColor(message);
	}

	switch (message)
	{
	case WM_NCCREATE:
		return TRUE;
	case WM_PAINT:
		return paintAndValidate(hwnd);
	case WM_ERASEBKGND:
		return eraseBackground(window, wParam);
	case WM_CLOSE:
		DestroyWindow(hwnd);
		return 0;
	case WM_SYSCOMMAND:
		return (wParam & 0xFFF0) == SC_CLOSE ? SendMessageA(hwnd, WM_CLOSE, 0, 0) : 0;
	case WM_WINDOWPOSCHANGED:
		return sendGeometryNotifications(window, lParam);
	case WM_SHOWWINDOW:
		return showForOwner(window, wParam, lParam);
	case WM_NCCALCSIZE:
		return calculateClientArea(window, lParam);
	case WM_NCPAINT:
		return paintFrame(window);
	case WM_SETCURSOR:
		return setCursor(window, wParam, lParam);
	case WM_MOUSEACTIVATE:
		return mouseActivate(window, wParam, lParam);
	case WM_ACTIVATE:
		return activate(window, wParam);
	case WM_NCHITTEST:
		return HTCLIENT;
	case WM_CONTEXTMENU:
	case WM_MOUSEWHEEL:
		return forwardToParent(window, message, wParam, lParam);
	case WM_CANCELMODE:
		return cancelMode(window);
	default:
		return 0;
	}
}
