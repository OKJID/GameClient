#include "AfxInternal.h"

#include <cstring>

IMPLEMENT_DYNAMIC(CStatic, CWnd)
IMPLEMENT_DYNAMIC(CButton, CWnd)
IMPLEMENT_DYNAMIC(CListBox, CWnd)
IMPLEMENT_DYNAMIC(CComboBox, CWnd)
IMPLEMENT_DYNAMIC(CEdit, CWnd)
IMPLEMENT_DYNAMIC(CScrollBar, CWnd)

UINT CListBox::ItemFromPoint(CPoint pt, BOOL& bOutside) const
{
	const DWORD result = static_cast<DWORD>(AFX_CONTROL_SEND(LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y)));
	bOutside = HIWORD(result) != 0 ? TRUE : FALSE;
	return LOWORD(result);
}

void CListBox::GetText(int nIndex, CString& rString) const
{
	const int length = GetTextLen(nIndex);
	if (length == LB_ERR)
	{
		rString.Empty();
		return;
	}
	GetText(nIndex, rString.GetBufferSetLength(length));
	rString.ReleaseBuffer();
}

int CListBox::SelItemRange(BOOL bSelect, int nFirstItem, int nLastItem)
{
	return static_cast<int>(AFX_CONTROL_SEND(LB_SELITEMRANGE, bSelect, MAKELONG(nFirstItem, nLastItem)));
}

void CComboBox::GetLBText(int nIndex, CString& rString) const
{
	const int length = GetLBTextLen(nIndex);
	if (length == CB_ERR)
	{
		rString.Empty();
		return;
	}
	GetLBText(nIndex, rString.GetBufferSetLength(length));
	rString.ReleaseBuffer();
}

void CEdit::GetSel(int& nStartChar, int& nEndChar) const
{
	DWORD start = 0;
	DWORD end = 0;
	AFX_CONTROL_SEND(EM_GETSEL, &start, &end);
	nStartChar = static_cast<int>(start);
	nEndChar = static_cast<int>(end);
}

int CEdit::GetLine(int nIndex, LPSTR lpszBuffer, int nMaxLength) const
{
	const WORD capacity = static_cast<WORD>(nMaxLength);
	memcpy(lpszBuffer, &capacity, sizeof(capacity));
	return static_cast<int>(AFX_CONTROL_SEND(EM_GETLINE, nIndex, lpszBuffer));
}

void CEdit::SetSel(DWORD dwSelection, BOOL bNoScroll)
{
	SetSel(static_cast<short>(LOWORD(dwSelection)), static_cast<short>(HIWORD(dwSelection)), bNoScroll);
}

void CEdit::SetSel(int nStartChar, int nEndChar, BOOL bNoScroll)
{
	AFX_CONTROL_SEND(EM_SETSEL, nStartChar, nEndChar);
	if (!bNoScroll)
	{
		AFX_CONTROL_SEND(EM_SCROLLCARET, 0, 0);
	}
}

BOOL CScrollBar::GetScrollInfo(LPSCROLLINFO lpScrollInfo, UINT nMask)
{
	lpScrollInfo->cbSize = sizeof(SCROLLINFO);
	lpScrollInfo->fMask = nMask;
	return ::GetScrollInfo(m_hWnd, SB_CTL, lpScrollInfo);
}

int CScrollBar::GetScrollLimit()
{
	SCROLLINFO info = {};
	info.cbSize = sizeof(info);
	info.fMask = SIF_RANGE | SIF_PAGE;
	if (!::GetScrollInfo(m_hWnd, SB_CTL, &info))
	{
		return 0;
	}
	return info.nMax - (info.nPage > 0 ? static_cast<int>(info.nPage) - 1 : 0);
}
