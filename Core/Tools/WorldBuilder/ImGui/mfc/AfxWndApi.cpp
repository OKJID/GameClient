#include "AfxInternal.h"

DWORD CWnd::GetStyle() const
{
	return static_cast<DWORD>(::GetWindowLongA(m_hWnd, GWL_STYLE));
}

DWORD CWnd::GetExStyle() const
{
	return static_cast<DWORD>(::GetWindowLongA(m_hWnd, GWL_EXSTYLE));
}

BOOL PASCAL CWnd::ModifyStyle(HWND hWnd, DWORD dwRemove, DWORD dwAdd, UINT nFlags)
{
	const DWORD style = static_cast<DWORD>(::GetWindowLongA(hWnd, GWL_STYLE));
	const DWORD newStyle = (style & ~dwRemove) | dwAdd;
	if (style == newStyle)
	{
		return FALSE;
	}
	::SetWindowLongA(hWnd, GWL_STYLE, static_cast<LONG>(newStyle));
	if (nFlags != 0)
	{
		::SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | nFlags);
	}
	return TRUE;
}

BOOL PASCAL CWnd::ModifyStyleEx(HWND hWnd, DWORD dwRemove, DWORD dwAdd, UINT nFlags)
{
	const DWORD style = static_cast<DWORD>(::GetWindowLongA(hWnd, GWL_EXSTYLE));
	const DWORD newStyle = (style & ~dwRemove) | dwAdd;
	if (style == newStyle)
	{
		return FALSE;
	}
	::SetWindowLongA(hWnd, GWL_EXSTYLE, static_cast<LONG>(newStyle));
	if (nFlags != 0)
	{
		::SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | nFlags);
	}
	return TRUE;
}

BOOL CWnd::ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags)
{
	return ModifyStyle(m_hWnd, dwRemove, dwAdd, nFlags);
}

BOOL CWnd::ModifyStyleEx(DWORD dwRemove, DWORD dwAdd, UINT nFlags)
{
	return ModifyStyleEx(m_hWnd, dwRemove, dwAdd, nFlags);
}

int CWnd::GetDlgCtrlID() const { return ::GetDlgCtrlID(m_hWnd); }
int CWnd::SetDlgCtrlID(int nID) { return static_cast<int>(::SetWindowLongA(m_hWnd, GWL_ID, nID)); }
CWnd* CWnd::GetParent() const { return FromHandle(::GetParent(m_hWnd)); }
CWnd* CWnd::SetParent(CWnd* pWndNewParent) { return FromHandle(::SetParent(m_hWnd, pWndNewParent->GetSafeHwnd())); }
CWnd* CWnd::GetOwner() const { return m_hWndOwner != nullptr ? FromHandle(m_hWndOwner) : GetParent(); }
void CWnd::SetOwner(CWnd* pOwnerWnd) { m_hWndOwner = pOwnerWnd->GetSafeHwnd(); }

CWnd* CWnd::GetTopLevelParent() const
{
	HWND topLevel = m_hWnd;
	for (HWND next = AfxGetParentOwner(topLevel); next != nullptr; next = AfxGetParentOwner(topLevel))
	{
		topLevel = next;
	}
	return FromHandle(topLevel);
}

CWnd* CWnd::GetTopLevelOwner() const
{
	HWND owner = m_hWnd;
	for (HWND next = ::GetWindow(owner, GW_OWNER); next != nullptr; next = ::GetWindow(owner, GW_OWNER))
	{
		owner = next;
	}
	return FromHandle(owner);
}

CFrameWnd* CWnd::GetParentFrame() const
{
	if (GetSafeHwnd() == nullptr)
	{
		return nullptr;
	}
	for (CWnd* parent = GetParent(); parent != nullptr; parent = parent->GetParent())
	{
		if (parent->IsFrameWnd())
		{
			return static_cast<CFrameWnd*>(parent);
		}
	}
	return nullptr;
}

CFrameWnd* CWnd::GetTopLevelFrame() const
{
	CFrameWnd* frame = IsFrameWnd() ? const_cast<CFrameWnd*>(static_cast<const CFrameWnd*>(this)) : GetParentFrame();
	for (CFrameWnd* parent = frame != nullptr ? frame->GetParentFrame() : nullptr; parent != nullptr; parent = parent->GetParentFrame())
	{
		frame = parent;
	}
	return frame;
}

