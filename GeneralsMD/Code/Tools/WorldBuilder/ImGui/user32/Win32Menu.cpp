#include "Win32Menu.h"

#include "Win32Resources.h"
#include "Win32Window.h"

#include <map>
#include <memory>

namespace
{

struct MenuState
{
	std::map<HMENU, std::unique_ptr<Win32Menu>> menus;
	uintptr_t nextHandle = 0x30010;
	Win32PopupMenuSession session;
};

MenuState& state()
{
	static MenuState menuState;
	return menuState;
}

HMENU registerMenu(bool isPopup)
{
	const HMENU handle = reinterpret_cast<HMENU>(state().nextHandle);
	state().nextHandle += 0x10;
	auto menu = std::make_unique<Win32Menu>();
	menu->isPopup = isPopup;
	state().menus[handle] = std::move(menu);
	return handle;
}

Win32MenuItem makeItem(UINT flags, UINT_PTR idOrPopup, LPCSTR text)
{
	Win32MenuItem item;
	item.flags = flags & ~(MF_BYPOSITION | MF_POPUP | MF_CHANGE | MF_APPEND | MF_DELETE | MF_REMOVE);
	if ((flags & MF_POPUP) != 0)
	{
		item.popup = reinterpret_cast<HMENU>(idOrPopup);
	}
	else
	{
		item.id = static_cast<UINT>(idOrPopup);
	}
	const bool hasText = (flags & (MF_SEPARATOR | MF_BITMAP | MF_OWNERDRAW)) == 0;
	item.text = hasText && text != nullptr ? text : "";
	return item;
}

bool locate(HMENU menuHandle, UINT position, UINT flags, Win32Menu*& owner, size_t& index)
{
	Win32Menu* menu = Win32Menus::find(menuHandle);
	if (menu == nullptr)
	{
		return false;
	}
	if ((flags & MF_BYPOSITION) != 0)
	{
		owner = menu;
		index = position;
		return position < menu->items.size();
	}

	for (size_t candidate = 0; candidate < menu->items.size(); ++candidate)
	{
		const Win32MenuItem& item = menu->items[candidate];
		if (!item.isPopup() && !item.isSeparator() && item.id == position)
		{
			owner = menu;
			index = candidate;
			return true;
		}
		if (item.isPopup() && locate(item.popup, position, flags, owner, index))
		{
			return true;
		}
	}
	return false;
}

void buildFromTemplate(HMENU menu, const std::vector<Win32MenuTemplateItem>& items)
{
	for (const Win32MenuTemplateItem& item : items)
	{
		if (item.isSeparator)
		{
			AppendMenuA(menu, MF_SEPARATOR, 0, nullptr);
			continue;
		}
		if (item.isPopup)
		{
			const HMENU popup = CreatePopupMenu();
			buildFromTemplate(popup, item.children);
			AppendMenuA(menu, MF_POPUP | item.flags, reinterpret_cast<UINT_PTR>(popup), item.text.c_str());
			continue;
		}
		AppendMenuA(menu, MF_STRING | item.flags, item.id, item.text.c_str());
	}
}

UINT replaceFlags(Win32MenuItem& item, UINT mask, UINT value)
{
	const UINT previous = item.flags & mask;
	item.flags = (item.flags & ~mask) | (value & mask);
	return previous;
}

void runPopupLoop(Win32PopupMenuSession& session)
{
	MSG message;
	while (session.isOpen && GetMessageA(&message, nullptr, 0, 0))
	{
		if (message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE)
		{
			Win32Menus::finishPopupSession(0);
			continue;
		}
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}
	if (message.message == WM_QUIT)
	{
		PostQuitMessage(static_cast<int>(message.wParam));
	}
}

}

