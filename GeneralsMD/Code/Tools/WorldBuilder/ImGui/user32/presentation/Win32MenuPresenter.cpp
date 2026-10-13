#include "Win32MenuPresenter.h"

#include "Win32ControlDrawing.h"
#include "Win32Messages.h"
#include "Win32Window.h"

#include "imgui.h"

#include <string>

namespace
{

const char kPopupSessionId[] = "##win32popupmenu";

struct MenuLabel
{
	std::string text;
	std::string shortcut;
};

void sendLater(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Win32PostTask(hwnd, [hwnd, message, wParam, lParam]() { SendMessageA(hwnd, message, wParam, lParam); });
}

MenuLabel parseLabel(const std::string& itemText)
{
	const size_t tab = itemText.find('\t');
	MenuLabel label;
	label.text = Win32ControlDrawing::withoutMnemonic(itemText.substr(0, tab));
	label.shortcut = tab != std::string::npos ? itemText.substr(tab + 1) : "";
	return label;
}

ImVec2 popupPivot(UINT flags)
{
	const float x = (flags & TPM_RIGHTALIGN) != 0 ? 1.0f : ((flags & TPM_CENTERALIGN) != 0 ? 0.5f : 0.0f);
	const float y = (flags & TPM_BOTTOMALIGN) != 0 ? 1.0f : ((flags & TPM_VCENTERALIGN) != 0 ? 0.5f : 0.0f);
	return ImVec2(x, y);
}

}

void Win32MenuPresenter::present()
{
	m_openedThisFrame.clear();
	m_hover = Win32MenuHover();
	presentMenuBar();
	presentPopupSession();
	finishMenuBar();
	finishHover();
	m_openPopups = m_openedThisFrame;
}

void Win32MenuPresenter::presentMenuBar()
{
	const Win32Window* host = Win32WindowManager::instance().hostWindow();
	if (host == nullptr || !host->isVisible() || Win32Menus::find(host->menu) == nullptr || !ImGui::BeginMainMenuBar())
	{
		return;
	}

	const HWND owner = host->hwnd;
	m_menuBarOwner = owner;
	Win32MenuDrawing drawing;
	drawing.owner = owner;
	drawing.rootMenu = host->menu;
	drawing.initializesPopups = true;
	drawing.choose = [owner](UINT id) { PostMessageA(owner, WM_COMMAND, MAKEWPARAM(id, 0), 0); };

	const bool isBlocked = !host->isEnabled() || Win32Menus::activePopupSession() != nullptr;
	if (isBlocked)
	{
		ImGui::BeginDisabled();
	}
	drawItems(host->menu, drawing);
	if (isBlocked)
	{
		ImGui::EndDisabled();
	}
	ImGui::EndMainMenuBar();
}

void Win32MenuPresenter::presentPopupSession()
{
	Win32PopupMenuSession* session = Win32Menus::activePopupSession();
	if (session == nullptr)
	{
		closeStalePopupSession();
		return;
	}

	if (session->needsOpening)
	{
		session->needsOpening = false;
		const ImVec2 position(static_cast<float>(session->position.x), static_cast<float>(session->position.y));
		ImGui::SetNextWindowPos(position, ImGuiCond_Always, popupPivot(session->flags));
		ImGui::OpenPopup(kPopupSessionId);
	}
	if (!ImGui::BeginPopup(kPopupSessionId, ImGuiWindowFlags_NoSavedSettings))
	{
		m_isSessionPopupOpen = false;
		m_lastHover = Win32MenuHover();
		Win32Menus::finishPopupSession(0);
		return;
	}

	m_isSessionPopupOpen = true;
	Win32MenuDrawing drawing;
	drawing.owner = session->owner;
	drawing.rootMenu = session->menu;
	drawing.choose = [](UINT id) { Win32Menus::finishPopupSession(id); };
	drawItems(session->menu, drawing);
	ImGui::EndPopup();
}