CWnd* CWnd::GetDlgItem(int nID) const { return FromHandle(::GetDlgItem(m_hWnd, nID)); }
void CWnd::GetDlgItem(int nID, HWND* phWnd) const { *phWnd = ::GetDlgItem(m_hWnd, nID); }

CWnd* CWnd::GetDescendantWindow(int nID, BOOL bOnlyPerm) const
{
	const HWND child = ::GetDlgItem(m_hWnd, nID);
	if (child != nullptr)
	{
		CWnd* window = bOnlyPerm ? FromHandlePermanent(child) : FromHandle(child);
		if (window != nullptr)
		{
			return window;
		}
	}
	for (HWND next = ::GetTopWindow(m_hWnd); next != nullptr; next = ::GetWindow(next, GW_HWNDNEXT))
	{
		CWnd* found = FromHandle(next)->GetDescendantWindow(nID, bOnlyPerm);
		if (found != nullptr)
		{
			return found;
		}
	}
	return nullptr;
}

CWnd* CWnd::GetWindow(UINT nCmd) const { return FromHandle(::GetWindow(m_hWnd, nCmd)); }
CWnd* CWnd::GetNextWindow(UINT nFlag) const { return FromHandle(::GetWindow(m_hWnd, nFlag)); }
CWnd* CWnd::GetTopWindow() const { return FromHandle(::GetTopWindow(m_hWnd)); }
CWnd* CWnd::GetLastActivePopup() const { return FromHandle(::GetLastActivePopup(m_hWnd)); }
CWnd* CWnd::ChildWindowFromPoint(POINT point) const { return FromHandle(::ChildWindowFromPoint(m_hWnd, point)); }
BOOL CWnd::IsChild(const CWnd* pWnd) const { return ::IsChild(m_hWnd, pWnd->GetSafeHwnd()); }

CWnd* PASCAL CWnd::GetFocus() { return FromHandle(::GetFocus()); }
CWnd* CWnd::SetFocus() { return FromHandle(::SetFocus(m_hWnd)); }
CWnd* PASCAL CWnd::GetActiveWindow() { return FromHandle(::GetActiveWindow()); }
CWnd* CWnd::SetActiveWindow() { return FromHandle(::SetActiveWindow(m_hWnd)); }
CWnd* PASCAL CWnd::GetCapture() { return FromHandle(::GetCapture()); }
CWnd* CWnd::SetCapture() { return FromHandle(::SetCapture(m_hWnd)); }
CWnd* PASCAL CWnd::GetForegroundWindow() { return FromHandle(::GetForegroundWindow()); }
BOOL CWnd::SetForegroundWindow() { return ::SetForegroundWindow(m_hWnd); }
CWnd* PASCAL CWnd::GetDesktopWindow() { return FromHandle(::GetDesktopWindow()); }
CWnd* PASCAL CWnd::WindowFromPoint(POINT point) { return FromHandle(::WindowFromPoint(point)); }

CWnd* PASCAL CWnd::GetSafeOwner(CWnd* pParent, HWND* pWndTop)
{
	return FromHandle(GetSafeOwner_(pParent->GetSafeHwnd(), pWndTop));
}

HWND PASCAL CWnd::GetSafeOwner_(HWND hParent, HWND* pWndTop)
{
	HWND hWnd = hParent != nullptr ? hParent : AfxGetMainWnd()->GetSafeHwnd();
	while (hWnd != nullptr && (::GetWindowLongA(hWnd, GWL_STYLE) & WS_CHILD) != 0)
	{
		hWnd = ::GetParent(hWnd);
	}

	HWND topLevel = hWnd;
	for (HWND next = topLevel != nullptr ? ::GetParent(topLevel) : nullptr; next != nullptr; next = ::GetParent(topLevel))
	{
		topLevel = next;
	}
	if (hParent == nullptr && hWnd != nullptr)
	{
		hWnd = ::GetLastActivePopup(hWnd);
	}

	if (pWndTop == nullptr)
	{
		return hWnd;
	}
	*pWndTop = nullptr;
	if (topLevel != nullptr && topLevel != hWnd && ::IsWindowEnabled(topLevel))
	{
		*pWndTop = topLevel;
		::EnableWindow(topLevel, FALSE);
	}
	return hWnd;
}

