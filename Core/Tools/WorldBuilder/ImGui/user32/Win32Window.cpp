#include "Win32Window.h"

#include "Win32Class.h"
#include "Win32Control.h"
#include "Win32Dialog.h"
#include "Win32Gdi.h"
#include "Win32Metrics.h"

#include <algorithm>

static const char kDesktopClassName[] = "#32769";
static const int kDefaultScrollRange = 100;

Win32WindowManager& Win32WindowManager::instance()
{
	static Win32WindowManager manager;
	return manager;
}

Win32Window* Win32WindowManager::find(HWND hwnd) const
{
	const auto it = m_windows.find(hwnd);
	return it == m_windows.end() ? nullptr : it->second.get();
}

Win32Window* Win32WindowManager::createWindow(Win32WindowClass* windowClass, DWORD exStyle, DWORD style, Win32Window* parent, Win32Window* owner)
{
	auto window = std::make_unique<Win32Window>();
	window->hwnd = reinterpret_cast<HWND>(m_nextHandle);
	m_nextHandle += 0x10;
	window->windowClass = windowClass;
	window->exStyle = exStyle;
	window->style = style;
	window->owner = owner;
	window->horizontalScroll.hasInfo = (style & WS_HSCROLL) != 0;
	window->horizontalScroll.maximum = window->horizontalScroll.hasInfo ? kDefaultScrollRange : 0;
	window->verticalScroll.hasInfo = (style & WS_VSCROLL) != 0;
	window->verticalScroll.maximum = window->verticalScroll.hasInfo ? kDefaultScrollRange : 0;

	Win32Window* created = window.get();
	m_windows[created->hwnd] = std::move(window);
	attachToParent(created, parent);
	return created;
}

void Win32WindowManager::releaseWindow(Win32Window* window)
{
	forgetWindow(window);
	detachFromParent(window);
	m_windows.erase(window->hwnd);
}

void Win32WindowManager::forgetWindow(Win32Window* window)
{
	if (m_focus == window)
	{
		m_focus = nullptr;
	}
	if (m_capture == window)
	{
		m_capture = nullptr;
	}
	if (m_active == window)
	{
		m_active = nullptr;
	}
	if (m_hostWindow == window)
	{
		m_hostWindow = nullptr;
	}
	for (auto& entry : m_windows)
	{
		if (entry.second->owner == window)
		{
			entry.second->owner = nullptr;
		}
		if (entry.second->lastActivePopup == window)
		{
			entry.second->lastActivePopup = nullptr;
		}
	}
}

std::vector<Win32Window*>& Win32WindowManager::siblingsOf(Win32Window* window)
{
	return window->parent != nullptr ? window->parent->children : m_topLevel;
}

void Win32WindowManager::attachToParent(Win32Window* window, Win32Window* parent)
{
	window->parent = parent;
	siblingsOf(window).push_back(window);
}

void Win32WindowManager::detachFromParent(Win32Window* window)
{
	std::vector<Win32Window*>& siblings = siblingsOf(window);
	siblings.erase(std::remove(siblings.begin(), siblings.end(), window), siblings.end());
	window->parent = nullptr;
}

void Win32WindowManager::bringToTop(Win32Window* window)
{
	std::vector<Win32Window*>& siblings = siblingsOf(window);
	siblings.erase(std::remove(siblings.begin(), siblings.end(), window), siblings.end());
	siblings.insert(siblings.begin(), window);
}

void Win32WindowManager::placeAtBottom(Win32Window* window)
{
	std::vector<Win32Window*>& siblings = siblingsOf(window);
	siblings.erase(std::remove(siblings.begin(), siblings.end(), window), siblings.end());
	siblings.push_back(window);
}

void Win32WindowManager::placeBehind(Win32Window* window, Win32Window* insertAfter)
{
	std::vector<Win32Window*>& siblings = siblingsOf(window);
	siblings.erase(std::remove(siblings.begin(), siblings.end(), window), siblings.end());
	const auto position = std::find(siblings.begin(), siblings.end(), insertAfter);
	if (position == siblings.end())
	{
		siblings.insert(siblings.begin(), window);
		return;
	}
	siblings.insert(position + 1, window);
}

Win32Window* Win32WindowManager::desktop()
{
	if (m_desktop != nullptr)
	{
		return m_desktop;
	}

	WNDCLASSA desktopClass = {};
	desktopClass.lpfnWndProc = DefWindowProcA;
	desktopClass.lpszClassName = kDesktopClassName;
	RegisterClassA(&desktopClass);

	auto window = std::make_unique<Win32Window>();
	window->hwnd = reinterpret_cast<HWND>(m_nextHandle);
	m_nextHandle += 0x10;
	window->windowClass = Win32ClassRegistry::instance().find(kDesktopClassName);
	window->windowProc = DefWindowProcA;
	window->style = WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN;
	window->rect.right = m_hostSize.cx;
	window->rect.bottom = m_hostSize.cy;
	window->clientRect = window->rect;
	m_desktop = window.get();
	m_windows[m_desktop->hwnd] = std::move(window);
	return m_desktop;
}

