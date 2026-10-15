#pragma once

#include "AfxWnd.h"

class CMenu : public CObject
{
	DECLARE_DYNCREATE(CMenu)

public:
	CMenu();
	~CMenu() override;

	HMENU m_hMenu;

	operator HMENU() const { return m_hMenu; }
	HMENU GetSafeHmenu() const;
	BOOL operator==(const CMenu& menu) const { return menu.m_hMenu == m_hMenu; }
	BOOL operator!=(const CMenu& menu) const { return menu.m_hMenu != m_hMenu; }

	static CMenu* PASCAL FromHandle(HMENU hMenu);
	static void PASCAL DeleteTempMap();
	BOOL Attach(HMENU hMenu);
	HMENU Detach();

	BOOL CreateMenu();
	BOOL CreatePopupMenu();
	BOOL LoadMenu(LPCSTR lpszResourceName);
	BOOL LoadMenu(UINT nIDResource);
	BOOL DestroyMenu();

	BOOL DeleteMenu(UINT nPosition, UINT nFlags);
	BOOL RemoveMenu(UINT nPosition, UINT nFlags);
	BOOL AppendMenu(UINT nFlags, UINT_PTR nIDNewItem = 0, LPCSTR lpszNewItem = nullptr);
	BOOL InsertMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem = 0, LPCSTR lpszNewItem = nullptr);
	BOOL ModifyMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem = 0, LPCSTR lpszNewItem = nullptr);
	UINT CheckMenuItem(UINT nIDCheckItem, UINT nCheck);
	UINT EnableMenuItem(UINT nIDEnableItem, UINT nEnable);
	BOOL CheckMenuRadioItem(UINT nIDFirst, UINT nIDLast, UINT nIDItem, UINT nFlags);

	UINT GetMenuItemCount() const;
	UINT GetMenuItemID(int nPos) const;
	UINT GetMenuState(UINT nID, UINT nFlags) const;
	int GetMenuString(UINT nIDItem, LPSTR lpString, int nMaxCount, UINT nFlags) const;
	int GetMenuString(UINT nIDItem, CString& rString, UINT nFlags) const;
	CMenu* GetSubMenu(int nPos) const;
	BOOL TrackPopupMenu(UINT nFlags, int x, int y, CWnd* pWnd, LPCRECT lpRect = nullptr);
};