void CWnd::SetWindowText(LPCSTR lpszString) { ::SetWindowTextA(m_hWnd, lpszString); }
int CWnd::GetWindowText(LPSTR lpszStringBuf, int nMaxCount) const { return ::GetWindowTextA(m_hWnd, lpszStringBuf, nMaxCount); }
int CWnd::GetWindowTextLength() const { return ::GetWindowTextLengthA(m_hWnd); }

void CWnd::GetWindowText(CString& rString) const
{
	const int length = ::GetWindowTextLengthA(m_hWnd);
	::GetWindowTextA(m_hWnd, rString.GetBufferSetLength(length), length + 1);
	rString.ReleaseBuffer();
}

void CWnd::SetFont(CFont* pFont, BOOL bRedraw) { SendMessage(WM_SETFONT, reinterpret_cast<WPARAM>(pFont->GetSafeHandle()), bRedraw); }
CFont* CWnd::GetFont() const { return CFont::FromHandle(reinterpret_cast<HFONT>(SendMessage(WM_GETFONT))); }
CMenu* CWnd::GetMenu() const { return CMenu::FromHandle(::GetMenu(m_hWnd)); }
BOOL CWnd::SetMenu(CMenu* pMenu) { return ::SetMenu(m_hWnd, pMenu->GetSafeHmenu()); }
void CWnd::DrawMenuBar() { ::DrawMenuBar(m_hWnd); }

BOOL CWnd::IsIconic() const { return ::IsIconic(m_hWnd); }
BOOL CWnd::IsZoomed() const { return ::IsZoomed(m_hWnd); }
BOOL CWnd::IsWindowVisible() const { return ::IsWindowVisible(m_hWnd); }
BOOL CWnd::IsWindowEnabled() const { return ::IsWindowEnabled(m_hWnd); }
BOOL CWnd::EnableWindow(BOOL bEnable) { return ::EnableWindow(m_hWnd, bEnable); }
BOOL CWnd::ShowWindow(int nCmdShow) { return ::ShowWindow(m_hWnd, nCmdShow); }
void CWnd::MoveWindow(int x, int y, int nWidth, int nHeight, BOOL bRepaint) { ::MoveWindow(m_hWnd, x, y, nWidth, nHeight, bRepaint); }

void CWnd::MoveWindow(LPCRECT lpRect, BOOL bRepaint)
{
	::MoveWindow(m_hWnd, lpRect->left, lpRect->top, lpRect->right - lpRect->left, lpRect->bottom - lpRect->top, bRepaint);
}

BOOL CWnd::SetWindowPos(const CWnd* pWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags)
{
	return ::SetWindowPos(m_hWnd, pWndInsertAfter != nullptr ? pWndInsertAfter->m_hWnd : nullptr, x, y, cx, cy, nFlags);
}

void CWnd::BringWindowToTop() { ::BringWindowToTop(m_hWnd); }
void CWnd::GetWindowRect(LPRECT lpRect) const { ::GetWindowRect(m_hWnd, lpRect); }
void CWnd::GetClientRect(LPRECT lpRect) const { ::GetClientRect(m_hWnd, lpRect); }
void CWnd::ClientToScreen(LPPOINT lpPoint) const { ::ClientToScreen(m_hWnd, lpPoint); }
void CWnd::ClientToScreen(LPRECT lpRect) const { ::MapWindowPoints(m_hWnd, nullptr, reinterpret_cast<LPPOINT>(lpRect), 2); }
void CWnd::ScreenToClient(LPPOINT lpPoint) const { ::ScreenToClient(m_hWnd, lpPoint); }
void CWnd::ScreenToClient(LPRECT lpRect) const { ::MapWindowPoints(nullptr, m_hWnd, reinterpret_cast<LPPOINT>(lpRect), 2); }
void CWnd::MapWindowPoints(CWnd* pwndTo, LPPOINT lpPoint, UINT nCount) const { ::MapWindowPoints(m_hWnd, pwndTo->GetSafeHwnd(), lpPoint, nCount); }
void CWnd::MapWindowPoints(CWnd* pwndTo, LPRECT lpRect) const { ::MapWindowPoints(m_hWnd, pwndTo->GetSafeHwnd(), reinterpret_cast<LPPOINT>(lpRect), 2); }

