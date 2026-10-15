#include "Win32Class.h"
#include "Win32Messages.h"
#include "Win32Window.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <vector>

namespace
{

Win32WindowManager& manager()
{
	return Win32WindowManager::instance();
}

bool isInside(const Win32Window* window, const Win32Window* ancestor)
{
	for (const Win32Window* current = window; current != nullptr; current = current->parent)
	{
		if (current == ancestor)
		{
			return true;
		}
	}
	return false;
}

void invalidateAfterGeometryChange(Win32Window* window)
{
	Win32InvalidateTree(window, true);
	if (window->parent != nullptr)
	{
		Win32InvalidateTree(window->parent, true);
	}
}

void hideWindow(Win32Window* window)
{
	SendMessageA(window->hwnd, WM_SHOWWINDOW, FALSE, 0);
	window->style &= ~WS_VISIBLE;
	invalidateAfterGeometryChange(window);

	Win32Window* focus = manager().focus();
	if (focus != nullptr && isInside(focus, window))
	{
		SetFocus(window->parent != nullptr ? window->parent->hwnd : nullptr);
	}
	if (manager().active() == window)
	{
		SetActiveWindow(window->owner != nullptr ? window->owner->hwnd : nullptr);
	}
}

void showWindow(Win32Window* window, int command)
{
	if (!window->isVisible())
	{
		SendMessageA(window->hwnd, WM_SHOWWINDOW, TRUE, 0);
		window->style |= WS_VISIBLE;
		invalidateAfterGeometryChange(window);
	}

	const bool activates = command != SW_SHOWNA && command != SW_SHOWNOACTIVATE && command != SW_SHOWMINNOACTIVE;
	if (!window->isChild() && activates)
	{
		SetActiveWindow(window->hwnd);
	}
}

void maximizeWindow(Win32Window* window)
{
	const SIZE host = manager().hostSize();
	window->style |= WS_MAXIMIZE;
	window->style &= ~WS_MINIMIZE;
	SetWindowPos(window->hwnd, nullptr, 0, 0, host.cx, host.cy, SWP_NOZORDER | SWP_NOACTIVATE);
}

void applyZOrder(Win32Window* window, HWND insertAfter)
{
	if (insertAfter == HWND_TOPMOST)
	{
		window->exStyle |= WS_EX_TOPMOST;
		manager().bringToTop(window);
		return;
	}
	if (insertAfter == HWND_NOTOPMOST)
	{
		window->exStyle &= ~WS_EX_TOPMOST;
		return;
	}
	if (insertAfter == HWND_TOP)
	{
		manager().bringToTop(window);
		return;
	}
	if (insertAfter == HWND_BOTTOM)
	{
		manager().placeAtBottom(window);
		return;
	}

	Win32Window* after = Win32FindWindow(insertAfter);
	if (after != nullptr && after->parent == window->parent)
	{
		manager().placeBehind(window, after);
	}
}

void applyWindowPos(Win32Window* window, WINDOWPOS& position)
{
	RECT rect = window->rect;
	if ((position.flags & SWP_NOMOVE) == 0)
	{
		OffsetRect(&rect, position.x - rect.left, position.y - rect.top);
	}
	if ((position.flags & SWP_NOSIZE) == 0)
	{
		rect.right = rect.left + std::max(0, position.cx);
		rect.bottom = rect.top + std::max(0, position.cy);
	}

	const bool moved = rect.left != window->rect.left || rect.top != window->rect.top;
	const bool sized = (rect.right - rect.left) != window->width() || (rect.bottom - rect.top) != window->height();
	window->rect = rect;
	position.flags |= moved ? 0 : SWP_NOMOVE;
	position.flags |= sized ? 0 : SWP_NOSIZE;
	position.x = rect.left;
	position.y = rect.top;
	position.cx = rect.right - rect.left;
	position.cy = rect.bottom - rect.top;
}

RECT clientRectInParent(const Win32Window* window)
{
	RECT client = manager().clientRectInWindow(window);
	OffsetRect(&client, window->rect.left, window->rect.top);
	return client;
}

void markClientChanges(WINDOWPOS& position, const RECT& before, const RECT& after)
{
	if (before.left == after.left && before.top == after.top)
	{
		position.flags |= SWP_NOCLIENTMOVE;
	}
	if (before.right - before.left == after.right - after.left && before.bottom - before.top == after.bottom - after.top)
	{
		position.flags |= SWP_NOCLIENTSIZE;
	}
}

void applyVisibilityFlags(Win32Window* window, UINT flags)
{
	if ((flags & SWP_SHOWWINDOW) != 0)
	{
		window->style |= WS_VISIBLE;
	}
	if ((flags & SWP_HIDEWINDOW) != 0)
	{
		window->style &= ~WS_VISIBLE;
	}
}

POINT clientOrigin(HWND hwnd)
{
	const Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return POINT();
	}
	return manager().clientOriginOnScreen(window);
}

