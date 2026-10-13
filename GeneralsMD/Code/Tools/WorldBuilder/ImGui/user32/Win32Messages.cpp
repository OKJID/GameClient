#include "Win32Messages.h"

#include "Win32InputQueue.h"

#include <chrono>
#include <deque>
#include <map>

namespace
{

struct Win32Timer
{
	HWND hwnd = nullptr;
	UINT_PTR id = 0;
	UINT elapse = 0;
	TIMERPROC timerProc = nullptr;
	DWORD dueTime = 0;
};

struct Win32Hook
{
	int type = 0;
	HOOKPROC hookProc = nullptr;
};

struct Win32PendingTask
{
	HWND hwnd = nullptr;
	Win32Task task;
};

struct Win32MessageState
{
	std::deque<MSG> posted;
	std::map<WPARAM, Win32PendingTask> tasks;
	WPARAM nextTaskId = 1;
	std::vector<Win32Timer> timers;
	std::vector<std::unique_ptr<Win32Hook>> hooks;
	Win32FramePump* framePump = nullptr;
	bool quitPosted = false;
	int quitCode = 0;
	DWORD lastMessageTime = 0;
	UINT_PTR nextThreadTimerId = 0x7FFF;
};

Win32MessageState& state()
{
	static Win32MessageState messageState;
	return messageState;
}

void fillMessage(MSG& message, HWND hwnd, UINT id, WPARAM wParam, LPARAM lParam)
{
	message.hwnd = hwnd;
	message.message = id;
	message.wParam = wParam;
	message.lParam = lParam;
	message.time = Win32CurrentTime();
	GetCursorPos(&message.pt);
}

bool takePostedMessage(LPMSG message, HWND hwnd, UINT filterMin, UINT filterMax, bool remove)
{
	std::deque<MSG>& posted = state().posted;
	for (auto it = posted.begin(); it != posted.end(); ++it)
	{
		if (!Win32MatchesMessageFilter(*it, hwnd, filterMin, filterMax))
		{
			continue;
		}
		*message = *it;
		if (remove)
		{
			posted.erase(it);
		}
		return true;
	}
	return false;
}

Win32Window* findWindowNeedingPaint(Win32Window* window)
{
	if (!window->isVisible())
	{
		return nullptr;
	}
	if (window->needsPaint)
	{
		return window;
	}
	for (Win32Window* child : window->children)
	{
		Win32Window* found = findWindowNeedingPaint(child);
		if (found != nullptr)
		{
			return found;
		}
	}
	return nullptr;
}

bool takePaintMessage(LPMSG message, HWND hwnd, UINT filterMin, UINT filterMax)
{
	MSG paint;
	fillMessage(paint, hwnd, WM_PAINT, 0, 0);
	if (!Win32MatchesMessageFilter(paint, nullptr, filterMin, filterMax))
	{
		return false;
	}

	const std::vector<Win32Window*> topLevel = Win32WindowManager::instance().topLevelWindows();
	for (Win32Window* window : topLevel)
	{
		Win32Window* found = findWindowNeedingPaint(window);
		if (found == nullptr || (hwnd != nullptr && found->hwnd != hwnd))
		{
			continue;
		}
		paint.hwnd = found->hwnd;
		*message = paint;
		return true;
	}
	return false;
}

bool takeTimerMessage(LPMSG message, HWND hwnd, UINT filterMin, UINT filterMax, bool remove)
{
	const DWORD now = Win32CurrentTime();
	for (Win32Timer& timer : state().timers)
	{
		if (static_cast<int>(now - timer.dueTime) < 0)
		{
			continue;
		}

		MSG tick;
		fillMessage(tick, timer.hwnd, WM_TIMER, timer.id, reinterpret_cast<LPARAM>(timer.timerProc));
		if (!Win32MatchesMessageFilter(tick, hwnd, filterMin, filterMax))
		{
			continue;
		}
		if (remove)
		{
			timer.dueTime = now + timer.elapse;
		}
		*message = tick;
		return true;
	}
	return false;
}

bool takeQuitMessage(LPMSG message, UINT filterMin, UINT filterMax, bool remove)
{
	if (!state().quitPosted)
	{
		return false;
	}

	MSG quit;
	fillMessage(quit, nullptr, WM_QUIT, static_cast<WPARAM>(state().quitCode), 0);
	if (!Win32MatchesMessageFilter(quit, nullptr, filterMin, filterMax))
	{
		return false;
	}
	if (remove)
	{
		state().quitPosted = false;
	}
	*message = quit;
	return true;
}

LRESULT sendPaintToControl(Win32Window* window, WPARAM wParam, LPARAM lParam)
{
	const HWND hwnd = window->hwnd;
	window->controlReceivedPaint = false;
	const LRESULT result = window->windowProc(hwnd, WM_PAINT, wParam, lParam);
	Win32Window* painted = Win32FindWindow(hwnd);
	if (painted != nullptr)
	{
		painted->controlPaints = painted->controlReceivedPaint;
	}
	return result;
}

LRESULT runTask(WPARAM taskId)
{
	const auto found = state().tasks.find(taskId);
	if (found == state().tasks.end())
	{
		return 0;
	}
	const Win32Task task = std::move(found->second.task);
	state().tasks.erase(found);
	task();
	return 0;
}

}

