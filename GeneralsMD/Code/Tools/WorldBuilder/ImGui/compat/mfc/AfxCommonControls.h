#pragma once

#include "AfxDialogs.h"

class CImageList : public CObject
{
	DECLARE_DYNCREATE(CImageList)

public:
	CImageList();
	~CImageList() override;

	HIMAGELIST m_hImageList;

	operator HIMAGELIST() const { return m_hImageList; }
	HIMAGELIST GetSafeHandle() const;

	static CImageList* PASCAL FromHandle(HIMAGELIST hImageList);
	static void PASCAL DeleteTempMap();
	BOOL Attach(HIMAGELIST hImageList);
	HIMAGELIST Detach();

	BOOL Create(int cx, int cy, UINT nFlags, int nInitial, int nGrow);
	BOOL Create(UINT nBitmapID, int cx, int nGrow, COLORREF crMask);
	BOOL Create(LPCSTR lpszBitmapID, int cx, int nGrow, COLORREF crMask);
	BOOL DeleteImageList();

	int GetImageCount() const { return ImageList_GetImageCount(m_hImageList); }
	BOOL SetImageCount(UINT uNewCount) { return ImageList_SetImageCount(m_hImageList, uNewCount); }
	COLORREF SetBkColor(COLORREF cr) { return ImageList_SetBkColor(m_hImageList, cr); }
	int Add(CBitmap* pbmImage, CBitmap* pbmMask);
	int Add(CBitmap* pbmImage, COLORREF crMask);
	int Add(HICON hIcon) { return ImageList_ReplaceIcon(m_hImageList, -1, hIcon); }
	BOOL Remove(int nImage) { return ImageList_Remove(m_hImageList, nImage); }
	int Replace(int nImage, HICON hIcon) { return ImageList_ReplaceIcon(m_hImageList, nImage, hIcon); }
	BOOL Draw(CDC* pDC, int nImage, POINT pt, UINT nStyle);
};

