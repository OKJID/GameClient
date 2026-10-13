#include "AfxInternal.h"

#include "afxext.h"

#include "Win32Resources.h"

#include <algorithm>
#include <cstring>

IMPLEMENT_DYNAMIC(CToolBar, CControlBar)

BEGIN_MESSAGE_MAP(CToolBar, CControlBar)
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_WM_PAINT()
END_MESSAGE_MAP()

namespace
{

const int kFlatSeparatorWidth = 6;
const int kSeparatorWidth = 8;
const int kImagePaddingX = 7;
const int kImagePaddingY = 7;

class CToolCmdUI : public CCmdUI
{
public:
	void Enable(BOOL bOn) override
	{
		m_bEnableChanged = TRUE;
		CToolBar* toolBar = static_cast<CToolBar*>(m_pOther);
		UINT style = toolBar->GetButtonStyle(static_cast<int>(m_nIndex)) & ~TBBS_DISABLED;
		if (!bOn)
		{
			style |= TBBS_DISABLED;
			style &= ~TBBS_PRESSED;
		}
		toolBar->SetButtonStyle(static_cast<int>(m_nIndex), style);
	}

	void SetCheck(int nCheck) override
	{
		CToolBar* toolBar = static_cast<CToolBar*>(m_pOther);
		UINT style = toolBar->GetButtonStyle(static_cast<int>(m_nIndex)) & ~(TBBS_CHECKED | TBBS_INDETERMINATE);
		if (nCheck == 1)
		{
			style |= TBBS_CHECKED;
		}
		else if (nCheck == 2)
		{
			style |= TBBS_INDETERMINATE;
		}
		toolBar->SetButtonStyle(static_cast<int>(m_nIndex), style | TBBS_CHECKBOX);
	}

	void SetText(LPCSTR) override
	{
	}
};

}

CToolBar::CToolBar() :
	m_hbmImageWell(nullptr),
	m_sizeButton(23, 22),
	m_sizeImage(16, 15)
{
	m_cyTopBorder = 3;
	m_cyBottomBorder = 3;
}

CToolBar::~CToolBar()
{
	if (m_hbmImageWell != nullptr)
	{
		::DeleteObject(m_hbmImageWell);
	}
}

LRESULT CToolBar::toolbarMessage(UINT message, WPARAM wParam, LPARAM lParam) const
{
	return const_cast<CToolBar*>(this)->DefWindowProc(message, wParam, lParam);
}

BOOL CToolBar::Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID)
{
	return CreateEx(pParentWnd, 0, dwStyle, CRect(m_cxLeftBorder, m_cyTopBorder, m_cxRightBorder, m_cyBottomBorder), nID);
}

