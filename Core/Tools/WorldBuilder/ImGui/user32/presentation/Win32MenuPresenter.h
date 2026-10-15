#pragma once

#include "Win32Menu.h"

#include <functional>
#include <set>

struct Win32MenuHover
{
	HWND owner = nullptr;
	HMENU menu = nullptr;
	UINT item = 0;
	UINT flags = 0;

	bool operator==(const Win32MenuHover& other) const
	{
		return owner == other.owner && menu == other.menu && item == other.item && flags == other.flags;
	}
};

struct Win32MenuDrawing
{
	HWND owner = nullptr;
	HMENU rootMenu = nullptr;
	bool initializesPopups = false;
	std::function<void(UINT)> choose;
};

class Win32MenuPresenter
{
public:
	void present();

private:
	void presentMenuBar();
	void presentPopupSession();
	void closeStalePopupSession();
	void drawItems(HMENU menu, const Win32MenuDrawing& drawing);
	void drawPopupItem(HMENU menu, UINT index, const Win32MenuItem& item, const Win32MenuDrawing& drawing);
	void drawCommandItem(HMENU menu, const Win32MenuItem& item, const Win32MenuDrawing& drawing);
	void notePopupOpened(HMENU popup, UINT index, const Win32MenuDrawing& drawing);
	void noteHover(HMENU menu, UINT item, UINT flags, HWND owner);
	void finishMenuBar();
	void finishHover();

	std::set<HMENU> m_openPopups;
	std::set<HMENU> m_openedThisFrame;
	Win32MenuHover m_hover;
	Win32MenuHover m_lastHover;
	HWND m_menuBarOwner = nullptr;
	bool m_isMenuBarActive = false;
	bool m_isMenuBarOpenThisFrame = false;
	bool m_isSessionPopupOpen = false;
};