Win32Window* deepestChildAt(Win32Window* window, POINT screenPoint)
{
	const POINT origin = manager().clientOriginOnScreen(window);
	for (Win32Window* child : window->children)
	{
		if (!child->isVisible())
		{
			continue;
		}
		RECT childRect = child->rect;
		OffsetRect(&childRect, origin.x, origin.y);
		if (PtInRect(&childRect, screenPoint))
		{
			return deepestChildAt(child, screenPoint);
		}
	}
	return window;
}

int copyText(const std::string& text, LPSTR buffer, int maxCount)
{
	if (buffer == nullptr || maxCount <= 0)
	{
		return 0;
	}
	strlcpy(buffer, text.c_str(), static_cast<size_t>(maxCount));
	return static_cast<int>(strlen(buffer));
}

}

BOOL ShowWindow(HWND hwnd, int command)
{
	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return FALSE;
	}

	const BOOL wasVisible = window->isVisible() ? TRUE : FALSE;
	switch (command)
	{
	case SW_HIDE:
		if (wasVisible)
		{
			hideWindow(window);
		}
		return wasVisible;
	case SW_SHOWMAXIMIZED:
		maximizeWindow(window);
		showWindow(window, command);
		return wasVisible;
	case SW_MINIMIZE:
	case SW_SHOWMINIMIZED:
	case SW_SHOWMINNOACTIVE:
		window->style |= WS_MINIMIZE;
		showWindow(window, command);
		return wasVisible;
	case SW_RESTORE:
		window->style &= ~(WS_MINIMIZE | WS_MAXIMIZE);
		showWindow(window, command);
		return wasVisible;
	default:
		showWindow(window, command);
		return wasVisible;
	}
}

BOOL ShowOwnedPopups(HWND ownerHandle, BOOL show)
{
	const Win32Window* owner = Win32FindWindow(ownerHandle);
	if (owner == nullptr)
	{
		return FALSE;
	}

	std::vector<HWND> owned;
	for (const Win32Window* window : manager().topLevelWindows())
	{
		if (window->owner == owner)
		{
			owned.push_back(window->hwnd);
		}
	}
	for (HWND hwnd : owned)
	{
		Win32Window* window = Win32FindWindow(hwnd);
		if (window == nullptr)
		{
			continue;
		}
		if (show && window->isHiddenByOwner)
		{
			window->isHiddenByOwner = false;
			SendMessageA(hwnd, WM_SHOWWINDOW, TRUE, SW_PARENTOPENING);
		}
		else if (!show && window->isVisible())
		{
			window->isHiddenByOwner = true;
			SendMessageA(hwnd, WM_SHOWWINDOW, FALSE, SW_PARENTCLOSING);
		}
	}
	return TRUE;
}