BOOL CToolBar::CreateEx(CWnd* pParentWnd, DWORD dwCtrlStyle, DWORD dwStyle, CRect rcBorders, UINT nID)
{
	m_cxLeftBorder = rcBorders.left;
	m_cyTopBorder = rcBorders.top;
	m_cxRightBorder = rcBorders.right;
	m_cyBottomBorder = rcBorders.bottom;
	m_dwStyle = dwStyle & CBRS_ALL;
	if (nID == AFX_IDW_TOOLBAR)
	{
		m_dwStyle |= CBRS_HIDE_INPLACE;
	}

	DWORD windowStyle = (dwStyle & ~CBRS_ALL) | CCS_NOPARENTALIGN | CCS_NOMOVEY | CCS_NODIVIDER | CCS_NORESIZE | dwCtrlStyle;
	if ((m_dwStyle & CBRS_TOOLTIPS) != 0)
	{
		windowStyle |= TBSTYLE_TOOLTIPS;
	}
	if (!CWnd::Create(TOOLBARCLASSNAME, nullptr, windowStyle, CRect(0, 0, 0, 0), pParentWnd, nID))
	{
		return FALSE;
	}
	DefWindowProc(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SetSizes(m_sizeButton, m_sizeImage);
	return TRUE;
}

void CToolBar::SetSizes(SIZE sizeButton, SIZE sizeImage)
{
	m_sizeButton = sizeButton;
	m_sizeImage = sizeImage;
	if (!::IsWindow(m_hWnd))
	{
		return;
	}
	SendMessage(TB_SETBITMAPSIZE, 0, MAKELONG(sizeImage.cx, sizeImage.cy));
	SendMessage(TB_SETBUTTONSIZE, 0, MAKELONG(sizeButton.cx, sizeButton.cy));
	Invalidate();
}

BOOL CToolBar::LoadToolBar(UINT nIDResource)
{
	return LoadToolBar(MAKEINTRESOURCE(nIDResource));
}

BOOL CToolBar::LoadToolBar(LPCSTR lpszResourceName)
{
	UINT id = 0;
	const Win32ToolbarTemplate* toolbarTemplate =
		Win32Resources::instance().resolveName(lpszResourceName, id) ? Win32Resources::instance().toolbar(id) : nullptr;
	if (toolbarTemplate == nullptr)
	{
		return FALSE;
	}
	const std::vector<UINT> items(toolbarTemplate->buttons.begin(), toolbarTemplate->buttons.end());
	if (!SetButtons(items.data(), static_cast<int>(items.size())))
	{
		return FALSE;
	}
	const CSize sizeImage(toolbarTemplate->buttonWidth, toolbarTemplate->buttonHeight);
	SetSizes(CSize(sizeImage.cx + kImagePaddingX, sizeImage.cy + kImagePaddingY), sizeImage);
	return LoadBitmap(lpszResourceName);
}

BOOL CToolBar::LoadBitmap(UINT nIDResource)
{
	return LoadBitmap(MAKEINTRESOURCE(nIDResource));
}

BOOL CToolBar::LoadBitmap(LPCSTR lpszResourceName)
{
	const HBITMAP bitmap = static_cast<HBITMAP>(::LoadImageA(AfxGetResourceHandle(), lpszResourceName, IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS));
	return bitmap != nullptr ? AddReplaceBitmap(bitmap) : FALSE;
}

BOOL CToolBar::SetBitmap(HBITMAP hbmImageWell)
{
	return AddReplaceBitmap(hbmImageWell);
}

BOOL CToolBar::AddReplaceBitmap(HBITMAP hbmImageWell)
{
	BITMAP bitmap = {};
	if (::GetObjectA(hbmImageWell, sizeof(bitmap), &bitmap) == 0)
	{
		return FALSE;
	}
	const int imageCount = bitmap.bmWidth / std::max<LONG>(1, m_sizeImage.cx);
	BOOL result = FALSE;
	if (m_hbmImageWell == nullptr)
	{
		TBADDBITMAP addBitmap = { nullptr, reinterpret_cast<UINT_PTR>(hbmImageWell) };
		result = DefWindowProc(TB_ADDBITMAP, static_cast<WPARAM>(imageCount), reinterpret_cast<LPARAM>(&addBitmap)) == 0 ? TRUE : FALSE;
	}
	else
	{
		TBREPLACEBITMAP replaceBitmap = { nullptr, reinterpret_cast<UINT_PTR>(m_hbmImageWell), nullptr, reinterpret_cast<UINT_PTR>(hbmImageWell),
			imageCount };
		result = static_cast<BOOL>(DefWindowProc(TB_REPLACEBITMAP, 0, reinterpret_cast<LPARAM>(&replaceBitmap)));
	}
	if (!result)
	{
		return FALSE;
	}
	if (m_hbmImageWell != nullptr)
	{
		::DeleteObject(m_hbmImageWell);
	}
	m_hbmImageWell = hbmImageWell;
	return TRUE;
}

BOOL CToolBar::SetButtons(const UINT* lpIDArray, int nIDCount)
{
	for (int count = static_cast<int>(DefWindowProc(TB_BUTTONCOUNT, 0, 0)); count > 0; --count)
	{
		DefWindowProc(TB_DELETEBUTTON, 0, 0);
	}

	const int separatorWidth = (GetStyle() & TBSTYLE_FLAT) != 0 ? kFlatSeparatorWidth : kSeparatorWidth;
	int image = 0;
	for (int index = 0; index < nIDCount; ++index)
	{
		TBBUTTON button = {};
		button.iString = -1;
		button.fsState = TBSTATE_ENABLED;
		button.idCommand = lpIDArray != nullptr ? static_cast<int>(lpIDArray[index]) : 0;
		const bool isSeparator = lpIDArray != nullptr && button.idCommand == 0;
		button.fsStyle = isSeparator ? TBSTYLE_SEP : TBSTYLE_BUTTON;
		button.iBitmap = isSeparator ? separatorWidth : image++;
		if (!DefWindowProc(TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&button)))
		{
			return FALSE;
		}
	}
	m_nCount = static_cast<int>(DefWindowProc(TB_BUTTONCOUNT, 0, 0));
	return TRUE;
}

int CToolBar::CommandToIndex(UINT nIDFind) const
{
	return static_cast<int>(toolbarMessage(TB_COMMANDTOINDEX, nIDFind));
}

UINT CToolBar::GetItemID(int nIndex) const
{
	TBBUTTON button = {};
	_GetButton(nIndex, &button);
	return static_cast<UINT>(button.idCommand);
}

void CToolBar::GetItemRect(int nIndex, LPRECT lpRect) const
{
	if (!toolbarMessage(TB_GETITEMRECT, static_cast<WPARAM>(nIndex), reinterpret_cast<LPARAM>(lpRect)))
	{
		::SetRectEmpty(lpRect);
	}
}