CDC* CWnd::GetDC() { return CDC::FromHandle(::GetDC(m_hWnd)); }
CDC* CWnd::GetWindowDC() { return CDC::FromHandle(::GetWindowDC(m_hWnd)); }
int CWnd::ReleaseDC(CDC* pDC) { return ::ReleaseDC(m_hWnd, pDC->m_hDC); }
void CWnd::UpdateWindow() { ::UpdateWindow(m_hWnd); }
void CWnd::SetRedraw(BOOL bRedraw) { SendMessage(WM_SETREDRAW, bRedraw); }
BOOL CWnd::GetUpdateRect(LPRECT lpRect, BOOL bErase) { return ::GetUpdateRect(m_hWnd, lpRect, bErase); }
int CWnd::GetUpdateRgn(CRgn* pRgn, BOOL bErase) { return ::GetUpdateRgn(m_hWnd, static_cast<HRGN>(pRgn->GetSafeHandle()), bErase); }
void CWnd::Invalidate(BOOL bErase) { ::InvalidateRect(m_hWnd, nullptr, bErase); }
void CWnd::InvalidateRect(LPCRECT lpRect, BOOL bErase) { ::InvalidateRect(m_hWnd, lpRect, bErase); }
void CWnd::InvalidateRgn(CRgn* pRgn, BOOL bErase) { ::InvalidateRgn(m_hWnd, static_cast<HRGN>(pRgn->GetSafeHandle()), bErase); }
void CWnd::ValidateRect(LPCRECT lpRect) { ::ValidateRect(m_hWnd, lpRect); }

BOOL CWnd::RedrawWindow(LPCRECT lpRectUpdate, CRgn* prgnUpdate, UINT flags)
{
	return ::RedrawWindow(m_hWnd, lpRectUpdate, prgnUpdate != nullptr ? static_cast<HRGN>(prgnUpdate->GetSafeHandle()) : nullptr, flags);
}

int CWnd::GetScrollPos(int nBar) const { return ::GetScrollPos(m_hWnd, nBar); }
int CWnd::SetScrollPos(int nBar, int nPos, BOOL bRedraw) { return ::SetScrollPos(m_hWnd, nBar, nPos, bRedraw); }
void CWnd::GetScrollRange(int nBar, LPINT lpMinPos, LPINT lpMaxPos) const { ::GetScrollRange(m_hWnd, nBar, lpMinPos, lpMaxPos); }
void CWnd::SetScrollRange(int nBar, int nMinPos, int nMaxPos, BOOL bRedraw) { ::SetScrollRange(m_hWnd, nBar, nMinPos, nMaxPos, bRedraw); }
BOOL CWnd::SetScrollInfo(int nBar, LPSCROLLINFO lpScrollInfo, BOOL bRedraw) { return ::SetScrollInfo(m_hWnd, nBar, lpScrollInfo, bRedraw) != 0; }
void CWnd::ScrollWindow(int xAmount, int yAmount, LPCRECT lpRect, LPCRECT lpClipRect) { ::ScrollWindow(m_hWnd, xAmount, yAmount, lpRect, lpClipRect); }

BOOL CWnd::GetScrollInfo(int nBar, LPSCROLLINFO lpScrollInfo, UINT nMask)
{
	lpScrollInfo->cbSize = sizeof(SCROLLINFO);
	lpScrollInfo->fMask = nMask;
	return ::GetScrollInfo(m_hWnd, nBar, lpScrollInfo);
}

LRESULT CWnd::SendMessage(UINT message, WPARAM wParam, LPARAM lParam) const { return ::SendMessageA(m_hWnd, message, wParam, lParam); }
BOOL CWnd::PostMessage(UINT message, WPARAM wParam, LPARAM lParam) const { return ::PostMessageA(m_hWnd, message, wParam, lParam); }

void CWnd::SendMessageToDescendants(UINT message, WPARAM wParam, LPARAM lParam, BOOL bDeep, BOOL bOnlyPerm)
{
	SendMessageToDescendants(m_hWnd, message, wParam, lParam, bDeep, bOnlyPerm);
}

