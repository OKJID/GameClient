#include "Win32InputQueue.h"

#include "Win32Class.h"
#include "Win32Messages.h"
#include "Win32Window.h"

#include <cstdlib>
#include <deque>

namespace
{

enum class Win32InputKind
{
	Mouse,
	Keyboard,
};

enum class Win32InputOutcome
{
	Deliver,
	Drop,
	BlockedClick,
	NonClientClick,
};

struct Win32RawInput
{
	Win32InputKind kind = Win32InputKind::Mouse;
	Win32MouseInput mouse;
	UINT message = 0;
	WPARAM wParam = 0;
	LPARAM lParam = 0;
	DWORD time = 0;
	POINT cursor = {};
};

struct Win32LastClick
{
	HWND hwnd = nullptr;
	UINT message = 0;
	DWORD time = 0;
	POINT point = {};
};

struct Win32InputState
{
	std::deque<Win32RawInput> events;
	Win32LastClick lastClick;
};

Win32InputState& state()
{
	static Win32InputState inputState;
	return inputState;
}

Win32WindowManager& manager()
{
	return Win32WindowManager::instance();
}

bool isButtonDown(UINT message)
{
	return message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN;
}

bool isButtonPress(UINT message)
{
	return isButtonDown(message) || message == WM_LBUTTONDBLCLK || message == WM_RBUTTONDBLCLK || message == WM_MBUTTONDBLCLK;
}

UINT systemKeyMessage(UINT message)
{
	switch (message)
	{
	case WM_KEYDOWN:
		return WM_SYSKEYDOWN;
	case WM_KEYUP:
		return WM_SYSKEYUP;
	case WM_CHAR:
		return WM_SYSCHAR;
	default:
		return message;
	}
}

Win32Window* childAt(Win32Window* window, POINT screenPoint)
{
	const POINT origin = manager().clientOriginOnScreen(window);
	const std::vector<Win32Window*> children = window->children;
	for (Win32Window* child : children)
	{
		if (!child->isVisible() || !child->isEnabled() || child->presentation == Win32Presentation::OwnWindow)
		{
			continue;
		}
		RECT rect = child->rect;
		OffsetRect(&rect, origin.x, origin.y);
		if (!PtInRect(&rect, screenPoint))
		{
			continue;
		}
		if (SendMessageA(child->hwnd, WM_NCHITTEST, 0, MAKELPARAM(screenPoint.x, screenPoint.y)) == HTTRANSPARENT)
		{
			continue;
		}
		return IsWindow(child->hwnd) ? childAt(child, screenPoint) : nullptr;
	}
	return window;
}

Win32Window* hitTest(Win32Window* root, POINT screenPoint)
{
	const RECT client = manager().clientRectOnScreen(root);
	return PtInRect(&client, screenPoint) ? childAt(root, screenPoint) : nullptr;
}

UINT clickMessage(UINT message, const Win32Window* target, POINT screenPoint, DWORD time)
{
	const bool hasDoubleClicks = target->windowClass != nullptr && (target->windowClass->style & CS_DBLCLKS) != 0;
	if (!isButtonDown(message) || !hasDoubleClicks)
	{
		return message;
	}

	const Win32LastClick& last = state().lastClick;
	const bool isNearLastClick = std::abs(screenPoint.x - last.point.x) <= GetSystemMetrics(SM_CXDOUBLECLK) / 2
		&& std::abs(screenPoint.y - last.point.y) <= GetSystemMetrics(SM_CYDOUBLECLK) / 2;
	const bool isDoubleClick = last.hwnd == target->hwnd && last.message == message && time - last.time <= GetDoubleClickTime() && isNearLastClick;
	return isDoubleClick ? message + (WM_LBUTTONDBLCLK - WM_LBUTTONDOWN) : message;
}

void rememberClick(const MSG& message)
{
	Win32LastClick& last = state().lastClick;
	if (!isButtonPress(message.message))
	{
		return;
	}
	if (!isButtonDown(message.message))
	{
		last = Win32LastClick();
		return;
	}
	last.hwnd = message.hwnd;
	last.message = message.message;
	last.time = message.time;
	last.point = message.pt;
}

Win32InputOutcome mouseTarget(const Win32RawInput& input, Win32Window*& target)
{
	target = manager().capture();
	if (target != nullptr)
	{
		return Win32InputOutcome::Deliver;
	}

	Win32Window* root = Win32FindWindow(input.mouse.rootWindow);
	if (root == nullptr)
	{
		return Win32InputOutcome::Drop;
	}
	const bool isClick = isButtonDown(input.mouse.message);
	if (Win32IsInputBlocked(root))
	{
		return isClick ? Win32InputOutcome::BlockedClick : Win32InputOutcome::Drop;
	}
	target = hitTest(root, input.mouse.screenPoint);
	if (target != nullptr)
	{
		return Win32InputOutcome::Deliver;
	}
	target = root;
	return isClick ? Win32InputOutcome::NonClientClick : Win32InputOutcome::Drop;
}

bool resolveWheel(const Win32RawInput& input, Win32Window* target, MSG& message)
{
	if (manager().capture() == nullptr && target->control != nullptr)
	{
		return false;
	}
	message.hwnd = target->hwnd;
	message.message = WM_MOUSEWHEEL;
	message.wParam = MAKEWPARAM(input.mouse.keyState, input.mouse.wheelDelta);
	message.lParam = MAKELPARAM(input.mouse.screenPoint.x, input.mouse.screenPoint.y);
	return true;
}

Win32InputOutcome resolveMouse(const Win32RawInput& input, MSG& message)
{
	Win32Window* target = nullptr;
	const Win32InputOutcome outcome = mouseTarget(input, target);
	message.hwnd = target != nullptr ? target->hwnd : nullptr;
	message.message = input.mouse.message;
	message.time = input.time;
	message.pt = input.mouse.screenPoint;
	if (outcome != Win32InputOutcome::Deliver)
	{
		return outcome;
	}
	if (input.mouse.message == WM_MOUSEWHEEL)
	{
		return resolveWheel(input, target, message) ? Win32InputOutcome::Deliver : Win32InputOutcome::Drop;
	}

	const POINT origin = manager().clientOriginOnScreen(target);
	message.message = clickMessage(input.mouse.message, target, input.mouse.screenPoint, input.time);
	message.wParam = input.mouse.keyState;
	message.lParam = MAKELPARAM(input.mouse.screenPoint.x - origin.x, input.mouse.screenPoint.y - origin.y);
	return Win32InputOutcome::Deliver;
}

Win32InputOutcome resolveKeyboard(const Win32RawInput& input, MSG& message)
{
	Win32Window* focus = manager().focus();
	Win32Window* target = focus != nullptr ? focus : manager().active();
	if (target == nullptr)
	{
		return Win32InputOutcome::Drop;
	}
	message.hwnd = target->hwnd;
	message.message = focus != nullptr ? input.message : systemKeyMessage(input.message);
	message.wParam = input.wParam;
	message.lParam = input.lParam;
	message.time = input.time;
	message.pt = input.cursor;
	return Win32InputOutcome::Deliver;
}

Win32InputOutcome resolve(const Win32RawInput& input, MSG& message)
{
	return input.kind == Win32InputKind::Mouse ? resolveMouse(input, message) : resolveKeyboard(input, message);
}

bool activateForClick(Win32Window* target, UINT message, int hitTestCode)
{
	Win32Window* topLevel = Win32TopLevelOf(target);
	if (topLevel == nullptr || manager().active() == topLevel)
	{
		return true;
	}
	const LRESULT result = SendMessageA(target->hwnd, WM_MOUSEACTIVATE, reinterpret_cast<WPARAM>(topLevel->hwnd), MAKELPARAM(hitTestCode, message));
	if ((result == MA_ACTIVATE || result == MA_ACTIVATEANDEAT) && IsWindow(topLevel->hwnd))
	{
		SetActiveWindow(topLevel->hwnd);
	}
	return result != MA_ACTIVATEANDEAT && result != MA_NOACTIVATEANDEAT;
}

bool prepareMouseDelivery(const MSG& message)
{
	rememberClick(message);
	if (manager().capture() != nullptr || message.message == WM_MOUSEWHEEL)
	{
		return true;
	}
	Win32Window* target = Win32FindWindow(message.hwnd);
	if (target == nullptr || (isButtonPress(message.message) && !activateForClick(target, message.message, HTCLIENT)))
	{
		return false;
	}
	if (!IsWindow(message.hwnd))
	{
		return false;
	}
	SendMessageA(message.hwnd, WM_SETCURSOR, reinterpret_cast<WPARAM>(message.hwnd), MAKELPARAM(HTCLIENT, message.message));
	return IsWindow(message.hwnd) != FALSE;
}

void handleUndelivered(const MSG& message, Win32InputOutcome outcome)
{
	if (outcome == Win32InputOutcome::BlockedClick)
	{
		MessageBeep(MB_OK);
		return;
	}
	Win32Window* root = Win32FindWindow(message.hwnd);
	if (outcome == Win32InputOutcome::NonClientClick && root != nullptr)
	{
		activateForClick(root, message.message, HTCAPTION);
	}
}

}

