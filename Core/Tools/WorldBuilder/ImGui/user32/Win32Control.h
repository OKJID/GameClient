#pragma once

#include "Win32Messages.h"

struct ImDrawList;

struct Win32DrawContext
{
	ImDrawList* drawList = nullptr;
	POINT clientOrigin = {};
	SIZE clientSize = {};
	bool isEnabled = true;
	bool hasFocus = false;
};

class Win32Control
{
public:
	explicit Win32Control(Win32Window& window) : m_window(window) {}
	virtual ~Win32Control() = default;

	virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
	virtual void draw(const Win32DrawContext& context) = 0;
	virtual bool drawsOwnFocus() const { return true; }

	void queueInput(Win32Task action);

protected:
	HWND hwnd() const { return m_window.hwnd; }
	int controlId() const { return static_cast<int>(m_window.id); }
	HWND parentHwnd() const;
	DWORD style() const { return m_window.style; }
	const std::string& text() const { return m_window.text; }
	const char* widgetId() const;

	LRESULT defaultProc(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT eraseWithSystemColor(HDC hdc, int colorIndex);
	void notifyCommand(WORD code);
	LRESULT notifyParent(NMHDR& header, UINT code);
	void takeFocus();

	Win32Window& m_window;

private:
	mutable char m_widgetId[32] = {};
};

LRESULT Win32ControlWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

template<class Control>
std::unique_ptr<Win32Control> Win32CreateControl(Win32Window& window)
{
	return std::make_unique<Control>(window);
}