void Win32SetFramePump(Win32FramePump* pump)
{
	state().framePump = pump;
}

bool Win32MatchesMessageFilter(const MSG& message, HWND hwnd, UINT filterMin, UINT filterMax)
{
	if (hwnd != nullptr && message.hwnd != hwnd)
	{
		return false;
	}
	if (filterMin == 0 && filterMax == 0)
	{
		return true;
	}
	return message.message >= filterMin && message.message <= filterMax;
}

DWORD Win32CurrentTime()
{
	using namespace std::chrono;
	static const steady_clock::time_point start = steady_clock::now();
	return static_cast<DWORD>(duration_cast<milliseconds>(steady_clock::now() - start).count());
}

void Win32PostTask(HWND hwnd, Win32Task task)
{
	const WPARAM taskId = state().nextTaskId++;
	state().tasks[taskId] = Win32PendingTask{ hwnd, std::move(task) };
	if (!PostMessageA(hwnd, WM_WB_TASK, taskId, 0))
	{
		state().tasks.erase(taskId);
	}
}

void Win32RemoveWindowMessages(Win32Window* window)
{
	std::deque<MSG>& posted = state().posted;
	for (auto it = posted.begin(); it != posted.end();)
	{
		it = it->hwnd == window->hwnd ? posted.erase(it) : it + 1;
	}

	std::map<WPARAM, Win32PendingTask>& tasks = state().tasks;
	for (auto it = tasks.begin(); it != tasks.end();)
	{
		it = it->second.hwnd == window->hwnd ? tasks.erase(it) : std::next(it);
	}
}

void Win32KillWindowTimers(Win32Window* window)
{
	std::vector<Win32Timer>& timers = state().timers;
	for (auto it = timers.begin(); it != timers.end();)
	{
		it = it->hwnd == window->hwnd ? timers.erase(it) : it + 1;
	}
}

void Win32InvalidateTree(Win32Window* window, bool erase)
{
	InvalidateRect(window->hwnd, nullptr, erase ? TRUE : FALSE);
	for (Win32Window* child : window->children)
	{
		Win32InvalidateTree(child, erase);
	}
}

LRESULT Win32CallCbtHooks(int code, WPARAM wParam, LPARAM lParam)
{
	for (const std::unique_ptr<Win32Hook>& hook : state().hooks)
	{
		if (hook->type != WH_CBT)
		{
			continue;
		}
		const LRESULT result = hook->hookProc(code, wParam, lParam);
		if (result != 0)
		{
			return result;
		}
	}
	return 0;
}

HHOOK SetWindowsHookExA(int hookType, HOOKPROC hookProc, HINSTANCE, DWORD)
{
	if (hookType != WH_CBT || hookProc == nullptr)
	{
		return nullptr;
	}

	auto hook = std::make_unique<Win32Hook>();
	hook->type = hookType;
	hook->hookProc = hookProc;
	HHOOK handle = hook.get();
	state().hooks.insert(state().hooks.begin(), std::move(hook));
	return handle;
}

BOOL UnhookWindowsHookEx(HHOOK hook)
{
	std::vector<std::unique_ptr<Win32Hook>>& hooks = state().hooks;
	for (auto it = hooks.begin(); it != hooks.end(); ++it)
	{
		if (it->get() == hook)
		{
			hooks.erase(it);
			return TRUE;
		}
	}
	return FALSE;
}

LRESULT CallNextHookEx(HHOOK, int, WPARAM, LPARAM)
{
	return 0;
}