class CTreeCtrl : public CWnd
{
	DECLARE_DYNAMIC(CTreeCtrl)

public:
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
		return CWnd::Create(WC_TREEVIEWA, nullptr, dwStyle, rect, pParentWnd, nID);
	}

	UINT GetCount() const { return static_cast<UINT>(AFX_CONTROL_SEND(TVM_GETCOUNT, 0, 0)); }
	UINT GetIndent() const { return static_cast<UINT>(AFX_CONTROL_SEND(TVM_GETINDENT, 0, 0)); }
	void SetIndent(UINT nIndent) { AFX_CONTROL_SEND(TVM_SETINDENT, nIndent, 0); }
	CImageList* GetImageList(UINT nImageList) const;
	CImageList* SetImageList(CImageList* pImageList, int nImageListType);
	HTREEITEM GetNextItem(HTREEITEM hItem, UINT nCode) const;
	HTREEITEM GetChildItem(HTREEITEM hItem) const { return GetNextItem(hItem, TVGN_CHILD); }
	HTREEITEM GetNextSiblingItem(HTREEITEM hItem) const { return GetNextItem(hItem, TVGN_NEXT); }
	HTREEITEM GetPrevSiblingItem(HTREEITEM hItem) const { return GetNextItem(hItem, TVGN_PREVIOUS); }
	HTREEITEM GetParentItem(HTREEITEM hItem) const { return GetNextItem(hItem, TVGN_PARENT); }
	HTREEITEM GetFirstVisibleItem() const { return GetNextItem(nullptr, TVGN_FIRSTVISIBLE); }
	HTREEITEM GetNextVisibleItem(HTREEITEM hItem) const { return GetNextItem(hItem, TVGN_NEXTVISIBLE); }
	HTREEITEM GetPrevVisibleItem(HTREEITEM hItem) const { return GetNextItem(hItem, TVGN_PREVIOUSVISIBLE); }
	HTREEITEM GetSelectedItem() const { return GetNextItem(nullptr, TVGN_CARET); }
	HTREEITEM GetDropHilightItem() const { return GetNextItem(nullptr, TVGN_DROPHILITE); }
	HTREEITEM GetRootItem() const { return GetNextItem(nullptr, TVGN_ROOT); }
	BOOL ItemHasChildren(HTREEITEM hItem) const;
	BOOL GetItem(TVITEMA* pItem) const { return static_cast<BOOL>(AFX_CONTROL_SEND(TVM_GETITEMA, 0, pItem)); }
	BOOL SetItem(TVITEMA* pItem) { return static_cast<BOOL>(AFX_CONTROL_SEND(TVM_SETITEMA, 0, pItem)); }
	BOOL SetItem(HTREEITEM hItem, UINT nMask, LPCSTR lpszItem, int nImage, int nSelectedImage, UINT nState, UINT nStateMask, LPARAM lParam);
	CString GetItemText(HTREEITEM hItem) const;
	BOOL SetItemText(HTREEITEM hItem, LPCSTR lpszItem);
	BOOL GetItemImage(HTREEITEM hItem, int& nImage, int& nSelectedImage) const;
	BOOL SetItemImage(HTREEITEM hItem, int nImage, int nSelectedImage);
	UINT GetItemState(HTREEITEM hItem, UINT nStateMask) const;
	BOOL SetItemState(HTREEITEM hItem, UINT nState, UINT nStateMask);
	DWORD_PTR GetItemData(HTREEITEM hItem) const;
	BOOL SetItemData(HTREEITEM hItem, DWORD_PTR dwData);
	BOOL GetItemRect(HTREEITEM hItem, LPRECT lpRect, BOOL bTextOnly) const;
	CEdit* GetEditControl() const;
	UINT GetVisibleCount() const { return static_cast<UINT>(AFX_CONTROL_SEND(TVM_GETVISIBLECOUNT, 0, 0)); }
	SHORT GetItemHeight() const { return static_cast<SHORT>(AFX_CONTROL_SEND(TVM_GETITEMHEIGHT, 0, 0)); }
	SHORT SetItemHeight(SHORT cyHeight) { return static_cast<SHORT>(AFX_CONTROL_SEND(TVM_SETITEMHEIGHT, cyHeight, 0)); }
	COLORREF SetBkColor(COLORREF clr) { return static_cast<COLORREF>(AFX_CONTROL_SEND(TVM_SETBKCOLOR, 0, clr)); }
	COLORREF SetTextColor(COLORREF clr) { return static_cast<COLORREF>(AFX_CONTROL_SEND(TVM_SETTEXTCOLOR, 0, clr)); }
	BOOL GetCheck(HTREEITEM hItem) const;
	BOOL SetCheck(HTREEITEM hItem, BOOL fCheck = TRUE);

	HTREEITEM InsertItem(LPTVINSERTSTRUCT lpInsertStruct);
	HTREEITEM InsertItem(UINT nMask, LPCSTR lpszItem, int nImage, int nSelectedImage, UINT nState, UINT nStateMask, LPARAM lParam,
		HTREEITEM hParent, HTREEITEM hInsertAfter);
	HTREEITEM InsertItem(LPCSTR lpszItem, HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST);
	HTREEITEM InsertItem(LPCSTR lpszItem, int nImage, int nSelectedImage, HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST);
	BOOL DeleteItem(HTREEITEM hItem) { return static_cast<BOOL>(AFX_CONTROL_SEND(TVM_DELETEITEM, 0, hItem)); }
	BOOL DeleteAllItems() { return DeleteItem(TVI_ROOT); }
	BOOL Expand(HTREEITEM hItem, UINT nCode) { return static_cast<BOOL>(AFX_CONTROL_SEND(TVM_EXPAND, nCode, hItem)); }
	BOOL Select(HTREEITEM hItem, UINT nCode) { return static_cast<BOOL>(AFX_CONTROL_SEND(TVM_SELECTITEM, nCode, hItem)); }
	BOOL SelectItem(HTREEITEM hItem) { return Select(hItem, TVGN_CARET); }
	BOOL SelectDropTarget(HTREEITEM hItem) { return Select(hItem, TVGN_DROPHILITE); }
	BOOL SelectSetFirstVisible(HTREEITEM hItem) { return Select(hItem, TVGN_FIRSTVISIBLE); }
	CEdit* EditLabel(HTREEITEM hItem);
	BOOL EndEditLabelNow(BOOL fCancelWithoutSave) { return static_cast<BOOL>(AFX_CONTROL_SEND(TVM_ENDEDITLABELNOW, fCancelWithoutSave, 0)); }
	HTREEITEM HitTest(CPoint pt, UINT* pFlags = nullptr) const;
	HTREEITEM HitTest(TVHITTESTINFO* pHitTestInfo) const;
	BOOL SortChildren(HTREEITEM hItem) { return static_cast<BOOL>(AFX_CONTROL_SEND(TVM_SORTCHILDREN, 0, hItem)); }
	BOOL SortChildrenCB(LPTVSORTCB pSort) { return static_cast<BOOL>(AFX_CONTROL_SEND(TVM_SORTCHILDRENCB, 0, pSort)); }
	BOOL EnsureVisible(HTREEITEM hItem) { return static_cast<BOOL>(AFX_CONTROL_SEND(TVM_ENSUREVISIBLE, 0, hItem)); }
};