namespace Win32Menus
{

Win32Menu* find(HMENU menu)
{
	const auto it = state().menus.find(menu);
	return it == state().menus.end() ? nullptr : it->second.get();
}

Win32MenuItem* findItem(HMENU menu, UINT position, UINT flags)
{
	Win32Menu* owner = nullptr;
	size_t index = 0;
	return locate(menu, position, flags, owner, index) ? &owner->items[index] : nullptr;
}

Win32PopupMenuSession* activePopupSession()
{
	return state().session.isOpen ? &state().session : nullptr;
}

void finishPopupSession(UINT chosenId)
{
	state().session.chosenId = chosenId;
	state().session.isOpen = false;
	PostMessageA(nullptr, WM_NULL, 0, 0);
}

void sendInitMenuPopups(HWND owner, HMENU menu)
{
	Win32Menu* found = find(menu);
	if (found == nullptr)
	{
		return;
	}
	for (size_t index = 0; index < found->items.size(); ++index)
	{
		const HMENU popup = found->items[index].popup;
		if (popup == nullptr)
		{
			continue;
		}
		SendMessageA(owner, WM_INITMENUPOPUP, reinterpret_cast<WPARAM>(popup), MAKELPARAM(index, FALSE));
		sendInitMenuPopups(owner, popup);
	}
}

}

HMENU CreateMenu()
{
	return registerMenu(false);
}

HMENU CreatePopupMenu()
{
	return registerMenu(true);
}

BOOL DestroyMenu(HMENU menu)
{
	Win32Menu* found = Win32Menus::find(menu);
	if (found == nullptr)
	{
		return FALSE;
	}
	const std::vector<Win32MenuItem> items = found->items;
	for (const Win32MenuItem& item : items)
	{
		if (item.isPopup())
		{
			DestroyMenu(item.popup);
		}
	}
	state().menus.erase(menu);
	return TRUE;
}

HMENU LoadMenuA(HINSTANCE, LPCSTR menuName)
{
	UINT id = 0;
	if (!Win32Resources::instance().resolveName(menuName, id))
	{
		return nullptr;
	}
	const Win32MenuTemplateItem* menuTemplate = Win32Resources::instance().menu(id);
	if (menuTemplate == nullptr)
	{
		return nullptr;
	}

	const HMENU menu = CreateMenu();
	buildFromTemplate(menu, menuTemplate->children);
	return menu;
}

HMENU GetMenu(HWND hwnd)
{
	const Win32Window* window = Win32FindWindow(hwnd);
	return window != nullptr && !window->isChild() ? window->menu : nullptr;
}

BOOL SetMenu(HWND hwnd, HMENU menu)
{
	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr || window->isChild())
	{
		return FALSE;
	}
	window->menu = menu;
	return TRUE;
}

BOOL DrawMenuBar(HWND hwnd)
{
	return Win32FindWindow(hwnd) != nullptr ? TRUE : FALSE;
}

HMENU GetSubMenu(HMENU menu, int position)
{
	Win32Menu* found = Win32Menus::find(menu);
	if (found == nullptr || position < 0 || static_cast<size_t>(position) >= found->items.size())
	{
		return nullptr;
	}
	return found->items[static_cast<size_t>(position)].popup;
}

int GetMenuItemCount(HMENU menu)
{
	Win32Menu* found = Win32Menus::find(menu);
	return found != nullptr ? static_cast<int>(found->items.size()) : -1;
}

UINT GetMenuItemID(HMENU menu, int position)
{
	Win32MenuItem* item = position >= 0 ? Win32Menus::findItem(menu, static_cast<UINT>(position), MF_BYPOSITION) : nullptr;
	return item != nullptr && !item->isPopup() ? item->id : static_cast<UINT>(-1);
}

UINT GetMenuState(HMENU menu, UINT id, UINT flags)
{
	Win32MenuItem* item = Win32Menus::findItem(menu, id, flags);
	if (item == nullptr)
	{
		return static_cast<UINT>(-1);
	}
	if (!item->isPopup())
	{
		return item->flags;
	}
	const UINT count = static_cast<UINT>(GetMenuItemCount(item->popup));
	return (count << 8) | (item->flags & 0xFF) | MF_POPUP;
}

int GetMenuStringA(HMENU menu, UINT id, LPSTR text, int maxCount, UINT flags)
{
	Win32MenuItem* item = Win32Menus::findItem(menu, id, flags);
	if (item == nullptr)
	{
		return 0;
	}
	if (text == nullptr || maxCount <= 0)
	{
		return static_cast<int>(item->text.size());
	}
	return Win32CopyWindowText(item->text, text, maxCount);
}

BOOL AppendMenuA(HMENU menu, UINT flags, UINT_PTR idOrPopup, LPCSTR text)
{
	Win32Menu* found = Win32Menus::find(menu);
	if (found == nullptr)
	{
		return FALSE;
	}
	found->items.push_back(makeItem(flags, idOrPopup, text));
	return TRUE;
}

