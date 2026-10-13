#include "Win32Messages.h"
#include "Win32Window.h"

namespace
{

Win32WindowManager& manager()
{
	return Win32WindowManager::instance();
}

Win32Window* ownerRootOf(Win32Window* window)
{
	Win32Window* root = window;
	while (root->owner != nullptr)
	{
		root = root->owner;
	}
	return root;
}

void rememberActivePopup(Win32Window* window)
{
	Win32Window* root = ownerRootOf(window);
	root->lastActivePopup = window != root ? window : nullptr;
}

bool canTakeFocus(const Win32Window* window)
{
	for (const Win32Window* current = window; current != nullptr; current = current->isChild() ? current->parent : nullptr)
	{
		if (!current->isEnabled())
		{
			return false;
		}
	}
	return true;
}

}

HWND SetFocus(HWND hwnd)
{
	Win32Window* window = Win32FindWindow(hwnd);
	Win32Window* previous = manager().focus();
	const HWND previousHandle = previous != nullptr ? previous->hwnd : nullptr;
	if (window == previous)
	{
		return previousHandle;
	}
	if (hwnd != nullptr && (window == nullptr || !canTakeFocus(window)))
	{
		return nullptr;
	}

	if (window != nullptr)
	{
		Win32Window* topLevel = Win32TopLevelOf(window);
		if (manager().active() != topLevel)
		{
			SetActiveWindow(topLevel->hwnd);
		}
	}

	if (previous != nullptr && manager().focus() == previous)
	{
		manager().setFocusWindow(nullptr);
		SendMessageA(previousHandle, WM_KILLFOCUS, reinterpret_cast<WPARAM>(hwnd), 0);
	}

	manager().setFocusWindow(window);
	if (window != nullptr)
	{
		SendMessageA(hwnd, WM_SETFOCUS, reinterpret_cast<WPARAM>(previousHandle), 0);
	}
	return IsWindow(previousHandle) ? previousHandle : nullptr;
}

HWND GetFocus()
{
	Win32Window* focus = manager().focus();
	return focus != nullptr ? focus->hwnd : nullptr;
}

HWND SetActiveWindow(HWND hwnd)
{
	Win32Window* window = Win32TopLevelOf(Win32FindWindow(hwnd));
	Win32Window* previous = manager().active();
	const HWND previousHandle = previous != nullptr ? previous->hwnd : nullptr;
	if (window == previous)
	{
		return previousHandle;
	}

	const HWND windowHandle = window != nullptr ? window->hwnd : nullptr;
	if (previous != nullptr)
	{
		SendMessageA(previousHandle, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), reinterpret_cast<LPARAM>(windowHandle));
	}

	manager().setActiveWindow(window);
	if (window == nullptr)
	{
		return previousHandle;
	}

	manager().bringToTop(window);
	rememberActivePopup(window);
	SendMessageA(windowHandle, WM_ACTIVATE, MAKEWPARAM(WA_ACTIVE, 0), reinterpret_cast<LPARAM>(previousHandle));
	return IsWindow(previousHandle) ? previousHandle : nullptr;
}

HWND GetActiveWindow()
{
	Win32Window* active = manager().active();
	return active != nullptr ? active->hwnd : nullptr;
}

HWND GetLastActivePopup(HWND hwnd)
{
	const Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return nullptr;
	}
	const Win32Window* popup = window->lastActivePopup;
	return popup != nullptr && popup->isVisible() && popup->isEnabled() ? popup->hwnd : hwnd;
}

BOOL SetForegroundWindow(HWND hwnd)
{
	return SetActiveWindow(hwnd) != nullptr || GetActiveWindow() == hwnd ? TRUE : FALSE;
}

HWND GetForegroundWindow()
{
	return GetActiveWindow();
}

HWND SetCapture(HWND hwnd)
{
	Win32Window* window = Win32FindWindow(hwnd);
	Win32Window* previous = manager().capture();
	const HWND previousHandle = previous != nullptr ? previous->hwnd : nullptr;
	manager().setCaptureWindow(window);
	if (previous != nullptr && previous != window)
	{
		SendMessageA(previousHandle, WM_CAPTURECHANGED, 0, reinterpret_cast<LPARAM>(hwnd));
	}
	return previousHandle;
}

BOOL ReleaseCapture()
{
	Win32Window* previous = manager().capture();
	manager().setCaptureWindow(nullptr);
	if (previous != nullptr)
	{
		SendMessageA(previous->hwnd, WM_CAPTURECHANGED, 0, 0);
	}
	return TRUE;
}

HWND GetCapture()
{
	Win32Window* capture = manager().capture();
	return capture != nullptr ? capture->hwnd : nullptr;
}

BOOL InvalidateRect(HWND hwnd, const RECT*, BOOL erase)
{
	if (hwnd == nullptr)
	{
		for (Win32Window* window : manager().topLevelWindows())
		{
			Win32InvalidateTree(window, erase != FALSE);
		}
		return TRUE;
	}

	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return FALSE;
	}
	window->needsPaint = true;
	window->needsErase = window->needsErase || erase != FALSE;
	GetClientRect(hwnd, &window->updateRect);
	return TRUE;
}

BOOL InvalidateRgn(HWND hwnd, HRGN, BOOL erase)
{
	return InvalidateRect(hwnd, nullptr, erase);
}

BOOL ValidateRect(HWND hwnd, const RECT*)
{
	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return FALSE;
	}
	window->needsPaint = false;
	window->needsErase = false;
	SetRectEmpty(&window->updateRect);
	return TRUE;
}

BOOL GetUpdateRect(HWND hwnd, LPRECT rect, BOOL)
{
	const Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr || !window->needsPaint)
	{
		if (rect != nullptr)
		{
			SetRectEmpty(rect);
		}
		return FALSE;
	}
	if (rect != nullptr)
	{
		*rect = window->updateRect;
	}
	return TRUE;
}

BOOL UpdateWindow(HWND hwnd)
{
	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return FALSE;
	}
	if (window->needsPaint && window->isVisible())
	{
		SendMessageA(hwnd, WM_PAINT, 0, 0);
	}
	return TRUE;
}

static void updateTree(Win32Window* window)
{
	UpdateWindow(window->hwnd);
	const std::vector<Win32Window*> children = window->children;
	for (Win32Window* child : children)
	{
		if (IsWindow(child->hwnd))
		{
			updateTree(child);
		}
	}
}

BOOL RedrawWindow(HWND hwnd, const RECT* updateRect, HRGN, UINT flags)
{
	Win32Window* window = hwnd != nullptr ? Win32FindWindow(hwnd) : nullptr;
	if (hwnd != nullptr && window == nullptr)
	{
		return FALSE;
	}
	if (window == nullptr)
	{
		return InvalidateRect(nullptr, updateRect, (flags & RDW_ERASE) != 0);
	}

	const bool allChildren = (flags & RDW_ALLCHILDREN) != 0;
	if ((flags & RDW_INVALIDATE) != 0)
	{
		if (allChildren)
		{
			Win32InvalidateTree(window, (flags & RDW_ERASE) != 0);
		}
		InvalidateRect(hwnd, updateRect, (flags & RDW_ERASE) != 0);
	}
	if ((flags & RDW_VALIDATE) != 0)
	{
		ValidateRect(hwnd, updateRect);
	}
	if ((flags & (RDW_UPDATENOW | RDW_ERASENOW)) != 0)
	{
		allChildren ? updateTree(window) : static_cast<void>(UpdateWindow(hwnd));
	}
	return TRUE;
}
