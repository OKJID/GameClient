#include "AfxInternal.h"

#include "afxcmn.h"

#include <cstring>

IMPLEMENT_DYNCREATE(CImageList, CObject)
IMPLEMENT_DYNAMIC(CTreeCtrl, CWnd)
IMPLEMENT_DYNAMIC(CListCtrl, CWnd)
IMPLEMENT_DYNAMIC(CSliderCtrl, CWnd)
IMPLEMENT_DYNAMIC(CProgressCtrl, CWnd)
IMPLEMENT_DYNAMIC(CTabCtrl, CWnd)
IMPLEMENT_DYNAMIC(CRichEditCtrl, CWnd)

namespace
{

const int kInitialTextCapacity = 256;

HIMAGELIST imageListHandle(LRESULT result)
{
	return reinterpret_cast<HIMAGELIST>(result);
}

HTREEITEM treeItemHandle(LRESULT result)
{
	return reinterpret_cast<HTREEITEM>(result);
}

}

CImageList::CImageList() :
	m_hImageList(nullptr)
{
}

CImageList::~CImageList()
{
	DeleteImageList();
}

HIMAGELIST CImageList::GetSafeHandle() const
{
	const CImageList* self = this;
	return self != nullptr ? m_hImageList : nullptr;
}

CImageList* PASCAL CImageList::FromHandle(HIMAGELIST hImageList)
{
	return AfxImageListMap().fromHandle(hImageList);
}

void PASCAL CImageList::DeleteTempMap()
{
	AfxImageListMap().deleteTemporaries();
}

BOOL CImageList::Attach(HIMAGELIST hImageList)
{
	if (hImageList == nullptr)
	{
		return FALSE;
	}
	m_hImageList = hImageList;
	AfxImageListMap().setPermanent(hImageList, this);
	return TRUE;
}

HIMAGELIST CImageList::Detach()
{
	const HIMAGELIST hImageList = m_hImageList;
	if (hImageList != nullptr)
	{
		AfxImageListMap().removeHandle(hImageList);
	}
	m_hImageList = nullptr;
	return hImageList;
}

BOOL CImageList::Create(int cx, int cy, UINT nFlags, int nInitial, int nGrow)
{
	return Attach(ImageList_Create(cx, cy, nFlags, nInitial, nGrow));
}

BOOL CImageList::Create(UINT nBitmapID, int cx, int nGrow, COLORREF crMask)
{
	return Create(MAKEINTRESOURCE(nBitmapID), cx, nGrow, crMask);
}

BOOL CImageList::Create(LPCSTR lpszBitmapID, int cx, int nGrow, COLORREF crMask)
{
	return Attach(ImageList_LoadBitmap(AfxGetResourceHandle(), lpszBitmapID, cx, nGrow, crMask));
}

BOOL CImageList::DeleteImageList()
{
	if (m_hImageList == nullptr)
	{
		return FALSE;
	}
	return ImageList_Destroy(Detach());
}

int CImageList::Add(CBitmap* pbmImage, CBitmap* pbmMask)
{
	return ImageList_Add(m_hImageList, static_cast<HBITMAP>(pbmImage->GetSafeHandle()), static_cast<HBITMAP>(pbmMask->GetSafeHandle()));
}

int CImageList::Add(CBitmap* pbmImage, COLORREF crMask)
{
	return ImageList_AddMasked(m_hImageList, static_cast<HBITMAP>(pbmImage->GetSafeHandle()), crMask);
}

BOOL CImageList::Draw(CDC* pDC, int nImage, POINT pt, UINT nStyle)
{
	return ImageList_Draw(m_hImageList, nImage, pDC->GetSafeHdc(), pt.x, pt.y, nStyle);
}

CImageList* CTreeCtrl::GetImageList(UINT nImageList) const
{
	return CImageList::FromHandle(imageListHandle(AFX_CONTROL_SEND(TVM_GETIMAGELIST, nImageList, 0)));
}

CImageList* CTreeCtrl::SetImageList(CImageList* pImageList, int nImageListType)
{
	return CImageList::FromHandle(imageListHandle(AFX_CONTROL_SEND(TVM_SETIMAGELIST, nImageListType, pImageList->GetSafeHandle())));
}

HTREEITEM CTreeCtrl::GetNextItem(HTREEITEM hItem, UINT nCode) const
{
	return treeItemHandle(AFX_CONTROL_SEND(TVM_GETNEXTITEM, nCode, hItem));
}

