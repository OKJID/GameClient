#pragma once

#include "AfxGdi.h"

#include <type_traits>

template<class Parameter, class Value>
Parameter AfxMessageParameter(Value value)
{
	if constexpr (std::is_null_pointer_v<Value>)
	{
		return 0;
	}
	else if constexpr (std::is_pointer_v<Value>)
	{
		return reinterpret_cast<Parameter>(value);
	}
	else
	{
		return static_cast<Parameter>(value);
	}
}

#define AFX_CONTROL_SEND(message, wParam, lParam) \
	::SendMessageA(m_hWnd, message, AfxMessageParameter<WPARAM>(wParam), AfxMessageParameter<LPARAM>(lParam))

class CStatic : public CWnd
{
	DECLARE_DYNAMIC(CStatic)

public:
	BOOL Create(LPCSTR lpszText, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID = 0xffff)
	{
		return CWnd::Create("STATIC", lpszText, dwStyle, rect, pParentWnd, nID);
	}
	HICON SetIcon(HICON hIcon) { return reinterpret_cast<HICON>(AFX_CONTROL_SEND(STM_SETICON, hIcon, 0)); }
	HICON GetIcon() const { return reinterpret_cast<HICON>(AFX_CONTROL_SEND(STM_GETICON, 0, 0)); }
	HBITMAP SetBitmap(HBITMAP hBitmap) { return reinterpret_cast<HBITMAP>(AFX_CONTROL_SEND(STM_SETIMAGE, IMAGE_BITMAP, hBitmap)); }
	HBITMAP GetBitmap() const { return reinterpret_cast<HBITMAP>(AFX_CONTROL_SEND(STM_GETIMAGE, IMAGE_BITMAP, 0)); }
	HCURSOR SetCursor(HCURSOR hCursor) { return reinterpret_cast<HCURSOR>(AFX_CONTROL_SEND(STM_SETIMAGE, IMAGE_CURSOR, hCursor)); }
	HCURSOR GetCursor() { return reinterpret_cast<HCURSOR>(AFX_CONTROL_SEND(STM_GETIMAGE, IMAGE_CURSOR, 0)); }
};

class CButton : public CWnd
{
	DECLARE_DYNAMIC(CButton)

public:
	BOOL Create(LPCSTR lpszCaption, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
		return CWnd::Create("BUTTON", lpszCaption, dwStyle, rect, pParentWnd, nID);
	}
	UINT GetState() const { return static_cast<UINT>(AFX_CONTROL_SEND(BM_GETSTATE, 0, 0)); }
	void SetState(BOOL bHighlight) { AFX_CONTROL_SEND(BM_SETSTATE, bHighlight, 0); }
	int GetCheck() const { return static_cast<int>(AFX_CONTROL_SEND(BM_GETCHECK, 0, 0)); }
	void SetCheck(int nCheck) { AFX_CONTROL_SEND(BM_SETCHECK, nCheck, 0); }
	UINT GetButtonStyle() const { return static_cast<UINT>(::GetWindowLongA(m_hWnd, GWL_STYLE) & 0xff); }
	void SetButtonStyle(UINT nStyle, BOOL bRedraw = TRUE) { AFX_CONTROL_SEND(BM_SETSTYLE, nStyle, bRedraw); }
	HICON SetIcon(HICON hIcon) { return reinterpret_cast<HICON>(AFX_CONTROL_SEND(BM_SETIMAGE, IMAGE_ICON, hIcon)); }
	HICON GetIcon() const { return reinterpret_cast<HICON>(AFX_CONTROL_SEND(BM_GETIMAGE, IMAGE_ICON, 0)); }
	HBITMAP SetBitmap(HBITMAP hBitmap) { return reinterpret_cast<HBITMAP>(AFX_CONTROL_SEND(BM_SETIMAGE, IMAGE_BITMAP, hBitmap)); }
	HBITMAP GetBitmap() const { return reinterpret_cast<HBITMAP>(AFX_CONTROL_SEND(BM_GETIMAGE, IMAGE_BITMAP, 0)); }
	HCURSOR SetCursor(HCURSOR hCursor) { return reinterpret_cast<HCURSOR>(AFX_CONTROL_SEND(BM_SETIMAGE, IMAGE_CURSOR, hCursor)); }
	HCURSOR GetCursor() { return reinterpret_cast<HCURSOR>(AFX_CONTROL_SEND(BM_GETIMAGE, IMAGE_CURSOR, 0)); }
};

