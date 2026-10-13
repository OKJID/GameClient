#include "AfxInternal.h"

IMPLEMENT_DYNCREATE(CMenu, CObject)

CMenu::CMenu() :
	m_hMenu(nullptr)
{
}

CMenu::~CMenu()
{
	DestroyMenu();
}

HMENU CMenu::GetSafeHmenu() const
{
	const CMenu* self = this;
	return self != nullptr ? m_hMenu : nullptr;
}

CMenu* PASCAL CMenu::FromHandle(HMENU hMenu)
{
	return AfxMenuMap().fromHandle(hMenu);
}

void PASCAL CMenu::DeleteTempMap()
{
	AfxMenuMap().deleteTemporaries();
}

BOOL CMenu::Attach(HMENU hMenu)
{
	if (hMenu == nullptr)
	{
		return FALSE;
	}
	m_hMenu = hMenu;
	AfxMenuMap().setPermanent(hMenu, this);
	return TRUE;
}

HMENU CMenu::Detach()
{
	const HMENU hMenu = m_hMenu;
	if (hMenu != nullptr)
	{
		AfxMenuMap().removeHandle(hMenu);
	}
	m_hMenu = nullptr;
	return hMenu;
}

BOOL CMenu::CreateMenu() { return Attach(::CreateMenu()); }
BOOL CMenu::CreatePopupMenu() { return Attach(::CreatePopupMenu()); }
BOOL CMenu::LoadMenu(LPCSTR lpszResourceName) { return Attach(::LoadMenuA(AfxGetResourceHandle(), lpszResourceName)); }
BOOL CMenu::LoadMenu(UINT nIDResource) { return LoadMenu(MAKEINTRESOURCE(nIDResource)); }

BOOL CMenu::DestroyMenu()
{
	if (m_hMenu == nullptr)
	{
		return FALSE;
	}
	return ::DestroyMenu(Detach());
}

BOOL CMenu::DeleteMenu(UINT nPosition, UINT nFlags) { return ::DeleteMenu(m_hMenu, nPosition, nFlags); }
BOOL CMenu::RemoveMenu(UINT nPosition, UINT nFlags) { return ::RemoveMenu(m_hMenu, nPosition, nFlags); }
BOOL CMenu::AppendMenu(UINT nFlags, UINT_PTR nIDNewItem, LPCSTR lpszNewItem) { return ::AppendMenuA(m_hMenu, nFlags, nIDNewItem, lpszNewItem); }

BOOL CMenu::InsertMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem, LPCSTR lpszNewItem)
{
	return ::InsertMenuA(m_hMenu, nPosition, nFlags, nIDNewItem, lpszNewItem);
}

BOOL CMenu::ModifyMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem, LPCSTR lpszNewItem)
{
	return ::ModifyMenuA(m_hMenu, nPosition, nFlags, nIDNewItem, lpszNewItem);
}

UINT CMenu::CheckMenuItem(UINT nIDCheckItem, UINT nCheck) { return static_cast<UINT>(::CheckMenuItem(m_hMenu, nIDCheckItem, nCheck)); }
UINT CMenu::EnableMenuItem(UINT nIDEnableItem, UINT nEnable) { return static_cast<UINT>(::EnableMenuItem(m_hMenu, nIDEnableItem, nEnable)); }

BOOL CMenu::CheckMenuRadioItem(UINT nIDFirst, UINT nIDLast, UINT nIDItem, UINT nFlags)
{
	return ::CheckMenuRadioItem(m_hMenu, nIDFirst, nIDLast, nIDItem, nFlags);
}

UINT CMenu::GetMenuItemCount() const { return static_cast<UINT>(::GetMenuItemCount(m_hMenu)); }
UINT CMenu::GetMenuItemID(int nPos) const { return ::GetMenuItemID(m_hMenu, nPos); }
UINT CMenu::GetMenuState(UINT nID, UINT nFlags) const { return ::GetMenuState(m_hMenu, nID, nFlags); }

int CMenu::GetMenuString(UINT nIDItem, LPSTR lpString, int nMaxCount, UINT nFlags) const
{
	return ::GetMenuStringA(m_hMenu, nIDItem, lpString, nMaxCount, nFlags);
}

int CMenu::GetMenuString(UINT nIDItem, CString& rString, UINT nFlags) const
{
	const int length = ::GetMenuStringA(m_hMenu, nIDItem, nullptr, 0, nFlags);
	::GetMenuStringA(m_hMenu, nIDItem, rString.GetBufferSetLength(length), length + 1, nFlags);
	rString.ReleaseBuffer();
	return rString.GetLength();
}

CMenu* CMenu::GetSubMenu(int nPos) const
{
	return FromHandle(::GetSubMenu(m_hMenu, nPos));
}

BOOL CMenu::TrackPopupMenu(UINT nFlags, int x, int y, CWnd* pWnd, LPCRECT lpRect)
{
	AfxThreadState& state = AfxGetThreadState();
	state.hTrackingWindow = pWnd->GetSafeHwnd();
	state.hTrackingMenu = m_hMenu;
	const BOOL result = ::TrackPopupMenu(m_hMenu, nFlags, x, y, 0, state.hTrackingWindow, lpRect);
	state.hTrackingWindow = nullptr;
	state.hTrackingMenu = nullptr;
	return result;
}
