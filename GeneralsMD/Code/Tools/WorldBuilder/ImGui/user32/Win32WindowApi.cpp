#include "Win32Class.h"
#include "Win32Control.h"
#include "Win32Gdi.h"
#include "Win32Messages.h"
#include "Win32Window.h"

#include <algorithm>

namespace
{

const int kDefaultTopLevelOffset = 40;

Win32WindowManager& manager()
{
	return Win32WindowManager::instance();
}

bool resolveRelatives(DWORD style, HWND parentHandle, Win32Window*& parent, Win32Window*& owner)
{
	Win32Window* given = Win32FindWindow(parentHandle);
	if (manager().isDesktop(given))
	{
		parent = nullptr;
		owner = nullptr;
		return (style & WS_CHILD) == 0;
	}

	if ((style & WS_CHILD) != 0)
	{
		parent = given;
		owner = nullptr;
		return parent != nullptr;
	}

	parent = nullptr;
	owner = Win32TopLevelOf(given);
	return parentHandle == nullptr || given != nullptr;
}

RECT initialRect(DWORD style, int x, int y, int width, int height)
{
	const SIZE host = manager().hostSize();
	const bool isTopLevel = (style & WS_CHILD) == 0;
	if (isTopLevel && x == CW_USEDEFAULT)
	{
		x = kDefaultTopLevelOffset;
		y = kDefaultTopLevelOffset;
	}
	if (isTopLevel && width == CW_USEDEFAULT)
	{
		width = host.cx * 2 / 3;
		height = host.cy * 2 / 3;
	}

	RECT rect;
	rect.left = x == CW_USEDEFAULT ? 0 : x;
	rect.top = y == CW_USEDEFAULT ? 0 : y;
	rect.right = rect.left + std::max(0, width == CW_USEDEFAULT ? 0 : width);
	rect.bottom = rect.top + std::max(0, height);
	return rect;
}

void fillCreateStruct(CREATESTRUCTA& create, const Win32Window* window, LPCSTR className, LPCSTR windowName, HMENU menu, HINSTANCE instance, void* param)
{
	create.lpCreateParams = param;
	create.hInstance = instance;
	create.hMenu = menu;
	create.hwndParent = window->parent != nullptr ? window->parent->hwnd : (window->owner != nullptr ? window->owner->hwnd : nullptr);
	create.x = window->rect.left;
	create.y = window->rect.top;
	create.cx = window->width();
	create.cy = window->height();
	create.style = static_cast<LONG>(window->style);
	create.lpszName = windowName;
	create.lpszClass = className;
	create.dwExStyle = window->exStyle;
}

void applyCreateStruct(Win32Window* window, const CREATESTRUCTA& create)
{
	window->style = static_cast<DWORD>(create.style);
	window->exStyle = create.dwExStyle;
	window->rect.left = create.x;
	window->rect.top = create.y;
	window->rect.right = create.x + create.cx;
	window->rect.bottom = create.y + create.cy;
}

void notifyParentOfChild(Win32Window* window, UINT event)
{
	if (!window->isChild() || window->parent == nullptr || (window->exStyle & WS_EX_NOPARENTNOTIFY) != 0)
	{
		return;
	}
	SendMessageA(window->parent->hwnd, WM_PARENTNOTIFY, MAKEWPARAM(event, window->id), reinterpret_cast<LPARAM>(window->hwnd));
}

void sendInitialGeometry(Win32Window* window)
{
	const RECT client = manager().clientRectInWindow(window);
	SendMessageA(window->hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(client.right - client.left, client.bottom - client.top));
	SendMessageA(window->hwnd, WM_MOVE, 0, MAKELPARAM(window->rect.left + client.left, window->rect.top + client.top));
}

bool runCreationMessages(Win32Window* window, CREATESTRUCTA& create)
{
	CBT_CREATEWNDA cbtCreate = { &create, HWND_TOP };
	if (Win32CallCbtHooks(HCBT_CREATEWND, reinterpret_cast<WPARAM>(window->hwnd), reinterpret_cast<LPARAM>(&cbtCreate)) != 0)
	{
		return false;
	}
	applyCreateStruct(window, create);
	Win32ApplyHostWindowPolicy(window, create);

	if (!SendMessageA(window->hwnd, WM_NCCREATE, 0, reinterpret_cast<LPARAM>(&create)))
	{
		return false;
	}
	Win32RecalculateClientRect(window);
	return SendMessageA(window->hwnd, WM_CREATE, 0, reinterpret_cast<LPARAM>(&create)) != -1;
}

bool isInside(Win32Window* window, Win32Window* ancestor)
{
	for (Win32Window* current = window; current != nullptr; current = current->parent)
	{
		if (current == ancestor)
		{
			return true;
		}
	}
	return false;
}

void releaseFocusAndCapture(Win32Window* window)
{
	Win32Window* focus = manager().focus();
	if (focus != nullptr && isInside(focus, window))
	{
		SetFocus(window->parent != nullptr ? window->parent->hwnd : (window->owner != nullptr ? window->owner->hwnd : nullptr));
	}

	Win32Window* capture = manager().capture();
	if (capture != nullptr && isInside(capture, window))
	{
		ReleaseCapture();
	}
}

void destroyOwnedWindows(Win32Window* window)
{
	const std::vector<Win32Window*> topLevel = manager().topLevelWindows();
	for (Win32Window* candidate : topLevel)
	{
		if (candidate->owner == window && IsWindow(candidate->hwnd))
		{
			DestroyWindow(candidate->hwnd);
		}
	}
}

void destroyTree(Win32Window* window)
{
	const HWND hwnd = window->hwnd;
	SendMessageA(hwnd, WM_DESTROY, 0, 0);

	const std::vector<Win32Window*> children = window->children;
	for (Win32Window* child : children)
	{
		Win32Window* stillAlive = Win32FindWindow(child->hwnd);
		if (stillAlive != nullptr && !stillAlive->isDestroying)
		{
			stillAlive->isDestroying = true;
			destroyTree(stillAlive);
		}
	}

	Win32KillWindowTimers(window);
	Win32RemoveWindowMessages(window);
	SendMessageA(hwnd, WM_NCDESTROY, 0, 0);

	Win32Window* remaining = Win32FindWindow(hwnd);
	if (remaining != nullptr)
	{
		manager().releaseWindow(remaining);
	}
}

void activateNextTopLevel(Win32Window* window)
{
	if (manager().active() != window)
	{
		return;
	}

	Win32Window* next = window->owner;
	if (next == nullptr)
	{
		for (Win32Window* candidate : manager().topLevelWindows())
		{
			if (candidate != window && candidate->isVisible() && !candidate->isDestroying)
			{
				next = candidate;
				break;
			}
		}
	}
	SetActiveWindow(next != nullptr ? next->hwnd : nullptr);
}

}