class CListCtrl : public CWnd
{
	DECLARE_DYNAMIC(CListCtrl)

public:
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
		return CWnd::Create(WC_LISTVIEWA, nullptr, dwStyle, rect, pParentWnd, nID);
	}

	int GetItemCount() const { return static_cast<int>(AFX_CONTROL_SEND(LVM_GETITEMCOUNT, 0, 0)); }
	BOOL GetItem(LVITEMA* pItem) const { return static_cast<BOOL>(AFX_CONTROL_SEND(LVM_GETITEMA, 0, pItem)); }
	BOOL SetItem(const LVITEMA* pItem) { return static_cast<BOOL>(AFX_CONTROL_SEND(LVM_SETITEMA, 0, pItem)); }
	BOOL SetItem(int nItem, int nSubItem, UINT nMask, LPCSTR lpszItem, int nImage, UINT nState, UINT nStateMask, LPARAM lParam);
	UINT GetItemState(int nItem, UINT nMask) const { return static_cast<UINT>(AFX_CONTROL_SEND(LVM_GETITEMSTATE, nItem, nMask)); }
	BOOL SetItemState(int nItem, UINT nState, UINT nMask);
	BOOL SetItemState(int nItem, LVITEMA* pItem) { return static_cast<BOOL>(AFX_CONTROL_SEND(LVM_SETITEMSTATE, nItem, pItem)); }
	CString GetItemText(int nItem, int nSubItem) const;
	int GetItemText(int nItem, int nSubItem, LPSTR lpszText, int nLen) const;
	BOOL SetItemText(int nItem, int nSubItem, LPCSTR lpszText);
	DWORD_PTR GetItemData(int nItem) const;
	BOOL SetItemData(int nItem, DWORD_PTR dwData);
	BOOL GetItemRect(int nItem, LPRECT lpRect, UINT nCode) const;
	int GetNextItem(int nItem, int nFlags) const { return static_cast<int>(AFX_CONTROL_SEND(LVM_GETNEXTITEM, nItem, MAKELPARAM(nFlags, 0))); }
	UINT GetSelectedCount() const { return static_cast<UINT>(AFX_CONTROL_SEND(LVM_GETSELECTEDCOUNT, 0, 0)); }
	POSITION GetFirstSelectedItemPosition() const;
	int GetNextSelectedItem(POSITION& pos) const;
	int GetColumnWidth(int nCol) const { return static_cast<int>(AFX_CONTROL_SEND(LVM_GETCOLUMNWIDTH, nCol, 0)); }
	BOOL SetColumnWidth(int nCol, int cx) { return static_cast<BOOL>(AFX_CONTROL_SEND(LVM_SETCOLUMNWIDTH, nCol, MAKELPARAM(cx, 0))); }
	DWORD GetExtendedStyle() const { return static_cast<DWORD>(AFX_CONTROL_SEND(LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0)); }
	DWORD SetExtendedStyle(DWORD dwNewStyle) { return static_cast<DWORD>(AFX_CONTROL_SEND(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwNewStyle)); }
	CImageList* GetImageList(int nImageList) const;
	CImageList* SetImageList(CImageList* pImageList, int nImageListType);

	int InsertItem(const LVITEMA* pItem) { return static_cast<int>(AFX_CONTROL_SEND(LVM_INSERTITEMA, 0, pItem)); }
	int InsertItem(int nItem, LPCSTR lpszItem);
	int InsertItem(int nItem, LPCSTR lpszItem, int nImage);
	int InsertItem(UINT nMask, int nItem, LPCSTR lpszItem, UINT nState, UINT nStateMask, int nImage, LPARAM lParam);
	BOOL DeleteItem(int nItem) { return static_cast<BOOL>(AFX_CONTROL_SEND(LVM_DELETEITEM, nItem, 0)); }
	BOOL DeleteAllItems() { return static_cast<BOOL>(AFX_CONTROL_SEND(LVM_DELETEALLITEMS, 0, 0)); }
	int InsertColumn(int nCol, const LVCOLUMNA* pColumn) { return static_cast<int>(AFX_CONTROL_SEND(LVM_INSERTCOLUMNA, nCol, pColumn)); }
	int InsertColumn(int nCol, LPCSTR lpszColumnHeading, int nFormat = LVCFMT_LEFT, int nWidth = -1, int nSubItem = -1);
	BOOL DeleteColumn(int nCol) { return static_cast<BOOL>(AFX_CONTROL_SEND(LVM_DELETECOLUMN, nCol, 0)); }
	int FindItem(LVFINDINFOA* pFindInfo, int nStart = -1) const { return static_cast<int>(AFX_CONTROL_SEND(LVM_FINDITEMA, nStart, pFindInfo)); }
	int HitTest(LVHITTESTINFO* pHitTestInfo) const { return static_cast<int>(AFX_CONTROL_SEND(LVM_HITTEST, 0, pHitTestInfo)); }
	int HitTest(CPoint pt, UINT* pFlags = nullptr) const;
	int SubItemHitTest(LVHITTESTINFO* pInfo) { return static_cast<int>(AFX_CONTROL_SEND(LVM_SUBITEMHITTEST, 0, pInfo)); }
	BOOL EnsureVisible(int nItem, BOOL bPartialOK) { return static_cast<BOOL>(AFX_CONTROL_SEND(LVM_ENSUREVISIBLE, nItem, MAKELPARAM(bPartialOK, 0))); }
	BOOL RedrawItems(int nFirst, int nLast) { return static_cast<BOOL>(AFX_CONTROL_SEND(LVM_REDRAWITEMS, nFirst, nLast)); }
	BOOL SortItems(PFNLVCOMPARE pfnCompare, DWORD_PTR dwData) { return static_cast<BOOL>(AFX_CONTROL_SEND(LVM_SORTITEMS, dwData, pfnCompare)); }
};