BOOL CTreeCtrl::ItemHasChildren(HTREEITEM hItem) const
{
	TVITEMA item = {};
	item.hItem = hItem;
	item.mask = TVIF_CHILDREN;
	AFX_CONTROL_SEND(TVM_GETITEMA, 0, &item);
	return item.cChildren;
}

BOOL CTreeCtrl::SetItem(HTREEITEM hItem, UINT nMask, LPCSTR lpszItem, int nImage, int nSelectedImage, UINT nState, UINT nStateMask, LPARAM lParam)
{
	TVITEMA item = {};
	item.hItem = hItem;
	item.mask = nMask;
	item.pszText = const_cast<LPSTR>(lpszItem);
	item.iImage = nImage;
	item.iSelectedImage = nSelectedImage;
	item.state = nState;
	item.stateMask = nStateMask;
	item.lParam = lParam;
	return static_cast<BOOL>(AFX_CONTROL_SEND(TVM_SETITEMA, 0, &item));
}

CString CTreeCtrl::GetItemText(HTREEITEM hItem) const
{
	TVITEMA item = {};
	item.hItem = hItem;
	item.mask = TVIF_TEXT;
	CString text;
	int capacity = kInitialTextCapacity;
	for (;;)
	{
		item.pszText = text.GetBufferSetLength(capacity);
		item.cchTextMax = capacity;
		AFX_CONTROL_SEND(TVM_GETITEMA, 0, &item);
		if (static_cast<int>(strlen(item.pszText)) < capacity - 1)
		{
			break;
		}
		capacity *= 2;
	}
	text.ReleaseBuffer();
	return text;
}

BOOL CTreeCtrl::SetItemText(HTREEITEM hItem, LPCSTR lpszItem)
{
	return SetItem(hItem, TVIF_TEXT, lpszItem, 0, 0, 0, 0, 0);
}

BOOL CTreeCtrl::GetItemImage(HTREEITEM hItem, int& nImage, int& nSelectedImage) const
{
	TVITEMA item = {};
	item.hItem = hItem;
	item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	const BOOL result = static_cast<BOOL>(AFX_CONTROL_SEND(TVM_GETITEMA, 0, &item));
	if (!result)
	{
		return FALSE;
	}
	nImage = item.iImage;
	nSelectedImage = item.iSelectedImage;
	return TRUE;
}

BOOL CTreeCtrl::SetItemImage(HTREEITEM hItem, int nImage, int nSelectedImage)
{
	return SetItem(hItem, TVIF_IMAGE | TVIF_SELECTEDIMAGE, nullptr, nImage, nSelectedImage, 0, 0, 0);
}

UINT CTreeCtrl::GetItemState(HTREEITEM hItem, UINT nStateMask) const
{
	TVITEMA item = {};
	item.hItem = hItem;
	item.mask = TVIF_STATE;
	item.stateMask = nStateMask;
	AFX_CONTROL_SEND(TVM_GETITEMA, 0, &item);
	return item.state & nStateMask;
}

BOOL CTreeCtrl::SetItemState(HTREEITEM hItem, UINT nState, UINT nStateMask)
{
	return SetItem(hItem, TVIF_STATE, nullptr, 0, 0, nState, nStateMask, 0);
}

DWORD_PTR CTreeCtrl::GetItemData(HTREEITEM hItem) const
{
	TVITEMA item = {};
	item.hItem = hItem;
	item.mask = TVIF_PARAM;
	AFX_CONTROL_SEND(TVM_GETITEMA, 0, &item);
	return static_cast<DWORD_PTR>(item.lParam);
}

BOOL CTreeCtrl::SetItemData(HTREEITEM hItem, DWORD_PTR dwData)
{
	return SetItem(hItem, TVIF_PARAM, nullptr, 0, 0, 0, 0, static_cast<LPARAM>(dwData));
}

BOOL CTreeCtrl::GetItemRect(HTREEITEM hItem, LPRECT lpRect, BOOL bTextOnly) const
{
	memcpy(lpRect, &hItem, sizeof(hItem));
	return static_cast<BOOL>(AFX_CONTROL_SEND(TVM_GETITEMRECT, bTextOnly, lpRect));
}

CEdit* CTreeCtrl::GetEditControl() const
{
	return static_cast<CEdit*>(CWnd::FromHandle(reinterpret_cast<HWND>(AFX_CONTROL_SEND(TVM_GETEDITCONTROL, 0, 0))));
}

BOOL CTreeCtrl::GetCheck(HTREEITEM hItem) const
{
	TVITEMA item = {};
	item.mask = TVIF_HANDLE | TVIF_STATE;
	item.hItem = hItem;
	item.stateMask = TVIS_STATEIMAGEMASK;
	AFX_CONTROL_SEND(TVM_GETITEMA, 0, &item);
	return static_cast<BOOL>(item.state >> 12) - 1;
}