BOOL IsWindowVisible(HWND hwnd)
{
	for (const Win32Window* window = Win32FindWindow(hwnd); window != nullptr; window = window->isChild() ? window->parent : nullptr)
	{
		if (!window->isVisible())
		{
			return FALSE;
		}
		if (!window->isChild())
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL EnableWindow(HWND hwnd, BOOL enable)
{
	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return FALSE;
	}

	const BOOL wasDisabled = window->isEnabled() ? FALSE : TRUE;
	const bool changes = (enable != FALSE) == (wasDisabled != FALSE);
	if (!changes)
	{
		return wasDisabled;
	}

	if (enable)
	{
		window->style &= ~WS_DISABLED;
	}
	else
	{
		window->style |= WS_DISABLED;
		if (manager().focus() == window)
		{
			SetFocus(nullptr);
		}
		if (manager().capture() == window)
		{
			ReleaseCapture();
		}
	}
	SendMessageA(hwnd, WM_ENABLE, enable, 0);
	Win32InvalidateTree(window, true);
	return wasDisabled;
}

BOOL IsWindowEnabled(HWND hwnd)
{
	const Win32Window* window = Win32FindWindow(hwnd);
	return window != nullptr && window->isEnabled() ? TRUE : FALSE;
}

BOOL IsIconic(HWND hwnd)
{
	const Win32Window* window = Win32FindWindow(hwnd);
	return window != nullptr && (window->style & WS_MINIMIZE) != 0 ? TRUE : FALSE;
}

BOOL IsZoomed(HWND hwnd)
{
	const Win32Window* window = Win32FindWindow(hwnd);
	return window != nullptr && (window->style & WS_MAXIMIZE) != 0 ? TRUE : FALSE;
}

BOOL SetWindowPos(HWND hwnd, HWND insertAfter, int x, int y, int width, int height, UINT flags)
{
	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr || manager().isDesktop(window))
	{
		return FALSE;
	}

	WINDOWPOS position = { hwnd, insertAfter, x, y, width, height, flags & ~(SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE) };
	if ((flags & SWP_NOSENDCHANGING) == 0)
	{
		SendMessageA(hwnd, WM_WINDOWPOSCHANGING, 0, reinterpret_cast<LPARAM>(&position));
	}

	const RECT clientBefore = clientRectInParent(window);
	applyWindowPos(window, position);
	if ((position.flags & SWP_NOSIZE) == 0 || (flags & SWP_FRAMECHANGED) != 0)
	{
		Win32RecalculateClientRect(window);
	}
	markClientChanges(position, clientBefore, clientRectInParent(window));
	if ((position.flags & SWP_NOZORDER) == 0)
	{
		applyZOrder(window, position.hwndInsertAfter);
	}
	applyVisibilityFlags(window, position.flags);
	if ((position.flags & SWP_NOREDRAW) == 0)
	{
		invalidateAfterGeometryChange(window);
	}

	SendMessageA(hwnd, WM_WINDOWPOSCHANGED, 0, reinterpret_cast<LPARAM>(&position));
	if (!window->isChild() && (flags & (SWP_NOACTIVATE | SWP_HIDEWINDOW)) == 0 && (flags & SWP_SHOWWINDOW) != 0)
	{
		SetActiveWindow(hwnd);
	}
	return TRUE;
}

BOOL MoveWindow(HWND hwnd, int x, int y, int width, int height, BOOL repaint)
{
	const UINT redraw = repaint ? 0 : SWP_NOREDRAW;
	return SetWindowPos(hwnd, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE | redraw);
}

HDWP BeginDeferWindowPos(int numWindows)
{
	auto* positions = new std::vector<WINDOWPOS>();
	positions->reserve(static_cast<size_t>(std::max(0, numWindows)));
	return positions;
}

HDWP DeferWindowPos(HDWP positions, HWND hwnd, HWND insertAfter, int x, int y, int width, int height, UINT flags)
{
	auto* deferred = static_cast<std::vector<WINDOWPOS>*>(positions);
	if (deferred == nullptr)
	{
		return nullptr;
	}
	if (Win32FindWindow(hwnd) == nullptr)
	{
		delete deferred;
		return nullptr;
	}
	deferred->push_back({ hwnd, insertAfter, x, y, width, height, flags });
	return deferred;
}

BOOL EndDeferWindowPos(HDWP positions)
{
	std::unique_ptr<std::vector<WINDOWPOS>> deferred(static_cast<std::vector<WINDOWPOS>*>(positions));
	if (deferred == nullptr)
	{
		return FALSE;
	}
	for (const WINDOWPOS& position : *deferred)
	{
		SetWindowPos(position.hwnd, position.hwndInsertAfter, position.x, position.y, position.cx, position.cy, position.flags);
	}
	return TRUE;
}

BOOL BringWindowToTop(HWND hwnd)
{
	return SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

BOOL GetWindowRect(HWND hwnd, LPRECT rect)
{
	const Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr || rect == nullptr)
	{
		return FALSE;
	}
	*rect = manager().windowRectOnScreen(window);
	return TRUE;
}

BOOL AdjustWindowRect(LPRECT rect, DWORD style, BOOL menu)
{
	return AdjustWindowRectEx(rect, style, menu, 0);
}

