#pragma once

#include "Win32Control.h"
#include "Win32Gdi.h"

#include <string>
#include <vector>

class Win32Toolbar : public Win32Control
{
public:
	explicit Win32Toolbar(Win32Window& window) : Win32Control(window) {}

	LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
	void draw(const Win32DrawContext& context) override;
	bool drawsOwnFocus() const override { return false; }

private:
	struct ImageBlock
	{
		TBADDBITMAP source = {};
		Win32ImagePtr image;
		int firstIndex = 0;
		int count = 0;
	};

	bool isSeparator(const TBBUTTON& button) const { return (button.fsStyle & TBSTYLE_SEP) != 0; }
	bool isVisible(const TBBUTTON& button) const { return (button.fsState & TBSTATE_HIDDEN) == 0; }
	int buttonCount() const { return static_cast<int>(m_buttons.size()); }
	int indexOfCommand(WPARAM commandId) const;
	TBBUTTON* buttonByCommand(WPARAM commandId);

	LRESULT handleButtonMessage(UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT handleStateMessage(UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT handleLayoutMessage(UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT handleMouse(UINT message, LPARAM lParam);
	LRESULT setState(WPARAM commandId, BYTE mask, bool isSet);
	LRESULT queryState(WPARAM commandId, BYTE mask) const;
	LRESULT addBitmap(int imageCount, const TBADDBITMAP* source);
	LRESULT replaceBitmap(const TBREPLACEBITMAP* replacement);
	HWND notifyWindow() const { return m_notifyWindow != nullptr ? m_notifyWindow : parentHwnd(); }
	LRESULT insertButtons(int index, int count, const TBBUTTON* buttons);
	LRESULT deleteButton(int index);
	LRESULT getButtonInfo(WPARAM commandId, TBBUTTONINFOA* info) const;
	LRESULT setButtonInfo(WPARAM commandId, const TBBUTTONINFOA* info);
	void autoSize();

	int separatorWidth(const TBBUTTON& button) const;
	RECT itemRect(int index) const;
	int itemAt(POINT point) const;
	SIZE contentSize() const;
	void pressItem(int index);
	void trackPress(int index);
	void releaseItem(POINT point);
	void toggleCheck(int index);
	void updateHotItem(int index);
	std::string requestToolTipText(const TBBUTTON& button) const;

	void drawButton(const Win32DrawContext& context, int index);
	void drawImage(const Win32DrawContext& context, const TBBUTTON& button, ImVec2 min, ImVec2 max);
	void drawToolTip(const Win32DrawContext& context);

	std::vector<TBBUTTON> m_buttons;
	std::vector<ImageBlock> m_imageBlocks;
	int m_imageCount = 0;
	SIZE m_buttonSize = { 24, 22 };
	SIZE m_bitmapSize = { 16, 15 };
	DWORD m_extendedStyle = 0;
	HWND m_notifyWindow = nullptr;
	int m_hotItem = -1;
	double m_hotSince = 0.0;
	int m_pressedItem = -1;
	std::string m_toolTipText;
};