class CSliderCtrl : public CWnd
{
	DECLARE_DYNAMIC(CSliderCtrl)

public:
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
		return CWnd::Create(TRACKBAR_CLASSA, nullptr, dwStyle, rect, pParentWnd, nID);
	}

	int GetLineSize() const { return static_cast<int>(AFX_CONTROL_SEND(TBM_GETLINESIZE, 0, 0)); }
	int SetLineSize(int nSize) { return static_cast<int>(AFX_CONTROL_SEND(TBM_SETLINESIZE, 0, nSize)); }
	int GetPageSize() const { return static_cast<int>(AFX_CONTROL_SEND(TBM_GETPAGESIZE, 0, 0)); }
	int SetPageSize(int nSize) { return static_cast<int>(AFX_CONTROL_SEND(TBM_SETPAGESIZE, 0, nSize)); }
	int GetRangeMax() const { return static_cast<int>(AFX_CONTROL_SEND(TBM_GETRANGEMAX, 0, 0)); }
	int GetRangeMin() const { return static_cast<int>(AFX_CONTROL_SEND(TBM_GETRANGEMIN, 0, 0)); }
	void GetRange(int& nMin, int& nMax) const { nMin = GetRangeMin(); nMax = GetRangeMax(); }
	void SetRangeMin(int nMin, BOOL bRedraw = FALSE) { AFX_CONTROL_SEND(TBM_SETRANGEMIN, bRedraw, nMin); }
	void SetRangeMax(int nMax, BOOL bRedraw = FALSE) { AFX_CONTROL_SEND(TBM_SETRANGEMAX, bRedraw, nMax); }
	void SetRange(int nMin, int nMax, BOOL bRedraw = FALSE) { SetRangeMin(nMin, FALSE); SetRangeMax(nMax, bRedraw); }
	void GetSelection(int& nMin, int& nMax) const;
	void SetSelection(int nMin, int nMax);
	void ClearSel(BOOL bRedraw = FALSE) { AFX_CONTROL_SEND(TBM_CLEARSEL, bRedraw, 0); }
	int GetPos() const { return static_cast<int>(AFX_CONTROL_SEND(TBM_GETPOS, 0, 0)); }
	void SetPos(int nPos) { AFX_CONTROL_SEND(TBM_SETPOS, TRUE, nPos); }
	void SetTicFreq(int nFreq) { AFX_CONTROL_SEND(TBM_SETTICFREQ, nFreq, 0); }
};