class CListBox : public CWnd
{
	DECLARE_DYNAMIC(CListBox)

public:
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
		return CWnd::Create("LISTBOX", nullptr, dwStyle, rect, pParentWnd, nID);
	}
	int GetCount() const { return static_cast<int>(AFX_CONTROL_SEND(LB_GETCOUNT, 0, 0)); }
	int GetTopIndex() const { return static_cast<int>(AFX_CONTROL_SEND(LB_GETTOPINDEX, 0, 0)); }
	int GetHorizontalExtent() const { return static_cast<int>(AFX_CONTROL_SEND(LB_GETHORIZONTALEXTENT, 0, 0)); }
	void SetHorizontalExtent(int cxExtent) { AFX_CONTROL_SEND(LB_SETHORIZONTALEXTENT, cxExtent, 0); }
	int SetTopIndex(int nIndex) { return static_cast<int>(AFX_CONTROL_SEND(LB_SETTOPINDEX, nIndex, 0)); }
	DWORD_PTR GetItemData(int nIndex) const { return static_cast<DWORD_PTR>(AFX_CONTROL_SEND(LB_GETITEMDATA, nIndex, 0)); }
	int SetItemData(int nIndex, DWORD_PTR dwItemData) { return static_cast<int>(AFX_CONTROL_SEND(LB_SETITEMDATA, nIndex, dwItemData)); }
	void* GetItemDataPtr(int nIndex) const { return reinterpret_cast<void*>(GetItemData(nIndex)); }
	int SetItemDataPtr(int nIndex, void* pData) { return SetItemData(nIndex, reinterpret_cast<DWORD_PTR>(pData)); }
	int GetItemRect(int nIndex, LPRECT lpRect) const { return static_cast<int>(AFX_CONTROL_SEND(LB_GETITEMRECT, nIndex, lpRect)); }
	UINT ItemFromPoint(CPoint pt, BOOL& bOutside) const;
	int GetItemHeight(int nIndex) const { return static_cast<int>(AFX_CONTROL_SEND(LB_GETITEMHEIGHT, nIndex, 0)); }
	int SetItemHeight(int nIndex, UINT cyItemHeight) { return static_cast<int>(AFX_CONTROL_SEND(LB_SETITEMHEIGHT, nIndex, MAKELONG(cyItemHeight, 0))); }
	int GetSel(int nIndex) const { return static_cast<int>(AFX_CONTROL_SEND(LB_GETSEL, nIndex, 0)); }
	int GetText(int nIndex, LPSTR lpszBuffer) const { return static_cast<int>(AFX_CONTROL_SEND(LB_GETTEXT, nIndex, lpszBuffer)); }
	void GetText(int nIndex, CString& rString) const;
	int GetTextLen(int nIndex) const { return static_cast<int>(AFX_CONTROL_SEND(LB_GETTEXTLEN, nIndex, 0)); }
	int GetCurSel() const { return static_cast<int>(AFX_CONTROL_SEND(LB_GETCURSEL, 0, 0)); }
	int SetCurSel(int nSelect) { return static_cast<int>(AFX_CONTROL_SEND(LB_SETCURSEL, nSelect, 0)); }
	int SetSel(int nIndex, BOOL bSelect = TRUE) { return static_cast<int>(AFX_CONTROL_SEND(LB_SETSEL, bSelect, nIndex)); }
	int GetCaretIndex() const { return static_cast<int>(AFX_CONTROL_SEND(LB_GETCARETINDEX, 0, 0)); }
	int SetCaretIndex(int nIndex, BOOL bScroll = TRUE) { return static_cast<int>(AFX_CONTROL_SEND(LB_SETCARETINDEX, nIndex, MAKELONG(bScroll, 0))); }
	int GetSelCount() const { return static_cast<int>(AFX_CONTROL_SEND(LB_GETSELCOUNT, 0, 0)); }
	int GetSelItems(int nMaxItems, LPINT rgIndex) const { return static_cast<int>(AFX_CONTROL_SEND(LB_GETSELITEMS, nMaxItems, rgIndex)); }
	int SelItemRange(BOOL bSelect, int nFirstItem, int nLastItem);
	int AddString(LPCSTR lpszItem) { return static_cast<int>(AFX_CONTROL_SEND(LB_ADDSTRING, 0, lpszItem)); }
	int DeleteString(UINT nIndex) { return static_cast<int>(AFX_CONTROL_SEND(LB_DELETESTRING, nIndex, 0)); }
	int InsertString(int nIndex, LPCSTR lpszItem) { return static_cast<int>(AFX_CONTROL_SEND(LB_INSERTSTRING, nIndex, lpszItem)); }
	void ResetContent() { AFX_CONTROL_SEND(LB_RESETCONTENT, 0, 0); }
	int FindString(int nStartAfter, LPCSTR lpszItem) const { return static_cast<int>(AFX_CONTROL_SEND(LB_FINDSTRING, nStartAfter, lpszItem)); }
	int FindStringExact(int nIndexStart, LPCSTR lpszFind) const { return static_cast<int>(AFX_CONTROL_SEND(LB_FINDSTRINGEXACT, nIndexStart, lpszFind)); }
	int SelectString(int nStartAfter, LPCSTR lpszItem) { return static_cast<int>(AFX_CONTROL_SEND(LB_SELECTSTRING, nStartAfter, lpszItem)); }
};