HWND CreateWindowExA(DWORD exStyle, LPCSTR className, LPCSTR windowName, DWORD style, int x, int y, int width, int height,
	HWND parentHandle, HMENU menu, HINSTANCE instance, void* param)
{
	Win32WindowClass* windowClass = Win32ClassRegistry::instance().find(className);
	Win32Window* parent = nullptr;
	Win32Window* owner = nullptr;
	if (windowClass == nullptr || !resolveRelatives(style, parentHandle, parent, owner))
	{
		return nullptr;
	}

	Win32Window* window = manager().createWindow(windowClass, exStyle, style & ~WS_VISIBLE, parent, owner);
	window->rect = initialRect(style, x, y, width, height);
	window->id = (style & WS_CHILD) != 0 ? reinterpret_cast<UINT_PTR>(menu) : 0;
	window->menu = (style & WS_CHILD) == 0 ? menu : nullptr;
	window->instance = instance;
	window->text = windowName != nullptr && !IS_INTRESOURCE(windowName) ? windowName : "";
	window->windowProc = windowClass->windowProc;
	window->extraWords.assign(static_cast<size_t>((windowClass->windowExtra + sizeof(LONG_PTR) - 1) / sizeof(LONG_PTR)), 0);
	const HWND hwnd = window->hwnd;

	CREATESTRUCTA create = {};
	fillCreateStruct(create, window, className, windowName, menu, instance, param);
	create.style = static_cast<LONG>(style & ~WS_VISIBLE);
	if (!runCreationMessages(window, create))
	{
		if (IsWindow(hwnd))
		{
			DestroyWindow(hwnd);
		}
		return nullptr;
	}

	sendInitialGeometry(window);
	notifyParentOfChild(window, WM_CREATE);
	if ((style & WS_VISIBLE) != 0 && IsWindow(hwnd))
	{
		ShowWindow(hwnd, SW_SHOW);
	}
	return IsWindow(hwnd) ? hwnd : nullptr;
}