void Win32MenuPresenter::closeStalePopupSession()
{
	if (!m_isSessionPopupOpen)
	{
		return;
	}
	m_isSessionPopupOpen = false;
	m_lastHover = Win32MenuHover();
	if (ImGui::BeginPopup(kPopupSessionId))
	{
		ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
}

void Win32MenuPresenter::drawItems(HMENU menu, const Win32MenuDrawing& drawing)
{
	const Win32Menu* found = Win32Menus::find(menu);
	if (found == nullptr)
	{
		return;
	}

	const std::vector<Win32MenuItem> items = found->items;
	for (UINT index = 0; index < items.size(); ++index)
	{
		const Win32MenuItem& item = items[index];
		ImGui::PushID(static_cast<int>(index));
		if (item.isSeparator())
		{
			ImGui::Separator();
		}
		else if (item.isPopup())
		{
			drawPopupItem(menu, index, item, drawing);
		}
		else
		{
			drawCommandItem(menu, item, drawing);
		}
		ImGui::PopID();
	}
}

void Win32MenuPresenter::drawPopupItem(HMENU menu, UINT index, const Win32MenuItem& item, const Win32MenuDrawing& drawing)
{
	const MenuLabel label = parseLabel(item.text);
	const bool isOpen = ImGui::BeginMenu(label.text.c_str(), !item.isGrayed());
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		noteHover(menu, index, MF_POPUP | MF_HILITE | (item.flags & (MF_GRAYED | MF_DISABLED)), drawing.owner);
	}
	if (!isOpen)
	{
		return;
	}
	notePopupOpened(item.popup, index, drawing);
	drawItems(item.popup, drawing);
	ImGui::EndMenu();
}

void Win32MenuPresenter::drawCommandItem(HMENU menu, const Win32MenuItem& item, const Win32MenuDrawing& drawing)
{
	const MenuLabel label = parseLabel(item.text);
	const char* shortcut = label.shortcut.empty() ? nullptr : label.shortcut.c_str();
	const bool isChosen = ImGui::MenuItem(label.text.c_str(), shortcut, item.isChecked(), !item.isGrayed());
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		noteHover(menu, item.id, MF_HILITE | (item.flags & (MF_GRAYED | MF_DISABLED | MF_CHECKED)), drawing.owner);
	}
	if (isChosen)
	{
		drawing.choose(item.id);
	}
}

void Win32MenuPresenter::notePopupOpened(HMENU popup, UINT index, const Win32MenuDrawing& drawing)
{
	m_openedThisFrame.insert(popup);
	if (!drawing.initializesPopups)
	{
		return;
	}

	m_isMenuBarOpenThisFrame = true;
	if (!m_isMenuBarActive)
	{
		m_isMenuBarActive = true;
		sendLater(drawing.owner, WM_ENTERMENULOOP, FALSE, 0);
		sendLater(drawing.owner, WM_INITMENU, reinterpret_cast<WPARAM>(drawing.rootMenu), 0);
	}
	if (m_openPopups.count(popup) == 0)
	{
		sendLater(drawing.owner, WM_INITMENUPOPUP, reinterpret_cast<WPARAM>(popup), MAKELPARAM(index, FALSE));
	}
}

void Win32MenuPresenter::noteHover(HMENU menu, UINT item, UINT flags, HWND owner)
{
	m_hover.owner = owner;
	m_hover.menu = menu;
	m_hover.item = item;
	m_hover.flags = flags;
}

void Win32MenuPresenter::finishMenuBar()
{
	const bool closes = m_isMenuBarActive && !m_isMenuBarOpenThisFrame;
	m_isMenuBarOpenThisFrame = false;
	if (!closes)
	{
		return;
	}
	m_isMenuBarActive = false;
	m_lastHover = Win32MenuHover();
	sendLater(m_menuBarOwner, WM_MENUSELECT, MAKEWPARAM(0, 0xFFFF), 0);
	sendLater(m_menuBarOwner, WM_EXITMENULOOP, FALSE, 0);
}

void Win32MenuPresenter::finishHover()
{
	if (m_hover.owner == nullptr || m_hover == m_lastHover)
	{
		return;
	}
	m_lastHover = m_hover;
	sendLater(m_hover.owner, WM_MENUSELECT, MAKEWPARAM(m_hover.item, m_hover.flags), reinterpret_cast<LPARAM>(m_hover.menu));
}