BOOL AdjustWindowRectEx(LPRECT rect, DWORD style, BOOL, DWORD exStyle)
{
	if (rect == nullptr)
	{
		return FALSE;
	}
	const Win32NonClientMetrics metrics = Win32NonClientMetricsFor(style, exStyle, false);
	rect->left -= metrics.borderWidth;
	rect->right += metrics.borderWidth;
	rect->top -= metrics.borderWidth + metrics.captionHeight + metrics.menuHeight;
	rect->bottom += metrics.borderWidth;
	return TRUE;
}

BOOL GetClientRect(HWND hwnd, LPRECT rect)
{
	const Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr || rect == nullptr)
	{
		return FALSE;
	}

	const RECT client = manager().clientRectInWindow(window);
	SetRect(rect, 0, 0, client.right - client.left, client.bottom - client.top);
	return TRUE;
}

BOOL ClientToScreen(HWND hwnd, LPPOINT point)
{
	if (Win32FindWindow(hwnd) == nullptr || point == nullptr)
	{
		return FALSE;
	}

	const POINT origin = clientOrigin(hwnd);
	point->x += origin.x;
	point->y += origin.y;
	return TRUE;
}

BOOL ScreenToClient(HWND hwnd, LPPOINT point)
{
	if (Win32FindWindow(hwnd) == nullptr || point == nullptr)
	{
		return FALSE;
	}

	const POINT origin = clientOrigin(hwnd);
	point->x -= origin.x;
	point->y -= origin.y;
	return TRUE;
}

int MapWindowPoints(HWND from, HWND to, LPPOINT points, UINT count)
{
	const POINT fromOrigin = clientOrigin(from);
	const POINT toOrigin = clientOrigin(to);
	const int dx = fromOrigin.x - toOrigin.x;
	const int dy = fromOrigin.y - toOrigin.y;
	for (UINT index = 0; index < count; ++index)
	{
		points[index].x += dx;
		points[index].y += dy;
	}
	return MAKELONG(static_cast<WORD>(dx), static_cast<WORD>(dy));
}

HWND WindowFromPoint(POINT point)
{
	for (Win32Window* window : manager().topLevelWindows())
	{
		const RECT rect = manager().windowRectOnScreen(window);
		if (window->isVisible() && PtInRect(&rect, point))
		{
			return deepestChildAt(window, point)->hwnd;
		}
	}
	return nullptr;
}

HWND ChildWindowFromPoint(HWND parentHandle, POINT point)
{
	Win32Window* parent = Win32FindWindow(parentHandle);
	if (parent == nullptr)
	{
		return nullptr;
	}

	for (Win32Window* child : parent->children)
	{
		if (PtInRect(&child->rect, point))
		{
			return child->hwnd;
		}
	}

	RECT client;
	GetClientRect(parentHandle, &client);
	return PtInRect(&client, point) ? parentHandle : nullptr;
}

BOOL SetWindowTextA(HWND hwnd, LPCSTR text)
{
	return SendMessageA(hwnd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(text != nullptr ? text : "")) != 0 ? TRUE : FALSE;
}

int GetWindowTextA(HWND hwnd, LPSTR text, int maxCount)
{
	return static_cast<int>(SendMessageA(hwnd, WM_GETTEXT, static_cast<WPARAM>(maxCount), reinterpret_cast<LPARAM>(text)));
}

int GetWindowTextLengthA(HWND hwnd)
{
	return static_cast<int>(SendMessageA(hwnd, WM_GETTEXTLENGTH, 0, 0));
}

int Win32CopyWindowText(const std::string& text, LPSTR buffer, int maxCount)
{
	return copyText(text, buffer, maxCount);
}

LONG_PTR GetWindowLongPtrA(HWND hwnd, int index)
{
	const Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return 0;
	}

	switch (index)
	{
	case GWL_STYLE:
		return static_cast<LONG_PTR>(window->style);
	case GWL_EXSTYLE:
		return static_cast<LONG_PTR>(window->exStyle);
	case GWLP_ID:
		return static_cast<LONG_PTR>(window->id);
	case GWLP_WNDPROC:
		return reinterpret_cast<LONG_PTR>(window->windowProc);
	case GWLP_USERDATA:
		return window->userData;
	case GWLP_HINSTANCE:
		return reinterpret_cast<LONG_PTR>(window->instance);
	case GWLP_HWNDPARENT:
		return reinterpret_cast<LONG_PTR>(window->isChild() ? (window->parent != nullptr ? window->parent->hwnd : nullptr) : (window->owner != nullptr ? window->owner->hwnd : nullptr));
	default:
		break;
	}

	const size_t slot = static_cast<size_t>(index) / sizeof(LONG_PTR);
	return index >= 0 && slot < window->extraWords.size() ? window->extraWords[slot] : 0;
}