UINT CToolBar::GetButtonStyle(int nIndex) const
{
	TBBUTTON button = {};
	_GetButton(nIndex, &button);
	return static_cast<UINT>(MAKELONG(button.fsStyle, button.fsState));
}

void CToolBar::SetButtonStyle(int nIndex, UINT nStyle)
{
	TBBUTTON button = {};
	_GetButton(nIndex, &button);
	if (button.fsStyle == static_cast<BYTE>(LOWORD(nStyle)) && button.fsState == static_cast<BYTE>(HIWORD(nStyle)))
	{
		return;
	}
	button.fsStyle = static_cast<BYTE>(LOWORD(nStyle));
	button.fsState = static_cast<BYTE>(HIWORD(nStyle));
	_SetButton(nIndex, &button);
}

void CToolBar::GetButtonInfo(int nIndex, UINT& nID, UINT& nStyle, int& iImage) const
{
	TBBUTTON button = {};
	_GetButton(nIndex, &button);
	nID = static_cast<UINT>(button.idCommand);
	nStyle = static_cast<UINT>(MAKELONG(button.fsStyle, button.fsState));
	iImage = button.iBitmap;
}

void CToolBar::SetButtonInfo(int nIndex, UINT nID, UINT nStyle, int iImage)
{
	TBBUTTON button = {};
	_GetButton(nIndex, &button);
	const TBBUTTON saved = button;
	button.idCommand = static_cast<int>(nID);
	button.iBitmap = iImage;
	button.fsStyle = static_cast<BYTE>(LOWORD(nStyle));
	button.fsState = static_cast<BYTE>(HIWORD(nStyle));
	if (memcmp(&saved, &button, sizeof(TBBUTTON)) != 0)
	{
		_SetButton(nIndex, &button);
	}
}

void CToolBar::_GetButton(int nIndex, TBBUTTON* pButton) const
{
	toolbarMessage(TB_GETBUTTON, static_cast<WPARAM>(nIndex), reinterpret_cast<LPARAM>(pButton));
	pButton->fsState ^= TBSTATE_ENABLED;
}

void CToolBar::_SetButton(int nIndex, TBBUTTON* pButton)
{
	TBBUTTON current = {};
	DefWindowProc(TB_GETBUTTON, static_cast<WPARAM>(nIndex), reinterpret_cast<LPARAM>(&current));
	pButton->fsState ^= TBSTATE_ENABLED;
	if (memcmp(pButton, &current, sizeof(TBBUTTON)) == 0)
	{
		return;
	}
	const DWORD style = GetStyle();
	ModifyStyle(WS_VISIBLE, 0);
	DefWindowProc(TB_DELETEBUTTON, static_cast<WPARAM>(nIndex), 0);
	DefWindowProc(TB_INSERTBUTTON, static_cast<WPARAM>(nIndex), reinterpret_cast<LPARAM>(pButton));
	ModifyStyle(0, style & WS_VISIBLE);
	Invalidate();
}

CSize CToolBar::CalcSize(const TBBUTTON* pData, int nCount) const
{
	const bool isFlat = (GetStyle() & TBSTYLE_FLAT) != 0;
	CPoint current(0, 0);
	CSize size(0, 0);
	for (int index = 0; index < nCount; ++index)
	{
		const TBBUTTON& button = pData[index];
		if ((button.fsState & TBSTATE_HIDDEN) != 0)
		{
			continue;
		}
		const bool isSeparator = (button.fsStyle & TBSTYLE_SEP) != 0;
		const bool wraps = (button.fsState & TBSTATE_WRAP) != 0;
		const int separatorHeight = isFlat ? button.iBitmap : button.iBitmap * 2 / 3;
		if (!isSeparator)
		{
			size.cx = std::max<LONG>(current.x + m_sizeButton.cx, size.cx);
			size.cy = std::max<LONG>(current.y + m_sizeButton.cy, size.cy);
		}
		else if (wraps)
		{
			size.cy = std::max<LONG>(current.y + m_sizeButton.cy + separatorHeight, size.cy);
		}
		else
		{
			size.cx = std::max<LONG>(current.x + button.iBitmap, size.cx);
		}
		current.x += isSeparator ? button.iBitmap : m_sizeButton.cx;
		if (wraps)
		{
			current.x = 0;
			current.y += m_sizeButton.cy + (isSeparator ? separatorHeight : 0);
		}
	}
	return size;
}