namespace Win32InputQueue
{

void postMouse(const Win32MouseInput& input)
{
	Win32RawInput raw;
	raw.kind = Win32InputKind::Mouse;
	raw.mouse = input;
	raw.time = Win32CurrentTime();
	raw.cursor = input.screenPoint;
	state().events.push_back(raw);
}

void postKeyboard(UINT message, WPARAM virtualKey, LPARAM keyData)
{
	Win32RawInput raw;
	raw.kind = Win32InputKind::Keyboard;
	raw.message = message;
	raw.wParam = virtualKey;
	raw.lParam = keyData;
	raw.time = Win32CurrentTime();
	GetCursorPos(&raw.cursor);
	state().events.push_back(raw);
}

bool take(LPMSG message, HWND hwnd, UINT filterMin, UINT filterMax, bool remove)
{
	std::deque<Win32RawInput>& events = state().events;
	for (size_t index = 0; index < events.size();)
	{
		const Win32RawInput input = events[index];
		MSG resolved = {};
		const Win32InputOutcome outcome = resolve(input, resolved);
		if (outcome != Win32InputOutcome::Deliver && !remove)
		{
			++index;
			continue;
		}
		if (outcome != Win32InputOutcome::Deliver)
		{
			events.erase(events.begin() + static_cast<long>(index));
			handleUndelivered(resolved, outcome);
			continue;
		}
		if (!Win32MatchesMessageFilter(resolved, hwnd, filterMin, filterMax))
		{
			++index;
			continue;
		}
		if (!remove)
		{
			*message = resolved;
			return true;
		}

		events.erase(events.begin() + static_cast<long>(index));
		const bool isDeliverable = input.kind == Win32InputKind::Keyboard || prepareMouseDelivery(resolved);
		if (isDeliverable)
		{
			*message = resolved;
			return true;
		}
	}
	return false;
}

}