BOOL CTreeCtrl::SetCheck(HTREEITEM hItem, BOOL fCheck)
{
	TVITEMA item = {};
	item.mask = TVIF_HANDLE | TVIF_STATE;
	item.hItem = hItem;
	item.stateMask = TVIS_STATEIMAGEMASK;
	item.state = INDEXTOSTATEIMAGEMASK(fCheck ? 2 : 1);
	return static_cast<BOOL>(AFX_CONTROL_SEND(TVM_SETITEMA, 0, &item));
}

HTREEITEM CTreeCtrl::InsertItem(LPTVINSERTSTRUCT lpInsertStruct)
{
	return treeItemHandle(AFX_CONTROL_SEND(TVM_INSERTITEMA, 0, lpInsertStruct));
}

HTREEITEM CTreeCtrl::InsertItem(UINT nMask, LPCSTR lpszItem, int nImage, int nSelectedImage, UINT nState, UINT nStateMask, LPARAM lParam,
	HTREEITEM hParent, HTREEITEM hInsertAfter)
{
	TVINSERTSTRUCTA insert = {};
	insert.hParent = hParent;
	insert.hInsertAfter = hInsertAfter;
	insert.item.mask = nMask;
	insert.item.pszText = const_cast<LPSTR>(lpszItem);
	insert.item.iImage = nImage;
	insert.item.iSelectedImage = nSelectedImage;
	insert.item.state = nState;
	insert.item.stateMask = nStateMask;
	insert.item.lParam = lParam;
	return InsertItem(&insert);
}

HTREEITEM CTreeCtrl::InsertItem(LPCSTR lpszItem, HTREEITEM hParent, HTREEITEM hInsertAfter)
{
	return InsertItem(TVIF_TEXT, lpszItem, 0, 0, 0, 0, 0, hParent, hInsertAfter);
}

HTREEITEM CTreeCtrl::InsertItem(LPCSTR lpszItem, int nImage, int nSelectedImage, HTREEITEM hParent, HTREEITEM hInsertAfter)
{
	return InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE, lpszItem, nImage, nSelectedImage, 0, 0, 0, hParent, hInsertAfter);
}

CEdit* CTreeCtrl::EditLabel(HTREEITEM hItem)
{
	return static_cast<CEdit*>(CWnd::FromHandle(reinterpret_cast<HWND>(AFX_CONTROL_SEND(TVM_EDITLABELA, 0, hItem))));
}

HTREEITEM CTreeCtrl::HitTest(CPoint pt, UINT* pFlags) const
{
	TVHITTESTINFO hitTest = {};
	hitTest.pt = pt;
	const HTREEITEM hItem = HitTest(&hitTest);
	if (pFlags != nullptr)
	{
		*pFlags = hitTest.flags;
	}
	return hItem;
}

HTREEITEM CTreeCtrl::HitTest(TVHITTESTINFO* pHitTestInfo) const
{
	return treeItemHandle(AFX_CONTROL_SEND(TVM_HITTEST, 0, pHitTestInfo));
}

BOOL CListCtrl::SetItem(int nItem, int nSubItem, UINT nMask, LPCSTR lpszItem, int nImage, UINT nState, UINT nStateMask, LPARAM lParam)
{
	LVITEMA item = {};
	item.mask = nMask;
	item.iItem = nItem;
	item.iSubItem = nSubItem;
	item.stateMask = nStateMask;
	item.state = nState;
	item.pszText = const_cast<LPSTR>(lpszItem);
	item.iImage = nImage;
	item.lParam = lParam;
	return SetItem(&item);
}

BOOL CListCtrl::SetItemState(int nItem, UINT nState, UINT nMask)
{
	LVITEMA item = {};
	item.stateMask = nMask;
	item.state = nState;
	return SetItemState(nItem, &item);
}

CString CListCtrl::GetItemText(int nItem, int nSubItem) const
{
	CString text;
	int capacity = kInitialTextCapacity;
	for (;;)
	{
		const int length = GetItemText(nItem, nSubItem, text.GetBufferSetLength(capacity), capacity);
		if (length < capacity - 1)
		{
			break;
		}
		capacity *= 2;
	}
	text.ReleaseBuffer();
	return text;
}

