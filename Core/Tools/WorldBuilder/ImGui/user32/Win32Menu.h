#pragma once

#include "Win32Api.h"

#include <string>
#include <vector>

#define MF_HELP_FLAG 0x00004000L

struct Win32MenuItem
{
	UINT id = 0;
	UINT flags = 0;
	std::string text;
	HMENU popup = nullptr;

	bool isSeparator() const { return (flags & MF_SEPARATOR) != 0; }
	bool isPopup() const { return popup != nullptr; }
	bool isGrayed() const { return (flags & (MF_GRAYED | MF_DISABLED)) != 0; }
	bool isChecked() const { return (flags & MF_CHECKED) != 0; }
};

struct Win32Menu
{
	std::vector<Win32MenuItem> items;
	bool isPopup = false;
};

struct Win32PopupMenuSession
{
	HMENU menu = nullptr;
	HWND owner = nullptr;
	POINT position = {};
	UINT flags = 0;
	UINT chosenId = 0;
	bool isOpen = false;
	bool needsOpening = false;
};

namespace Win32Menus
{
Win32Menu* find(HMENU menu);
Win32MenuItem* findItem(HMENU menu, UINT position, UINT flags);
Win32PopupMenuSession* activePopupSession();
void finishPopupSession(UINT chosenId);
void sendInitMenuPopups(HWND owner, HMENU menu);
}