CSize CToolBar::CalcLayout(DWORD dwMode)
{
	const int count = static_cast<int>(DefWindowProc(TB_BUTTONCOUNT, 0, 0));
	if (count == 0)
	{
		return CSize(0, 0);
	}
	std::vector<TBBUTTON> buttons(static_cast<size_t>(count));
	for (int index = 0; index < count; ++index)
	{
		_GetButton(index, &buttons[static_cast<size_t>(index)]);
	}
	CSize size = CalcSize(buttons.data(), count);
	CRect inside(0, 0, 0, 0);
	CalcInsideRect(inside, (dwMode & LM_HORZ) != 0);
	size.cy -= inside.Height();
	size.cx -= inside.Width();
	const CSize fixed = CControlBar::CalcFixedLayout((dwMode & LM_STRETCH) != 0, (dwMode & LM_HORZ) != 0);
	return CSize(std::max(size.cx, fixed.cx), std::max(size.cy, fixed.cy));
}

CSize CToolBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	return CalcLayout((bStretch ? LM_STRETCH : 0) | (bHorz ? LM_HORZ : 0));
}

CSize CToolBar::CalcDynamicLayout(int nLength, DWORD nMode)
{
	const bool isDocked = (nMode & (LM_HORZDOCK | LM_VERTDOCK)) != 0;
	if (nLength == -1 && (nMode & (LM_MRUWIDTH | LM_COMMIT)) == 0 && isDocked)
	{
		return CalcFixedLayout((nMode & LM_STRETCH) != 0, (nMode & LM_HORZDOCK) != 0);
	}
	return CalcLayout(nMode);
}

void CToolBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	CToolCmdUI state;
	state.m_pOther = this;
	state.m_nIndexMax = static_cast<UINT>(DefWindowProc(TB_BUTTONCOUNT, 0, 0));
	for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax; ++state.m_nIndex)
	{
		TBBUTTON button = {};
		_GetButton(static_cast<int>(state.m_nIndex), &button);
		state.m_nID = static_cast<UINT>(button.idCommand);
		if ((button.fsStyle & TBSTYLE_SEP) != 0)
		{
			continue;
		}
		const int reflectedUpdate = static_cast<int>(MAKELONG(static_cast<WORD>(CN_UPDATE_COMMAND_UI), WM_COMMAND + WM_REFLECT_BASE));
		if (CWnd::OnCmdMsg(0, reflectedUpdate, &state, nullptr) || CWnd::OnCmdMsg(state.m_nID, static_cast<int>(CN_UPDATE_COMMAND_UI), &state, nullptr))
		{
			continue;
		}
		state.DoUpdate(pTarget, bDisableIfNoHndler);
	}
	UpdateDialogControls(pTarget, bDisableIfNoHndler);
}

void CToolBar::OnBarStyleChange(DWORD dwOldStyle, DWORD dwNewStyle)
{
	if (m_hWnd == nullptr)
	{
		return;
	}
	if ((dwOldStyle & CBRS_BORDER_ANY) != (dwNewStyle & CBRS_BORDER_ANY))
	{
		SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}
	if ((dwOldStyle & CBRS_TOOLTIPS) != (dwNewStyle & CBRS_TOOLTIPS))
	{
		ModifyStyle((dwNewStyle & CBRS_TOOLTIPS) != 0 ? 0 : TBSTYLE_TOOLTIPS, (dwNewStyle & CBRS_TOOLTIPS) != 0 ? TBSTYLE_TOOLTIPS : 0);
	}
}

int CToolBar::OnToolHitTest(CPoint point) const
{
	const int childHit = CControlBar::OnToolHitTest(point);
	if (childHit != -1)
	{
		return childHit;
	}
	const int count = static_cast<int>(toolbarMessage(TB_BUTTONCOUNT));
	for (int index = 0; index < count; ++index)
	{
		CRect rect;
		TBBUTTON button = {};
		if (!toolbarMessage(TB_GETITEMRECT, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(&rect)))
		{
			continue;
		}
		++rect.bottom;
		++rect.right;
		if (rect.PtInRect(point) && toolbarMessage(TB_GETBUTTON, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(&button))
			&& (button.fsStyle & TBSTYLE_SEP) == 0)
		{
			const UINT id = GetItemID(index);
			return id != 0 ? static_cast<int>(id) : -1;
		}
	}
	return -1;
}

void CToolBar::OnNcCalcSize(BOOL, NCCALCSIZE_PARAMS* lpncsp)
{
	CRect inside(0, 0, 0, 0);
	CalcInsideRect(inside, (m_dwStyle & CBRS_ORIENT_HORZ) != 0);
	lpncsp->rgrc[0].left += inside.left;
	lpncsp->rgrc[0].top += inside.top;
	lpncsp->rgrc[0].right += inside.right;
	lpncsp->rgrc[0].bottom += inside.bottom;
}

void CToolBar::OnNcPaint()
{
	EraseNonClient();
}

void CToolBar::OnPaint()
{
	Default();
}
