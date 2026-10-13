#pragma once

#include "Win32Control.h"

class Win32Trackbar : public Win32Control
{
public:
	explicit Win32Trackbar(Win32Window& window) : Win32Control(window) {}

	LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
	void draw(const Win32DrawContext& context) override;

private:
	bool isVertical() const { return (style() & TBS_VERT) != 0; }
	int clampPosition(int position) const;
	void setRange(int minimum, int maximum, bool redraw);
	void moveByUser(int position, WORD code);
	void notifyScroll(WORD code);
	LRESULT handleKey(WPARAM key);
	LRESULT handleSelectionMessage(UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT handleMouse(UINT message, LPARAM lParam);

	int length() const;
	int thumbOffset() const;
	int positionAt(int offset) const;
	int offsetAlong(LPARAM point) const;
	RECT thumbRect() const;

	int m_minimum = 0;
	int m_maximum = 100;
	int m_position = 0;
	int m_lineSize = 1;
	int m_pageSize = 20;
	int m_tickFrequency = 1;
	int m_selectionStart = 0;
	int m_selectionEnd = 0;
	bool m_isTracking = false;
};

class Win32Progress : public Win32Control
{
public:
	explicit Win32Progress(Win32Window& window) : Win32Control(window) {}

	LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
	void draw(const Win32DrawContext& context) override;
	bool drawsOwnFocus() const override { return false; }

private:
	int setPosition(int position);

	int m_minimum = 0;
	int m_maximum = 100;
	int m_position = 0;
	int m_step = 10;
	COLORREF m_barColor = CLR_DEFAULT;
};