class CProgressCtrl : public CWnd
{
	DECLARE_DYNAMIC(CProgressCtrl)

public:
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
		return CWnd::Create(PROGRESS_CLASSA, nullptr, dwStyle, rect, pParentWnd, nID);
	}

	void SetRange(short nLower, short nUpper) { AFX_CONTROL_SEND(PBM_SETRANGE, 0, MAKELPARAM(nLower, nUpper)); }
	void SetRange32(int nLower, int nUpper) { AFX_CONTROL_SEND(PBM_SETRANGE32, nLower, nUpper); }
	void GetRange(int& nLower, int& nUpper) const;
	int GetPos() const { return static_cast<int>(AFX_CONTROL_SEND(PBM_GETPOS, 0, 0)); }
	int SetPos(int nPos) { return static_cast<int>(AFX_CONTROL_SEND(PBM_SETPOS, nPos, 0)); }
	int OffsetPos(int nPos) { return static_cast<int>(AFX_CONTROL_SEND(PBM_DELTAPOS, nPos, 0)); }
	int SetStep(int nStep) { return static_cast<int>(AFX_CONTROL_SEND(PBM_SETSTEP, nStep, 0)); }
	int StepIt() { return static_cast<int>(AFX_CONTROL_SEND(PBM_STEPIT, 0, 0)); }
	COLORREF SetBarColor(COLORREF clrBar) { return static_cast<COLORREF>(AFX_CONTROL_SEND(PBM_SETBARCOLOR, 0, clrBar)); }
};

class CTabCtrl : public CWnd
{
	DECLARE_DYNAMIC(CTabCtrl)

public:
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
		return CWnd::Create(WC_TABCONTROLA, nullptr, dwStyle, rect, pParentWnd, nID);
	}

	CImageList* GetImageList() const;
	CImageList* SetImageList(CImageList* pImageList);
	int GetItemCount() const { return static_cast<int>(AFX_CONTROL_SEND(TCM_GETITEMCOUNT, 0, 0)); }
	BOOL GetItem(int nItem, TCITEMA* pTabCtrlItem) const { return static_cast<BOOL>(AFX_CONTROL_SEND(TCM_GETITEMA, nItem, pTabCtrlItem)); }
	BOOL SetItem(int nItem, TCITEMA* pTabCtrlItem) { return static_cast<BOOL>(AFX_CONTROL_SEND(TCM_SETITEMA, nItem, pTabCtrlItem)); }
	BOOL GetItemRect(int nItem, LPRECT lpRect) const { return static_cast<BOOL>(AFX_CONTROL_SEND(TCM_GETITEMRECT, nItem, lpRect)); }
	int GetCurSel() const { return static_cast<int>(AFX_CONTROL_SEND(TCM_GETCURSEL, 0, 0)); }
	int SetCurSel(int nItem) { return static_cast<int>(AFX_CONTROL_SEND(TCM_SETCURSEL, nItem, 0)); }
	int GetCurFocus() const { return static_cast<int>(AFX_CONTROL_SEND(TCM_GETCURFOCUS, 0, 0)); }
	void SetCurFocus(int nItem) { AFX_CONTROL_SEND(TCM_SETCURFOCUS, nItem, 0); }
	LONG InsertItem(int nItem, TCITEMA* pTabCtrlItem) { return static_cast<LONG>(AFX_CONTROL_SEND(TCM_INSERTITEMA, nItem, pTabCtrlItem)); }
	LONG InsertItem(int nItem, LPCSTR lpszItem);
	LONG InsertItem(int nItem, LPCSTR lpszItem, int nImage);
	BOOL DeleteItem(int nItem) { return static_cast<BOOL>(AFX_CONTROL_SEND(TCM_DELETEITEM, nItem, 0)); }
	BOOL DeleteAllItems() { return static_cast<BOOL>(AFX_CONTROL_SEND(TCM_DELETEALLITEMS, 0, 0)); }
	void AdjustRect(BOOL bLarger, LPRECT lpRect) { AFX_CONTROL_SEND(TCM_ADJUSTRECT, bLarger, lpRect); }
	int HitTest(TCHITTESTINFO* pHitTestInfo) const { return static_cast<int>(AFX_CONTROL_SEND(TCM_HITTEST, 0, pHitTestInfo)); }
};