LONG_PTR SetWindowLongPtrA(HWND hwnd, int index, LONG_PTR value)
{
	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return 0;
	}

	const LONG_PTR previous = GetWindowLongPtrA(hwnd, index);
	switch (index)
	{
	case GWL_STYLE:
		window->style = static_cast<DWORD>(value);
		return previous;
	case GWL_EXSTYLE:
		window->exStyle = static_cast<DWORD>(value);
		return previous;
	case GWLP_ID:
		window->id = static_cast<UINT_PTR>(value);
		return previous;
	case GWLP_WNDPROC:
		window->windowProc = reinterpret_cast<WNDPROC>(value);
		return previous;
	case GWLP_USERDATA:
		window->userData = value;
		return previous;
	case GWLP_HINSTANCE:
		window->instance = reinterpret_cast<HINSTANCE>(value);
		return previous;
	default:
		break;
	}

	const size_t slot = static_cast<size_t>(index) / sizeof(LONG_PTR);
	if (index >= 0 && slot < window->extraWords.size())
	{
		window->extraWords[slot] = value;
	}
	return previous;
}

LONG GetWindowLongA(HWND hwnd, int index)
{
	return static_cast<LONG>(GetWindowLongPtrA(hwnd, index));
}

LONG SetWindowLongA(HWND hwnd, int index, LONG value)
{
	return static_cast<LONG>(SetWindowLongPtrA(hwnd, index, value));
}

BOOL SetRect(LPRECT rect, int left, int top, int right, int bottom)
{
	if (rect == nullptr)
	{
		return FALSE;
	}
	rect->left = left;
	rect->top = top;
	rect->right = right;
	rect->bottom = bottom;
	return TRUE;
}

BOOL SetRectEmpty(LPRECT rect)
{
	return SetRect(rect, 0, 0, 0, 0);
}

BOOL CopyRect(LPRECT destination, const RECT* source)
{
	if (destination == nullptr || source == nullptr)
	{
		return FALSE;
	}
	*destination = *source;
	return TRUE;
}

BOOL InflateRect(LPRECT rect, int dx, int dy)
{
	if (rect == nullptr)
	{
		return FALSE;
	}
	return SetRect(rect, rect->left - dx, rect->top - dy, rect->right + dx, rect->bottom + dy);
}

BOOL OffsetRect(LPRECT rect, int dx, int dy)
{
	if (rect == nullptr)
	{
		return FALSE;
	}
	return SetRect(rect, rect->left + dx, rect->top + dy, rect->right + dx, rect->bottom + dy);
}

BOOL IsRectEmpty(const RECT* rect)
{
	return rect == nullptr || rect->right <= rect->left || rect->bottom <= rect->top ? TRUE : FALSE;
}

BOOL IntersectRect(LPRECT destination, const RECT* first, const RECT* second)
{
	RECT result;
	result.left = std::max(first->left, second->left);
	result.top = std::max(first->top, second->top);
	result.right = std::min(first->right, second->right);
	result.bottom = std::min(first->bottom, second->bottom);
	if (IsRectEmpty(&result))
	{
		SetRectEmpty(destination);
		return FALSE;
	}
	*destination = result;
	return TRUE;
}

BOOL UnionRect(LPRECT destination, const RECT* first, const RECT* second)
{
	if (IsRectEmpty(first))
	{
		return CopyRect(destination, second) && !IsRectEmpty(second);
	}
	if (IsRectEmpty(second))
	{
		return CopyRect(destination, first);
	}
	return SetRect(destination, std::min(first->left, second->left), std::min(first->top, second->top),
		std::max(first->right, second->right), std::max(first->bottom, second->bottom));
}

BOOL EqualRect(const RECT* first, const RECT* second)
{
	return first->left == second->left && first->top == second->top && first->right == second->right && first->bottom == second->bottom ? TRUE : FALSE;
}

BOOL PtInRect(const RECT* rect, POINT point)
{
	return point.x >= rect->left && point.x < rect->right && point.y >= rect->top && point.y < rect->bottom ? TRUE : FALSE;
}
