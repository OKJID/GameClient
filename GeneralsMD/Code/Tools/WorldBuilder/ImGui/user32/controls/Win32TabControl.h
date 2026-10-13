#pragma once

#include "Win32Control.h"

#include <string>
#include <vector>

struct Win32Tab
{
	std::string text;
	LPARAM lParam = 0;
	int image = -1;
	RECT rect = {};
};

class Win32TabControl : public Win32Control
{
public:
	explicit Win32TabControl(Win32Window& window) : Win32Control(window) {}

	LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
	void draw(const Win32DrawContext& context) override;

private:
	bool isValid(int index) const { return index >= 0 && index < static_cast<int>(m_tabs.size()); }
	int stripHeight() const;
	void layoutTabs();

	int insertTab(int index, const TCITEMA* item);
	BOOL getTab(int index, TCITEMA* item) const;
	BOOL setTab(int index, const TCITEMA* item);
	BOOL deleteTab(int index);
	int setSelection(int index);
	void selectByUser(int index);
	int hitTest(TCHITTESTINFO* info) const;
	BOOL adjustRect(bool toWindow, RECT* rect) const;
	LRESULT handleTabMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);

	std::vector<Win32Tab> m_tabs;
	int m_selection = -1;
	HIMAGELIST m_imageList = nullptr;
};