class CComboBox : public CWnd
{
	DECLARE_DYNAMIC(CComboBox)

public:
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
		return CWnd::Create("COMBOBOX", nullptr, dwStyle, rect, pParentWnd, nID);
	}
	int GetCount() const { return static_cast<int>(AFX_CONTROL_SEND(CB_GETCOUNT, 0, 0)); }
	int GetCurSel() const { return static_cast<int>(AFX_CONTROL_SEND(CB_GETCURSEL, 0, 0)); }
	int SetCurSel(int nSelect) { return static_cast<int>(AFX_CONTROL_SEND(CB_SETCURSEL, nSelect, 0)); }
	DWORD GetEditSel() const { return static_cast<DWORD>(AFX_CONTROL_SEND(CB_GETEDITSEL, 0, 0)); }
	BOOL LimitText(int nMaxChars) { return static_cast<BOOL>(AFX_CONTROL_SEND(CB_LIMITTEXT, nMaxChars, 0)); }
	BOOL SetEditSel(int nStartChar, int nEndChar) { return static_cast<BOOL>(AFX_CONTROL_SEND(CB_SETEDITSEL, 0, MAKELONG(nStartChar, nEndChar))); }
	DWORD_PTR GetItemData(int nIndex) const { return static_cast<DWORD_PTR>(AFX_CONTROL_SEND(CB_GETITEMDATA, nIndex, 0)); }
	int SetItemData(int nIndex, DWORD_PTR dwItemData) { return static_cast<int>(AFX_CONTROL_SEND(CB_SETITEMDATA, nIndex, dwItemData)); }
	void* GetItemDataPtr(int nIndex) const { return reinterpret_cast<void*>(GetItemData(nIndex)); }
	int SetItemDataPtr(int nIndex, void* pData) { return SetItemData(nIndex, reinterpret_cast<DWORD_PTR>(pData)); }
	int GetLBText(int nIndex, LPSTR lpszText) const { return static_cast<int>(AFX_CONTROL_SEND(CB_GETLBTEXT, nIndex, lpszText)); }
	void GetLBText(int nIndex, CString& rString) const;
	int GetLBTextLen(int nIndex) const { return static_cast<int>(AFX_CONTROL_SEND(CB_GETLBTEXTLEN, nIndex, 0)); }
	int GetItemHeight(int nIndex) const { return static_cast<int>(AFX_CONTROL_SEND(CB_GETITEMHEIGHT, nIndex, 0)); }
	int FindStringExact(int nIndexStart, LPCSTR lpszFind) const { return static_cast<int>(AFX_CONTROL_SEND(CB_FINDSTRINGEXACT, nIndexStart, lpszFind)); }
	void GetDroppedControlRect(LPRECT lprect) const { AFX_CONTROL_SEND(CB_GETDROPPEDCONTROLRECT, 0, lprect); }
	BOOL GetDroppedState() const { return static_cast<BOOL>(AFX_CONTROL_SEND(CB_GETDROPPEDSTATE, 0, 0)); }
	int GetDroppedWidth() const { return static_cast<int>(AFX_CONTROL_SEND(CB_GETDROPPEDWIDTH, 0, 0)); }
	int SetDroppedWidth(UINT nWidth) { return static_cast<int>(AFX_CONTROL_SEND(CB_SETDROPPEDWIDTH, nWidth, 0)); }
	void ShowDropDown(BOOL bShowIt = TRUE) { AFX_CONTROL_SEND(CB_SHOWDROPDOWN, bShowIt, 0); }
	int AddString(LPCSTR lpszString) { return static_cast<int>(AFX_CONTROL_SEND(CB_ADDSTRING, 0, lpszString)); }
	int DeleteString(UINT nIndex) { return static_cast<int>(AFX_CONTROL_SEND(CB_DELETESTRING, nIndex, 0)); }
	int InsertString(int nIndex, LPCSTR lpszString) { return static_cast<int>(AFX_CONTROL_SEND(CB_INSERTSTRING, nIndex, lpszString)); }
	void ResetContent() { AFX_CONTROL_SEND(CB_RESETCONTENT, 0, 0); }
	int FindString(int nStartAfter, LPCSTR lpszString) const { return static_cast<int>(AFX_CONTROL_SEND(CB_FINDSTRING, nStartAfter, lpszString)); }
	int SelectString(int nStartAfter, LPCSTR lpszString) { return static_cast<int>(AFX_CONTROL_SEND(CB_SELECTSTRING, nStartAfter, lpszString)); }
	void Clear() { AFX_CONTROL_SEND(WM_CLEAR, 0, 0); }
	void Copy() { AFX_CONTROL_SEND(WM_COPY, 0, 0); }
	void Cut() { AFX_CONTROL_SEND(WM_CUT, 0, 0); }
	void Paste() { AFX_CONTROL_SEND(WM_PASTE, 0, 0); }
};