void PASCAL CWnd::SendMessageToDescendants(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL bDeep, BOOL bOnlyPerm)
{
	for (HWND child = ::GetTopWindow(hWnd); child != nullptr; child = ::GetWindow(child, GW_HWNDNEXT))
	{
		CWnd* window = bOnlyPerm ? FromHandlePermanent(child) : nullptr;
		if (!bOnlyPerm)
		{
			::SendMessageA(child, message, wParam, lParam);
		}
		else if (window != nullptr)
		{
			AfxCallWndProc(window, child, message, wParam, lParam);
		}
		if (bDeep && ::IsWindow(child) && ::GetTopWindow(child) != nullptr)
		{
			SendMessageToDescendants(child, message, wParam, lParam, bDeep, bOnlyPerm);
		}
	}
}

UINT_PTR CWnd::SetTimer(UINT_PTR nIDEvent, UINT nElapse, AFX_TIMERPROC lpfnTimer)
{
	return ::SetTimer(m_hWnd, nIDEvent, nElapse, reinterpret_cast<TIMERPROC>(lpfnTimer));
}

BOOL CWnd::KillTimer(UINT_PTR nIDEvent) { return ::KillTimer(m_hWnd, nIDEvent); }

void CWnd::CheckDlgButton(int nIDButton, UINT nCheck) { ::CheckDlgButton(m_hWnd, nIDButton, nCheck); }
void CWnd::CheckRadioButton(int nIDFirstButton, int nIDLastButton, int nIDCheckButton) { ::CheckRadioButton(m_hWnd, nIDFirstButton, nIDLastButton, nIDCheckButton); }
UINT CWnd::IsDlgButtonChecked(int nIDButton) const { return ::IsDlgButtonChecked(m_hWnd, nIDButton); }
UINT CWnd::GetDlgItemInt(int nID, BOOL* lpTrans, BOOL bSigned) const { return ::GetDlgItemInt(m_hWnd, nID, lpTrans, bSigned); }
void CWnd::SetDlgItemInt(int nID, UINT nValue, BOOL bSigned) { ::SetDlgItemInt(m_hWnd, nID, nValue, bSigned); }
int CWnd::GetDlgItemText(int nID, LPSTR lpStr, int nMaxCount) const { return static_cast<int>(::GetDlgItemTextA(m_hWnd, nID, lpStr, nMaxCount)); }
void CWnd::SetDlgItemText(int nID, LPCSTR lpszString) { ::SetDlgItemTextA(m_hWnd, nID, lpszString); }
LRESULT CWnd::SendDlgItemMessage(int nID, UINT message, WPARAM wParam, LPARAM lParam) const { return ::SendDlgItemMessageA(m_hWnd, nID, message, wParam, lParam); }
CWnd* CWnd::GetNextDlgGroupItem(CWnd* pWndCtl, BOOL bPrevious) const { return FromHandle(::GetNextDlgGroupItem(m_hWnd, pWndCtl->GetSafeHwnd(), bPrevious)); }
CWnd* CWnd::GetNextDlgTabItem(CWnd* pWndCtl, BOOL bPrevious) const { return FromHandle(::GetNextDlgTabItem(m_hWnd, pWndCtl->GetSafeHwnd(), bPrevious)); }

int CWnd::GetDlgItemText(int nID, CString& rString) const
{
	rString.Empty();
	const HWND control = ::GetDlgItem(m_hWnd, nID);
	if (control == nullptr)
	{
		return 0;
	}
	const int length = ::GetWindowTextLengthA(control);
	::GetWindowTextA(control, rString.GetBufferSetLength(length), length + 1);
	rString.ReleaseBuffer();
	return rString.GetLength();
}

int CWnd::GetCheckedRadioButton(int nIDFirstButton, int nIDLastButton)
{
	for (int nID = nIDFirstButton; nID <= nIDLastButton; ++nID)
	{
		if (::IsDlgButtonChecked(m_hWnd, nID) != 0)
		{
			return nID;
		}
	}
	return 0;
}

int CWnd::MessageBox(LPCSTR lpszText, LPCSTR lpszCaption, UINT nType)
{
	return ::MessageBoxA(GetSafeHwnd(), lpszText, lpszCaption != nullptr ? lpszCaption : AfxGetAppName(), nType);
}