class CRichEditCtrl : public CWnd
{
	DECLARE_DYNAMIC(CRichEditCtrl)

public:
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
		return CWnd::Create(RICHEDIT_CLASSA, nullptr, dwStyle, rect, pParentWnd, nID);
	}

	void GetSel(CHARRANGE& cr) const { AFX_CONTROL_SEND(EM_EXGETSEL, 0, &cr); }
	void GetSel(long& nStartChar, long& nEndChar) const;
	void SetSel(CHARRANGE& cr) { AFX_CONTROL_SEND(EM_EXSETSEL, 0, &cr); }
	void SetSel(long nStartChar, long nEndChar);
	DWORD GetEventMask() const { return static_cast<DWORD>(AFX_CONTROL_SEND(EM_GETEVENTMASK, 0, 0)); }
	DWORD SetEventMask(DWORD dwEventMask) { return static_cast<DWORD>(AFX_CONTROL_SEND(EM_SETEVENTMASK, 0, dwEventMask)); }
	long GetLimitText() const { return static_cast<long>(AFX_CONTROL_SEND(EM_GETLIMITTEXT, 0, 0)); }
	void LimitText(long nChars = 0) { AFX_CONTROL_SEND(EM_EXLIMITTEXT, 0, nChars); }
	long GetSelText(LPSTR lpBuf) const { return static_cast<long>(AFX_CONTROL_SEND(EM_GETSELTEXT, 0, lpBuf)); }
	CString GetSelText() const;
	WORD GetSelectionType() const { return static_cast<WORD>(AFX_CONTROL_SEND(EM_SELECTIONTYPE, 0, 0)); }
	long GetTextRange(int nFirst, int nLast, CString& refString) const;
	DWORD GetDefaultCharFormat(CHARFORMATA& cf) const;
	DWORD GetSelectionCharFormat(CHARFORMATA& cf) const;
	BOOL SetDefaultCharFormat(CHARFORMATA& cf);
	BOOL SetSelectionCharFormat(CHARFORMATA& cf);
	BOOL SetWordCharFormat(CHARFORMATA& cf);
	COLORREF SetBackgroundColor(BOOL bSysColor, COLORREF cr) { return static_cast<COLORREF>(AFX_CONTROL_SEND(EM_SETBKGNDCOLOR, bSysColor, cr)); }
	BOOL SetReadOnly(BOOL bReadOnly = TRUE) { return static_cast<BOOL>(AFX_CONTROL_SEND(EM_SETREADONLY, bReadOnly, 0)); }
	void HideSelection(BOOL bHide, BOOL bPerm) { AFX_CONTROL_SEND(EM_HIDESELECTION, bHide, bPerm); }
	void ReplaceSel(LPCSTR lpszNewText, BOOL bCanUndo = FALSE) { AFX_CONTROL_SEND(EM_REPLACESEL, bCanUndo, lpszNewText); }
};