int CListCtrl::GetItemText(int nItem, int nSubItem, LPSTR lpszText, int nLen) const
{
	LVITEMA item = {};
	item.iSubItem = nSubItem;
	item.cchTextMax = nLen;
	item.pszText = lpszText;
	return static_cast<int>(AFX_CONTROL_SEND(LVM_GETITEMTEXTA, nItem, &item));
}

BOOL CListCtrl::SetItemText(int nItem, int nSubItem, LPCSTR lpszText)
{
	LVITEMA item = {};
	item.iSubItem = nSubItem;
	item.pszText = const_cast<LPSTR>(lpszText);
	return static_cast<BOOL>(AFX_CONTROL_SEND(LVM_SETITEMTEXTA, nItem, &item));
}

DWORD_PTR CListCtrl::GetItemData(int nItem) const
{
	LVITEMA item = {};
	item.iItem = nItem;
	item.mask = LVIF_PARAM;
	GetItem(&item);
	return static_cast<DWORD_PTR>(item.lParam);
}

BOOL CListCtrl::SetItemData(int nItem, DWORD_PTR dwData)
{
	return SetItem(nItem, 0, LVIF_PARAM, nullptr, 0, 0, 0, static_cast<LPARAM>(dwData));
}

BOOL CListCtrl::GetItemRect(int nItem, LPRECT lpRect, UINT nCode) const
{
	lpRect->left = static_cast<LONG>(nCode);
	return static_cast<BOOL>(AFX_CONTROL_SEND(LVM_GETITEMRECT, nItem, lpRect));
}

POSITION CListCtrl::GetFirstSelectedItemPosition() const
{
	return reinterpret_cast<POSITION>(static_cast<INT_PTR>(1 + GetNextItem(-1, LVIS_SELECTED)));
}

int CListCtrl::GetNextSelectedItem(POSITION& pos) const
{
	const int current = static_cast<int>(reinterpret_cast<INT_PTR>(pos)) - 1;
	pos = reinterpret_cast<POSITION>(static_cast<INT_PTR>(1 + GetNextItem(current, LVIS_SELECTED)));
	return current;
}

CImageList* CListCtrl::GetImageList(int nImageList) const
{
	return CImageList::FromHandle(imageListHandle(AFX_CONTROL_SEND(LVM_GETIMAGELIST, nImageList, 0)));
}

CImageList* CListCtrl::SetImageList(CImageList* pImageList, int nImageListType)
{
	return CImageList::FromHandle(imageListHandle(AFX_CONTROL_SEND(LVM_SETIMAGELIST, nImageListType, pImageList->GetSafeHandle())));
}

int CListCtrl::InsertItem(int nItem, LPCSTR lpszItem)
{
	return InsertItem(LVIF_TEXT, nItem, lpszItem, 0, 0, 0, 0);
}

int CListCtrl::InsertItem(int nItem, LPCSTR lpszItem, int nImage)
{
	return InsertItem(LVIF_TEXT | LVIF_IMAGE, nItem, lpszItem, 0, 0, nImage, 0);
}

int CListCtrl::InsertItem(UINT nMask, int nItem, LPCSTR lpszItem, UINT nState, UINT nStateMask, int nImage, LPARAM lParam)
{
	LVITEMA item = {};
	item.mask = nMask;
	item.iItem = nItem;
	item.pszText = const_cast<LPSTR>(lpszItem);
	item.state = nState;
	item.stateMask = nStateMask;
	item.iImage = nImage;
	item.lParam = lParam;
	return InsertItem(&item);
}

int CListCtrl::InsertColumn(int nCol, LPCSTR lpszColumnHeading, int nFormat, int nWidth, int nSubItem)
{
	LVCOLUMNA column = {};
	column.mask = LVCF_TEXT | LVCF_FMT;
	column.pszText = const_cast<LPSTR>(lpszColumnHeading);
	column.fmt = nFormat;
	if (nWidth != -1)
	{
		column.mask |= LVCF_WIDTH;
		column.cx = nWidth;
	}
	if (nSubItem != -1)
	{
		column.mask |= LVCF_SUBITEM;
		column.iSubItem = nSubItem;
	}
	return InsertColumn(nCol, &column);
}

int CListCtrl::HitTest(CPoint pt, UINT* pFlags) const
{
	LVHITTESTINFO hitTest = {};
	hitTest.pt = pt;
	const int item = HitTest(&hitTest);
	if (pFlags != nullptr)
	{
		*pFlags = hitTest.flags;
	}
	return item;
}

void CSliderCtrl::GetSelection(int& nMin, int& nMax) const
{
	nMin = static_cast<int>(AFX_CONTROL_SEND(TBM_GETSELSTART, 0, 0));
	nMax = static_cast<int>(AFX_CONTROL_SEND(TBM_GETSELEND, 0, 0));
}