BOOL DestroyWindow(HWND hwnd)
{
	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return FALSE;
	}
	if (window->isDestroying)
	{
		return TRUE;
	}

	Win32CallCbtHooks(HCBT_DESTROYWND, reinterpret_cast<WPARAM>(hwnd), 0);
	window->isDestroying = true;
	destroyOwnedWindows(window);
	notifyParentOfChild(window, WM_DESTROY);
	releaseFocusAndCapture(window);
	activateNextTopLevel(window);
	if (window->isVisible() && window->parent != nullptr)
	{
		Win32InvalidateTree(window->parent, true);
	}
	if (!window->isChild() && window->menu != nullptr)
	{
		DestroyMenu(window->menu);
		window->menu = nullptr;
	}

	destroyTree(window);
	return TRUE;
}

BOOL IsWindow(HWND hwnd)
{
	return Win32FindWindow(hwnd) != nullptr ? TRUE : FALSE;
}

BOOL IsChild(HWND parentHandle, HWND hwnd)
{
	Win32Window* parent = Win32FindWindow(parentHandle);
	Win32Window* window = Win32FindWindow(hwnd);
	if (parent == nullptr || window == nullptr || window == parent)
	{
		return FALSE;
	}

	for (Win32Window* current = window; current != nullptr && current->isChild(); current = current->parent)
	{
		if (current->parent == parent)
		{
			return TRUE;
		}
	}
	return FALSE;
}

HWND GetParent(HWND hwnd)
{
	const Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return nullptr;
	}
	if (window->isChild())
	{
		return window->parent != nullptr ? window->parent->hwnd : nullptr;
	}
	const bool isPopup = (window->style & WS_POPUP) != 0;
	return isPopup && window->owner != nullptr ? window->owner->hwnd : nullptr;
}

HWND SetParent(HWND hwnd, HWND newParentHandle)
{
	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return nullptr;
	}

	const HWND previous = window->parent != nullptr ? window->parent->hwnd : nullptr;
	Win32Window* newParent = Win32FindWindow(newParentHandle);
	manager().detachFromParent(window);
	manager().attachToParent(window, newParent);
	if (newParent == nullptr)
	{
		window->style &= ~WS_CHILD;
	}
	Win32InvalidateTree(window, true);
	return previous;
}

HWND GetWindow(HWND hwnd, UINT command)
{
	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return nullptr;
	}

	if (command == GW_CHILD)
	{
		const std::vector<Win32Window*>& children = manager().isDesktop(window) ? manager().topLevelWindows() : window->children;
		return children.empty() ? nullptr : children.front()->hwnd;
	}
	if (command == GW_OWNER)
	{
		return window->owner != nullptr ? window->owner->hwnd : nullptr;
	}

	const std::vector<Win32Window*>& siblings = window->parent != nullptr ? window->parent->children : manager().topLevelWindows();
	const auto position = std::find(siblings.begin(), siblings.end(), window);
	switch (command)
	{
	case GW_HWNDFIRST:
		return siblings.front()->hwnd;
	case GW_HWNDLAST:
		return siblings.back()->hwnd;
	case GW_HWNDNEXT:
		return position + 1 < siblings.end() ? (*(position + 1))->hwnd : nullptr;
	case GW_HWNDPREV:
		return position > siblings.begin() ? (*(position - 1))->hwnd : nullptr;
	default:
		return nullptr;
	}
}

HWND GetTopWindow(HWND hwnd)
{
	if (hwnd == nullptr)
	{
		const std::vector<Win32Window*>& topLevel = manager().topLevelWindows();
		return topLevel.empty() ? nullptr : topLevel.front()->hwnd;
	}
	return GetWindow(hwnd, GW_CHILD);
}

HWND GetDesktopWindow()
{
	return manager().desktop()->hwnd;
}