LRESULT SendMessageA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr || window->windowProc == nullptr)
	{
		return 0;
	}
	if (message != WM_PAINT || window->control == nullptr)
	{
		return window->windowProc(hwnd, message, wParam, lParam);
	}
	return sendPaintToControl(window, wParam, lParam);
}

BOOL PostMessageA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (hwnd != nullptr && Win32FindWindow(hwnd) == nullptr)
	{
		return FALSE;
	}

	MSG posted;
	fillMessage(posted, hwnd, message, wParam, lParam);
	state().posted.push_back(posted);
	return TRUE;
}

void PostQuitMessage(int exitCode)
{
	state().quitPosted = true;
	state().quitCode = exitCode;
}

BOOL PeekMessageA(LPMSG message, HWND hwnd, UINT filterMin, UINT filterMax, UINT removeFlags)
{
	const bool remove = (removeFlags & PM_REMOVE) != 0;
	const bool found = takePostedMessage(message, hwnd, filterMin, filterMax, remove)
		|| Win32InputQueue::take(message, hwnd, filterMin, filterMax, remove)
		|| takeQuitMessage(message, filterMin, filterMax, remove)
		|| takePaintMessage(message, hwnd, filterMin, filterMax)
		|| takeTimerMessage(message, hwnd, filterMin, filterMax, remove);
	if (found && remove)
	{
		state().lastMessageTime = message->time;
	}
	return found ? TRUE : FALSE;
}

BOOL GetMessageA(LPMSG message, HWND hwnd, UINT filterMin, UINT filterMax)
{
	while (!PeekMessageA(message, hwnd, filterMin, filterMax, PM_REMOVE))
	{
		WaitMessage();
	}
	return message->message != WM_QUIT ? TRUE : FALSE;
}

BOOL WaitMessage()
{
	if (state().framePump == nullptr)
	{
		return FALSE;
	}
	state().framePump->runFrame();
	return TRUE;
}

DWORD GetMessageTime()
{
	return state().lastMessageTime;
}

BOOL TranslateMessage(const MSG* message)
{
	if (message == nullptr || message->message != WM_KEYDOWN)
	{
		return FALSE;
	}

	char symbol = 0;
	switch (message->wParam)
	{
	case VK_RETURN:
		symbol = '\r';
		break;
	case VK_TAB:
		symbol = '\t';
		break;
	case VK_ESCAPE:
		symbol = 0x1B;
		break;
	case VK_BACK:
		symbol = 0x08;
		break;
	default:
		return FALSE;
	}

	MSG character;
	fillMessage(character, message->hwnd, WM_CHAR, static_cast<WPARAM>(symbol), message->lParam);
	state().posted.push_front(character);
	return TRUE;
}

LRESULT DispatchMessageA(const MSG* message)
{
	if (message->message == WM_WB_TASK)
	{
		return runTask(message->wParam);
	}
	if (message->message == WM_TIMER && message->lParam != 0)
	{
		TIMERPROC timerProc = reinterpret_cast<TIMERPROC>(message->lParam);
		timerProc(message->hwnd, WM_TIMER, message->wParam, message->time);
		return 0;
	}
	return SendMessageA(message->hwnd, message->message, message->wParam, message->lParam);
}

LRESULT CallWindowProcA(WNDPROC windowProc, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return windowProc != nullptr ? windowProc(hwnd, message, wParam, lParam) : 0;
}

UINT_PTR SetTimer(HWND hwnd, UINT_PTR id, UINT elapse, TIMERPROC timerProc)
{
	if (hwnd != nullptr && Win32FindWindow(hwnd) == nullptr)
	{
		return 0;
	}

	const UINT_PTR timerId = hwnd != nullptr ? id : state().nextThreadTimerId++;
	KillTimer(hwnd, timerId);

	Win32Timer timer;
	timer.hwnd = hwnd;
	timer.id = timerId;
	timer.elapse = elapse > 0 ? elapse : 1;
	timer.timerProc = timerProc;
	timer.dueTime = Win32CurrentTime() + timer.elapse;
	state().timers.push_back(timer);
	return hwnd != nullptr ? (timerId != 0 ? timerId : 1) : timerId;
}

BOOL KillTimer(HWND hwnd, UINT_PTR id)
{
	std::vector<Win32Timer>& timers = state().timers;
	for (auto it = timers.begin(); it != timers.end(); ++it)
	{
		if (it->hwnd == hwnd && it->id == id)
		{
			timers.erase(it);
			return TRUE;
		}
	}
	return FALSE;
}