void CSliderCtrl::SetSelection(int nMin, int nMax)
{
	AFX_CONTROL_SEND(TBM_SETSELEND, FALSE, nMax);
	AFX_CONTROL_SEND(TBM_SETSELSTART, FALSE, nMin);
}

void CProgressCtrl::GetRange(int& nLower, int& nUpper) const
{
	PBRANGE range = {};
	AFX_CONTROL_SEND(PBM_GETRANGE, FALSE, &range);
	nLower = range.iLow;
	nUpper = range.iHigh;
}

CImageList* CTabCtrl::GetImageList() const
{
	return CImageList::FromHandle(imageListHandle(AFX_CONTROL_SEND(TCM_GETIMAGELIST, 0, 0)));
}

CImageList* CTabCtrl::SetImageList(CImageList* pImageList)
{
	return CImageList::FromHandle(imageListHandle(AFX_CONTROL_SEND(TCM_SETIMAGELIST, 0, pImageList->GetSafeHandle())));
}

LONG CTabCtrl::InsertItem(int nItem, LPCSTR lpszItem)
{
	TCITEMA item = {};
	item.mask = TCIF_TEXT;
	item.pszText = const_cast<LPSTR>(lpszItem);
	return InsertItem(nItem, &item);
}

LONG CTabCtrl::InsertItem(int nItem, LPCSTR lpszItem, int nImage)
{
	TCITEMA item = {};
	item.mask = TCIF_TEXT | TCIF_IMAGE;
	item.pszText = const_cast<LPSTR>(lpszItem);
	item.iImage = nImage;
	return InsertItem(nItem, &item);
}

void CRichEditCtrl::GetSel(long& nStartChar, long& nEndChar) const
{
	CHARRANGE range = {};
	AFX_CONTROL_SEND(EM_EXGETSEL, 0, &range);
	nStartChar = range.cpMin;
	nEndChar = range.cpMax;
}

void CRichEditCtrl::SetSel(long nStartChar, long nEndChar)
{
	CHARRANGE range = {};
	range.cpMin = nStartChar;
	range.cpMax = nEndChar;
	AFX_CONTROL_SEND(EM_EXSETSEL, 0, &range);
}

CString CRichEditCtrl::GetSelText() const
{
	long start = 0;
	long end = 0;
	GetSel(start, end);
	CString text;
	LPSTR buffer = text.GetBufferSetLength(static_cast<int>(end - start + 1));
	buffer[0] = '\0';
	GetSelText(buffer);
	text.ReleaseBuffer();
	return text;
}

long CRichEditCtrl::GetTextRange(int nFirst, int nLast, CString& refString) const
{
	TEXTRANGEA range = {};
	range.chrg.cpMin = nFirst;
	range.chrg.cpMax = nLast;
	range.lpstrText = refString.GetBuffer(nLast - nFirst + 1);
	const long length = static_cast<long>(AFX_CONTROL_SEND(EM_GETTEXTRANGE, 0, &range));
	refString.ReleaseBuffer(static_cast<int>(length));
	return length;
}

DWORD CRichEditCtrl::GetDefaultCharFormat(CHARFORMATA& cf) const
{
	cf.cbSize = sizeof(cf);
	return static_cast<DWORD>(AFX_CONTROL_SEND(EM_GETCHARFORMAT, SCF_DEFAULT, &cf));
}

DWORD CRichEditCtrl::GetSelectionCharFormat(CHARFORMATA& cf) const
{
	cf.cbSize = sizeof(cf);
	return static_cast<DWORD>(AFX_CONTROL_SEND(EM_GETCHARFORMAT, SCF_SELECTION, &cf));
}

BOOL CRichEditCtrl::SetDefaultCharFormat(CHARFORMATA& cf)
{
	cf.cbSize = sizeof(cf);
	return static_cast<BOOL>(AFX_CONTROL_SEND(EM_SETCHARFORMAT, SCF_DEFAULT, &cf));
}

BOOL CRichEditCtrl::SetSelectionCharFormat(CHARFORMATA& cf)
{
	cf.cbSize = sizeof(cf);
	return static_cast<BOOL>(AFX_CONTROL_SEND(EM_SETCHARFORMAT, SCF_SELECTION, &cf));
}

BOOL CRichEditCtrl::SetWordCharFormat(CHARFORMATA& cf)
{
	cf.cbSize = sizeof(cf);
	return static_cast<BOOL>(AFX_CONTROL_SEND(EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD, &cf));
}