class CEdit : public CWnd
{
	DECLARE_DYNAMIC(CEdit)

public:
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
		return CWnd::Create("EDIT", nullptr, dwStyle, rect, pParentWnd, nID);
	}
	BOOL CanUndo() const { return static_cast<BOOL>(AFX_CONTROL_SEND(EM_CANUNDO, 0, 0)); }
	int GetLineCount() const { return static_cast<int>(AFX_CONTROL_SEND(EM_GETLINECOUNT, 0, 0)); }
	BOOL GetModify() const { return static_cast<BOOL>(AFX_CONTROL_SEND(EM_GETMODIFY, 0, 0)); }
	void SetModify(BOOL bModified = TRUE) { AFX_CONTROL_SEND(EM_SETMODIFY, bModified, 0); }
	DWORD GetSel() const { return static_cast<DWORD>(AFX_CONTROL_SEND(EM_GETSEL, 0, 0)); }
	void GetSel(int& nStartChar, int& nEndChar) const;
	int GetLine(int nIndex, LPSTR lpszBuffer) const { return static_cast<int>(AFX_CONTROL_SEND(EM_GETLINE, nIndex, lpszBuffer)); }
	int GetLine(int nIndex, LPSTR lpszBuffer, int nMaxLength) const;
	UINT GetLimitText() const { return static_cast<UINT>(AFX_CONTROL_SEND(EM_GETLIMITTEXT, 0, 0)); }
	void SetLimitText(UINT nMax) { AFX_CONTROL_SEND(EM_LIMITTEXT, nMax, 0); }
	void LimitText(int nChars = 0) { AFX_CONTROL_SEND(EM_LIMITTEXT, nChars, 0); }
	int LineFromChar(int nIndex = -1) const { return static_cast<int>(AFX_CONTROL_SEND(EM_LINEFROMCHAR, nIndex, 0)); }
	int LineIndex(int nLine = -1) const { return static_cast<int>(AFX_CONTROL_SEND(EM_LINEINDEX, nLine, 0)); }
	int LineLength(int nLine = -1) const { return static_cast<int>(AFX_CONTROL_SEND(EM_LINELENGTH, nLine, 0)); }
	void ReplaceSel(LPCSTR lpszNewText, BOOL bCanUndo = FALSE) { AFX_CONTROL_SEND(EM_REPLACESEL, bCanUndo, lpszNewText); }
	void SetSel(DWORD dwSelection, BOOL bNoScroll = FALSE);
	void SetSel(int nStartChar, int nEndChar, BOOL bNoScroll = FALSE);
	BOOL SetReadOnly(BOOL bReadOnly = TRUE) { return static_cast<BOOL>(AFX_CONTROL_SEND(EM_SETREADONLY, bReadOnly, 0)); }
	BOOL Undo() { return static_cast<BOOL>(AFX_CONTROL_SEND(EM_UNDO, 0, 0)); }
	void Clear() { AFX_CONTROL_SEND(WM_CLEAR, 0, 0); }
	void Copy() { AFX_CONTROL_SEND(WM_COPY, 0, 0); }
	void Cut() { AFX_CONTROL_SEND(WM_CUT, 0, 0); }
	void Paste() { AFX_CONTROL_SEND(WM_PASTE, 0, 0); }
};

class CScrollBar : public CWnd
{
	DECLARE_DYNAMIC(CScrollBar)

public:
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
		return CWnd::Create("SCROLLBAR", nullptr, dwStyle, rect, pParentWnd, nID);
	}
	int GetScrollPos() const { return ::GetScrollPos(m_hWnd, SB_CTL); }
	int SetScrollPos(int nPos, BOOL bRedraw = TRUE) { return ::SetScrollPos(m_hWnd, SB_CTL, nPos, bRedraw); }
	void GetScrollRange(LPINT lpMinPos, LPINT lpMaxPos) const { ::GetScrollRange(m_hWnd, SB_CTL, lpMinPos, lpMaxPos); }
	void SetScrollRange(int nMinPos, int nMaxPos, BOOL bRedraw = TRUE) { ::SetScrollRange(m_hWnd, SB_CTL, nMinPos, nMaxPos, bRedraw); }
	BOOL SetScrollInfo(LPSCROLLINFO lpScrollInfo, BOOL bRedraw = TRUE) { return ::SetScrollInfo(m_hWnd, SB_CTL, lpScrollInfo, bRedraw); }
	BOOL GetScrollInfo(LPSCROLLINFO lpScrollInfo, UINT nMask = SIF_ALL);
	int GetScrollLimit();
};