void Win32WindowManager::setHostWindow(Win32Window* window)
{
	m_hostWindow = window;
	window->presentation = Win32Presentation::Host;
}

void Win32WindowManager::setHostSize(int width, int height)
{
	m_hostSize.cx = width;
	m_hostSize.cy = height;
	Win32Window* desktopWindow = desktop();
	desktopWindow->rect.right = width;
	desktopWindow->rect.bottom = height;
	desktopWindow->clientRect = desktopWindow->rect;
}

Win32NonClientMetrics Win32WindowManager::nonClientMetrics(const Win32Window* window) const
{
	return Win32NonClientMetricsFor(window->style, window->exStyle, window->presentation == Win32Presentation::Host);
}

Win32NonClientMetrics Win32NonClientMetricsFor(DWORD style, DWORD exStyle, bool isHost)
{
	Win32NonClientMetrics metrics;
	if (isHost || (style & WS_CHILD) != 0)
	{
		metrics.borderWidth = (style & WS_BORDER) != 0 || (exStyle & (WS_EX_CLIENTEDGE | WS_EX_STATICEDGE)) != 0 ? 1 : 0;
		return metrics;
	}

	const bool hasFrame = (style & (WS_BORDER | WS_DLGFRAME | WS_THICKFRAME)) != 0;
	metrics.borderWidth = hasFrame ? Win32Metrics::frameBorder() : 0;
	metrics.captionHeight = (style & WS_CAPTION) == WS_CAPTION ? Win32Metrics::captionHeight() : 0;
	return metrics;
}

RECT Win32WindowManager::clientRectInWindow(const Win32Window* window) const
{
	return window->clientRect;
}

void Win32RecalculateClientRect(Win32Window* window)
{
	RECT client = window->rect;
	SendMessageA(window->hwnd, WM_NCCALCSIZE, FALSE, reinterpret_cast<LPARAM>(&client));
	OffsetRect(&client, -window->rect.left, -window->rect.top);
	client.right = std::max(client.left, client.right);
	client.bottom = std::max(client.top, client.bottom);
	window->clientRect = client;
	window->needsNonClientPaint = true;
}

RECT Win32WindowManager::windowRectOnScreen(const Win32Window* window) const
{
	RECT rect = window->rect;
	if (window->parent == nullptr)
	{
		return rect;
	}

	const POINT origin = clientOriginOnScreen(window->parent);
	OffsetRect(&rect, origin.x, origin.y);
	return rect;
}

POINT Win32WindowManager::clientOriginOnScreen(const Win32Window* window) const
{
	const RECT windowRect = windowRectOnScreen(window);
	const RECT client = clientRectInWindow(window);
	POINT origin;
	origin.x = windowRect.left + client.left;
	origin.y = windowRect.top + client.top;
	return origin;
}

RECT Win32WindowManager::clientRectOnScreen(const Win32Window* window) const
{
	const RECT windowRect = windowRectOnScreen(window);
	RECT client = clientRectInWindow(window);
	OffsetRect(&client, windowRect.left, windowRect.top);
	return client;
}

namespace
{

Win32HostWindowPolicy& hostWindowPolicy()
{
	static Win32HostWindowPolicy policy = nullptr;
	return policy;
}

}

void Win32SetHostWindowPolicy(Win32HostWindowPolicy policy)
{
	hostWindowPolicy() = policy;
}

void Win32ApplyHostWindowPolicy(Win32Window* window, const CREATESTRUCTA& create)
{
	Win32WindowManager& manager = Win32WindowManager::instance();
	const Win32HostWindowPolicy policy = hostWindowPolicy();
	if (manager.hostWindow() != nullptr || policy == nullptr || window->isChild() || !policy(create))
	{
		return;
	}
	manager.setHostWindow(window);
}

bool Win32IsHostWindow(HWND hwnd)
{
	const Win32Window* window = Win32FindWindow(hwnd);
	return window != nullptr && window == Win32WindowManager::instance().hostWindow();
}

void Win32SetNativeSurface(HWND hwnd)
{
	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return;
	}
	window->presentsNativeSurface = true;
}

Win32Window* Win32FindWindow(HWND hwnd)
{
	return Win32WindowManager::instance().find(hwnd);
}

Win32Window* Win32TopLevelOf(Win32Window* window)
{
	while (window != nullptr && window->isChild() && window->parent != nullptr)
	{
		window = window->parent;
	}
	return window;
}

bool Win32IsInputBlocked(const Win32Window* window)
{
	for (const Win32Window* current = window; current != nullptr; current = current->isChild() ? current->parent : nullptr)
	{
		if (!current->isEnabled())
		{
			return true;
		}
	}
	return false;
}
