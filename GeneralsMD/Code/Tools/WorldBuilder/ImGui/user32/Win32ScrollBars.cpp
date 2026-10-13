#include "Win32Window.h"

#include <algorithm>
#include <vector>

namespace
{

Win32ScrollState* scrollStateOf(Win32Window* window, int bar)
{
	if (window == nullptr)
	{
		return nullptr;
	}
	switch (bar)
	{
	case SB_HORZ:
		return &window->horizontalScroll;
	case SB_VERT:
		return &window->verticalScroll;
	default:
		return nullptr;
	}
}

DWORD scrollStyleOf(int bar)
{
	return bar == SB_HORZ ? WS_HSCROLL : WS_VSCROLL;
}

int maximumPosition(const Win32ScrollState& state)
{
	const int pageSpan = state.page > 0 ? static_cast<int>(state.page) - 1 : 0;
	return std::max(state.minimum, state.maximum - pageSpan);
}

bool needsScrollBar(const Win32ScrollState& state)
{
	return state.maximum > state.minimum && state.page <= static_cast<UINT>(state.maximum - state.minimum);
}

void applyScrollInfo(Win32ScrollState& state, const SCROLLINFO& info)
{
	if ((info.fMask & SIF_RANGE) != 0)
	{
		state.minimum = info.nMin;
		state.maximum = info.nMax;
	}
	if ((info.fMask & SIF_PAGE) != 0)
	{
		const UINT rangeSpan = static_cast<UINT>(std::max(state.maximum - state.minimum + 1, 0));
		state.page = std::min(info.nPage, rangeSpan);
	}
	if ((info.fMask & SIF_POS) != 0)
	{
		state.position = info.nPos;
	}
	state.position = std::clamp(state.position, state.minimum, maximumPosition(state));
	state.hasInfo = true;
}

void updateScrollBarVisibility(Win32Window* window, int bar, UINT mask)
{
	const DWORD style = scrollStyleOf(bar);
	const bool isShown = (window->style & style) != 0;
	const bool keepsDisabledBar = (mask & SIF_DISABLENOSCROLL) != 0 && isShown;
	const bool shouldShow = needsScrollBar(*scrollStateOf(window, bar)) || keepsDisabledBar;
	if (shouldShow == isShown)
	{
		return;
	}

	window->style = shouldShow ? window->style | style : window->style & ~style;
	SetWindowPos(window->hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

void fillScrollInfo(const Win32ScrollState& state, SCROLLINFO& info)
{
	if ((info.fMask & SIF_RANGE) != 0)
	{
		info.nMin = state.minimum;
		info.nMax = state.maximum;
	}
	if ((info.fMask & SIF_PAGE) != 0)
	{
		info.nPage = state.page;
	}
	if ((info.fMask & SIF_POS) != 0)
	{
		info.nPos = state.position;
	}
	if ((info.fMask & SIF_TRACKPOS) != 0)
	{
		info.nTrackPos = state.trackPosition;
	}
}

void offsetChildren(Win32Window* window, int dx, int dy)
{
	const std::vector<Win32Window*> children = window->children;
	for (Win32Window* child : children)
	{
		SetWindowPos(child->hwnd, nullptr, child->rect.left + dx, child->rect.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
}

}

int SetScrollInfo(HWND hwnd, int bar, LPCSCROLLINFO info, BOOL redraw)
{
	if (bar == SB_CTL)
	{
		return static_cast<int>(SendMessageA(hwnd, SBM_SETSCROLLINFO, redraw, reinterpret_cast<LPARAM>(info)));
	}

	Win32Window* window = Win32FindWindow(hwnd);
	Win32ScrollState* state = scrollStateOf(window, bar);
	if (state == nullptr || info == nullptr)
	{
		return 0;
	}

	applyScrollInfo(*state, *info);
	updateScrollBarVisibility(window, bar, info->fMask);
	if (redraw)
	{
		window->needsNonClientPaint = true;
	}
	return state->position;
}

BOOL GetScrollInfo(HWND hwnd, int bar, LPSCROLLINFO info)
{
	if (bar == SB_CTL)
	{
		return static_cast<BOOL>(SendMessageA(hwnd, SBM_GETSCROLLINFO, 0, reinterpret_cast<LPARAM>(info)));
	}

	const Win32ScrollState* state = scrollStateOf(Win32FindWindow(hwnd), bar);
	if (state == nullptr || info == nullptr || !state->hasInfo)
	{
		return FALSE;
	}
	fillScrollInfo(*state, *info);
	return TRUE;
}

int SetScrollPos(HWND hwnd, int bar, int position, BOOL redraw)
{
	const int previous = GetScrollPos(hwnd, bar);
	SCROLLINFO info = {};
	info.cbSize = sizeof(info);
	info.fMask = SIF_POS;
	info.nPos = position;
	SetScrollInfo(hwnd, bar, &info, redraw);
	return previous;
}

int GetScrollPos(HWND hwnd, int bar)
{
	SCROLLINFO info = {};
	info.cbSize = sizeof(info);
	info.fMask = SIF_POS;
	return GetScrollInfo(hwnd, bar, &info) ? info.nPos : 0;
}

BOOL SetScrollRange(HWND hwnd, int bar, int minimum, int maximum, BOOL redraw)
{
	SCROLLINFO info = {};
	info.cbSize = sizeof(info);
	info.fMask = SIF_RANGE;
	info.nMin = minimum;
	info.nMax = maximum;
	SetScrollInfo(hwnd, bar, &info, redraw);
	return TRUE;
}

BOOL GetScrollRange(HWND hwnd, int bar, int* minimum, int* maximum)
{
	SCROLLINFO info = {};
	info.cbSize = sizeof(info);
	info.fMask = SIF_RANGE;
	const BOOL result = GetScrollInfo(hwnd, bar, &info);
	*minimum = result ? info.nMin : 0;
	*maximum = result ? info.nMax : 0;
	return result;
}

BOOL ScrollWindow(HWND hwnd, int dx, int dy, const RECT* scrollRect, const RECT* clipRect)
{
	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr)
	{
		return FALSE;
	}

	if (scrollRect == nullptr)
	{
		offsetChildren(window, dx, dy);
	}

	RECT invalid;
	if (scrollRect != nullptr)
	{
		invalid = *scrollRect;
	}
	else
	{
		GetClientRect(hwnd, &invalid);
	}
	if (clipRect != nullptr)
	{
		IntersectRect(&invalid, &invalid, clipRect);
	}
	InvalidateRect(hwnd, &invalid, TRUE);
	return TRUE;
}