BOOL InsertMenuA(HMENU menu, UINT position, UINT flags, UINT_PTR idOrPopup, LPCSTR text)
{
	Win32Menu* owner = nullptr;
	size_t index = 0;
	const bool appends = (flags & MF_BYPOSITION) != 0 && position == static_cast<UINT>(-1);
	if (appends || !locate(menu, position, flags, owner, index))
	{
		return appends || (flags & MF_BYPOSITION) != 0 ? AppendMenuA(menu, flags, idOrPopup, text) : FALSE;
	}
	owner->items.insert(owner->items.begin() + static_cast<long>(index), makeItem(flags, idOrPopup, text));
	return TRUE;
}

BOOL ModifyMenuA(HMENU menu, UINT position, UINT flags, UINT_PTR idOrPopup, LPCSTR text)
{
	Win32MenuItem* item = Win32Menus::findItem(menu, position, flags);
	if (item == nullptr)
	{
		return FALSE;
	}
	*item = makeItem(flags, idOrPopup, text);
	return TRUE;
}

BOOL RemoveMenu(HMENU menu, UINT position, UINT flags)
{
	Win32Menu* owner = nullptr;
	size_t index = 0;
	if (!locate(menu, position, flags, owner, index))
	{
		return FALSE;
	}
	owner->items.erase(owner->items.begin() + static_cast<long>(index));
	return TRUE;
}

BOOL DeleteMenu(HMENU menu, UINT position, UINT flags)
{
	Win32MenuItem* item = Win32Menus::findItem(menu, position, flags);
	if (item == nullptr)
	{
		return FALSE;
	}
	const HMENU popup = item->popup;
	RemoveMenu(menu, position, flags);
	if (popup != nullptr)
	{
		DestroyMenu(popup);
	}
	return TRUE;
}

DWORD CheckMenuItem(HMENU menu, UINT id, UINT check)
{
	Win32MenuItem* item = Win32Menus::findItem(menu, id, check);
	return item != nullptr ? replaceFlags(*item, MF_CHECKED, check) : static_cast<DWORD>(-1);
}

BOOL EnableMenuItem(HMENU menu, UINT id, UINT enable)
{
	Win32MenuItem* item = Win32Menus::findItem(menu, id, enable);
	return item != nullptr ? static_cast<BOOL>(replaceFlags(*item, MF_GRAYED | MF_DISABLED, enable)) : -1;
}

BOOL CheckMenuRadioItem(HMENU menu, UINT first, UINT last, UINT check, UINT flags)
{
	bool found = false;
	for (UINT position = first; position <= last; ++position)
	{
		Win32MenuItem* item = Win32Menus::findItem(menu, position, flags);
		if (item == nullptr)
		{
			continue;
		}
		replaceFlags(*item, MF_CHECKED, position == check ? MF_CHECKED : MF_UNCHECKED);
		found = found || position == check;
	}
	return found ? TRUE : FALSE;
}

BOOL TrackPopupMenu(HMENU menu, UINT flags, int x, int y, int, HWND owner, const RECT*)
{
	Win32PopupMenuSession& session = state().session;
	if (session.isOpen || Win32Menus::find(menu) == nullptr || !IsWindow(owner))
	{
		return FALSE;
	}

	session = Win32PopupMenuSession();
	session.menu = menu;
	session.owner = owner;
	session.position.x = x;
	session.position.y = y;
	session.flags = flags;
	session.isOpen = true;
	session.needsOpening = true;

	SendMessageA(owner, WM_ENTERMENULOOP, TRUE, 0);
	SendMessageA(owner, WM_INITMENUPOPUP, reinterpret_cast<WPARAM>(menu), MAKELPARAM(0, FALSE));
	Win32Menus::sendInitMenuPopups(owner, menu);
	runPopupLoop(session);
	SendMessageA(owner, WM_EXITMENULOOP, TRUE, 0);

	const UINT chosen = session.chosenId;
	session = Win32PopupMenuSession();
	if ((flags & TPM_RETURNCMD) != 0)
	{
		return static_cast<BOOL>(chosen);
	}
	if (chosen != 0 && (flags & TPM_NONOTIFY) == 0)
	{
		PostMessageA(owner, WM_COMMAND, MAKEWPARAM(chosen, 0), 0);
	}
	return chosen != 0 ? TRUE : FALSE;
}
