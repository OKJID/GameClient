#pragma once

#include "Win32Api.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class Win32Control;
class Win32DisplayList;
struct Win32DialogState;
struct Win32WindowClass;

enum class Win32Presentation
{
	Inline,
	OwnWindow,
	Host,
};

struct Win32ScrollState
{
	int minimum = 0;
	int maximum = 0;
	UINT page = 0;
	int position = 0;
	int trackPosition = 0;
	bool hasInfo = false;
};

struct Win32Window
{
	HWND hwnd = nullptr;
	Win32WindowClass* windowClass = nullptr;
	DWORD style = 0;
	DWORD exStyle = 0;
	UINT_PTR id = 0;
	HMENU menu = nullptr;
	std::string text;
	HFONT font = nullptr;
	HINSTANCE instance = nullptr;
	RECT rect = {};
	RECT clientRect = {};
	Win32Window* parent = nullptr;
	Win32Window* owner = nullptr;
	Win32Window* lastActivePopup = nullptr;
	std::vector<Win32Window*> children;
	WNDPROC windowProc = nullptr;
	LONG_PTR userData = 0;
	std::vector<LONG_PTR> extraWords;
	RECT updateRect = {};
	Win32ScrollState horizontalScroll;
	Win32ScrollState verticalScroll;
	bool needsPaint = false;
	bool needsErase = false;
	bool needsNonClientPaint = false;
	bool isDestroying = false;
	bool isHiddenByOwner = false;
	bool controlPaints = true;
	bool controlReceivedPaint = false;
	bool presentsNativeSurface = false;
	Win32Presentation presentation = Win32Presentation::Inline;
	std::unique_ptr<Win32Control> control;
	std::unique_ptr<Win32DisplayList> displayList;
	std::unique_ptr<Win32DisplayList> nonClientDisplayList;
	std::unique_ptr<Win32DialogState> dialogState;

	bool isChild() const { return (style & WS_CHILD) != 0; }
	bool isVisible() const { return (style & WS_VISIBLE) != 0; }
	bool isEnabled() const { return (style & WS_DISABLED) == 0; }
	int width() const { return rect.right - rect.left; }
	int height() const { return rect.bottom - rect.top; }
};

struct Win32NonClientMetrics
{
	int captionHeight = 0;
	int borderWidth = 0;
	int menuHeight = 0;
};

class Win32WindowManager
{
public:
	static Win32WindowManager& instance();

	Win32Window* find(HWND hwnd) const;
	Win32Window* createWindow(Win32WindowClass* windowClass, DWORD exStyle, DWORD style, Win32Window* parent, Win32Window* owner);
	void releaseWindow(Win32Window* window);
	const std::vector<Win32Window*>& topLevelWindows() const { return m_topLevel; }

	void attachToParent(Win32Window* window, Win32Window* parent);
	void detachFromParent(Win32Window* window);
	void bringToTop(Win32Window* window);
	void placeBehind(Win32Window* window, Win32Window* insertAfter);
	void placeAtBottom(Win32Window* window);

	Win32Window* desktop();
	bool isDesktop(const Win32Window* window) const { return window != nullptr && window == m_desktop; }

	Win32Window* hostWindow() const { return m_hostWindow; }
	void setHostWindow(Win32Window* window);
	SIZE hostSize() const { return m_hostSize; }
	void setHostSize(int width, int height);

	Win32NonClientMetrics nonClientMetrics(const Win32Window* window) const;
	RECT clientRectInWindow(const Win32Window* window) const;
	POINT clientOriginOnScreen(const Win32Window* window) const;
	RECT clientRectOnScreen(const Win32Window* window) const;
	RECT windowRectOnScreen(const Win32Window* window) const;

	Win32Window* focus() const { return m_focus; }
	void setFocusWindow(Win32Window* window) { m_focus = window; }
	Win32Window* capture() const { return m_capture; }
	void setCaptureWindow(Win32Window* window) { m_capture = window; }
	Win32Window* active() const { return m_active; }
	void setActiveWindow(Win32Window* window) { m_active = window; }

	void forgetWindow(Win32Window* window);

private:
	std::vector<Win32Window*>& siblingsOf(Win32Window* window);

	std::map<HWND, std::unique_ptr<Win32Window>> m_windows;
	std::vector<Win32Window*> m_topLevel;
	uintptr_t m_nextHandle = 0x10010;
	Win32Window* m_desktop = nullptr;
	Win32Window* m_hostWindow = nullptr;
	SIZE m_hostSize = {};
	Win32Window* m_focus = nullptr;
	Win32Window* m_capture = nullptr;
	Win32Window* m_active = nullptr;
};

Win32NonClientMetrics Win32NonClientMetricsFor(DWORD style, DWORD exStyle, bool isHost);
typedef bool (*Win32HostWindowPolicy)(const CREATESTRUCTA& create);

void Win32RecalculateClientRect(Win32Window* window);
void Win32SetHostWindowPolicy(Win32HostWindowPolicy policy);
void Win32ApplyHostWindowPolicy(Win32Window* window, const CREATESTRUCTA& create);
bool Win32IsHostWindow(HWND hwnd);
void Win32SetNativeSurface(HWND hwnd);
Win32Window* Win32FindWindow(HWND hwnd);
Win32Window* Win32TopLevelOf(Win32Window* window);
bool Win32IsInputBlocked(const Win32Window* window);
int Win32CopyWindowText(const std::string& text, LPSTR buffer, int maxCount);
