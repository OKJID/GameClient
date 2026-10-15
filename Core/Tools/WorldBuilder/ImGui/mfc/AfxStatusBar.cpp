#include "AfxInternal.h"

#include "afxext.h"

#include <algorithm>
#include <cstring>

IMPLEMENT_DYNAMIC(CStatusBar, CControlBar)

BEGIN_MESSAGE_MAP(CStatusBar, CControlBar)
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_MESSAGE(WM_SETTEXT, OnSetText)
	ON_MESSAGE(WM_GETTEXT, OnGetText)
	ON_MESSAGE(WM_GETTEXTLENGTH, OnGetTextLength)
	ON_MESSAGE(SB_SETMINHEIGHT, OnSetMinHeight)
END_MESSAGE_MAP()

namespace
{

const UINT kPaneNeedsUpdate = 0x0001;
const int kPaneBorder = 6;

class CStatusCmdUI : public CCmdUI
{
public:
	void Enable(BOOL bOn) override
	{
		m_bEnableChanged = TRUE;
		CStatusBar* statusBar = static_cast<CStatusBar*>(m_pOther);
		UINT style = statusBar->GetPaneStyle(static_cast<int>(m_nIndex)) & ~SBPS_DISABLED;
		statusBar->SetPaneStyle(static_cast<int>(m_nIndex), bOn ? style : style | SBPS_DISABLED);
	}

	void SetCheck(int nCheck) override
	{
		CStatusBar* statusBar = static_cast<CStatusBar*>(m_pOther);
		UINT style = statusBar->GetPaneStyle(static_cast<int>(m_nIndex)) & ~SBPS_POPOUT;
		statusBar->SetPaneStyle(static_cast<int>(m_nIndex), nCheck != 0 ? style | SBPS_POPOUT : style);
	}

	void SetText(LPCSTR lpszText) override
	{
		static_cast<CStatusBar*>(m_pOther)->SetPaneText(static_cast<int>(m_nIndex), lpszText);
	}
};

}

CStatusBar::CStatusBar() :
	m_nMinHeight(0)
{
	m_cyTopBorder = 2;
	m_cxLeftBorder = 0;
	m_cxRightBorder = 0;
	m_cyBottomBorder = 0;
}

CStatusBar::~CStatusBar()
{
}

BOOL CStatusBar::Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID)
{
	return CreateEx(pParentWnd, 0, dwStyle, nID);
}

BOOL CStatusBar::CreateEx(CWnd* pParentWnd, DWORD dwCtrlStyle, DWORD dwStyle, UINT nID)
{
	m_dwStyle = dwStyle & CBRS_ALL;
	DWORD windowStyle = (dwStyle & ~CBRS_ALL) | CCS_NOPARENTALIGN | CCS_NOMOVEY | CCS_NODIVIDER | CCS_NORESIZE | dwCtrlStyle;
	if ((pParentWnd->GetStyle() & WS_THICKFRAME) != 0)
	{
		windowStyle |= SBARS_SIZEGRIP;
	}
	return CWnd::Create(STATUSCLASSNAME, nullptr, windowStyle, CRect(0, 0, 0, 0), pParentWnd, nID);
}

BOOL CStatusBar::PreCreateWindow(CREATESTRUCT& cs)
{
	if ((m_dwStyle & (CBRS_ALIGN_ANY | CBRS_BORDER_ANY)) == CBRS_BOTTOM)
	{
		m_dwStyle &= ~(CBRS_BORDER_ANY | CBRS_BORDER_3D);
	}
	return CControlBar::PreCreateWindow(cs);
}

BOOL CStatusBar::SetIndicators(const UINT* lpIDArray, int nIDCount)
{
	m_panes.assign(static_cast<size_t>(nIDCount), Pane());
	m_nCount = nIDCount;
	BOOL result = TRUE;
	if (lpIDArray != nullptr)
	{
		loadIndicators(lpIDArray, result);
	}
	UpdateAllPanes(TRUE, TRUE);
	return result;
}

void CStatusBar::loadIndicators(const UINT* lpIDArray, BOOL& bResult)
{
	CClientDC dc(this);
	const HFONT font = reinterpret_cast<HFONT>(SendMessage(WM_GETFONT));
	const HGDIOBJ oldFont = font != nullptr ? ::SelectObject(dc.m_hDC, font) : nullptr;
	for (int index = 0; index < m_nCount; ++index)
	{
		Pane& pane = m_panes[static_cast<size_t>(index)];
		pane.nID = lpIDArray[index];
		pane.nFlags |= kPaneNeedsUpdate;
		if (pane.nID == 0)
		{
			pane.cxText = ::GetSystemMetrics(SM_CXSCREEN) / 4;
			pane.nStyle |= index == 0 ? SBPS_STRETCH | SBPS_NOBORDERS : 0;
			continue;
		}
		if (!pane.strText.LoadString(pane.nID))
		{
			bResult = FALSE;
			break;
		}
		pane.cxText = dc.GetTextExtent(pane.strText).cx;
		if (!SetPaneText(index, pane.strText, FALSE))
		{
			bResult = FALSE;
			break;
		}
	}
	if (oldFont != nullptr)
	{
		::SelectObject(dc.m_hDC, oldFont);
	}
}

std::vector<int> CStatusBar::paneRightEdges()
{
	CRect rect;
	GetWindowRect(&rect);
	rect.OffsetRect(-rect.left, -rect.top);
	CalcInsideRect(rect, TRUE);
	int borders[3] = {};
	DefWindowProc(SB_GETBORDERS, 0, reinterpret_cast<LPARAM>(borders));

	int extra = rect.Width() + borders[2];
	int stretchyCount = 0;
	for (const Pane& pane : m_panes)
	{
		stretchyCount += (pane.nStyle & SBPS_STRETCH) != 0 ? 1 : 0;
		extra -= pane.cxText + kPaneBorder + borders[2];
	}

	std::vector<int> rights;
	int right = borders[0];
	for (const Pane& pane : m_panes)
	{
		right += pane.cxText + kPaneBorder;
		if ((pane.nStyle & SBPS_STRETCH) != 0 && extra > 0)
		{
			const int added = extra / stretchyCount;
			right += added;
			--stretchyCount;
			extra -= added;
		}
		rights.push_back(right);
		right += borders[2];
	}
	return rights;
}

void CStatusBar::UpdateAllPanes(BOOL bUpdateRects, BOOL bUpdateText)
{
	if (bUpdateRects)
	{
		std::vector<int> rights = paneRightEdges();
		DefWindowProc(SB_SETPARTS, static_cast<WPARAM>(rights.size()), reinterpret_cast<LPARAM>(rights.data()));
	}
	if (!bUpdateText)
	{
		return;
	}
	for (int index = 0; index < m_nCount; ++index)
	{
		const Pane& pane = m_panes[static_cast<size_t>(index)];
		if ((pane.nFlags & kPaneNeedsUpdate) != 0)
		{
			SetPaneText(index, pane.strText);
		}
	}
}

int CStatusBar::CommandToIndex(UINT nIDFind) const
{
	for (int index = 0; index < m_nCount; ++index)
	{
		if (m_panes[static_cast<size_t>(index)].nID == nIDFind)
		{
			return index;
		}
	}
	return -1;
}

UINT CStatusBar::GetItemID(int nIndex) const
{
	return m_panes[static_cast<size_t>(nIndex)].nID;
}

void CStatusBar::GetItemRect(int nIndex, LPRECT lpRect) const
{
	if (!const_cast<CStatusBar*>(this)->DefWindowProc(SB_GETRECT, static_cast<WPARAM>(nIndex), reinterpret_cast<LPARAM>(lpRect)))
	{
		::SetRectEmpty(lpRect);
	}
}

void CStatusBar::GetPaneText(int nIndex, CString& rString) const
{
	rString = m_panes[static_cast<size_t>(nIndex)].strText;
}

CString CStatusBar::GetPaneText(int nIndex) const
{
	return m_panes[static_cast<size_t>(nIndex)].strText;
}

BOOL CStatusBar::SetPaneText(int nIndex, LPCSTR lpszNewText, BOOL bUpdate)
{
	Pane& pane = m_panes[static_cast<size_t>(nIndex)];
	const bool isUnchanged = (lpszNewText == nullptr && pane.strText.IsEmpty()) || (lpszNewText != nullptr && pane.strText.Compare(lpszNewText) == 0);
	if ((pane.nFlags & kPaneNeedsUpdate) == 0 && isUnchanged)
	{
		return TRUE;
	}
	pane.strText = lpszNewText != nullptr ? lpszNewText : "";
	if (!bUpdate)
	{
		pane.nFlags |= kPaneNeedsUpdate;
		return TRUE;
	}
	pane.nFlags &= ~kPaneNeedsUpdate;
	const LPCSTR shownText = (pane.nStyle & SBPS_DISABLED) != 0 ? nullptr : static_cast<LPCSTR>(pane.strText);
	DefWindowProc(SB_SETTEXT, static_cast<WORD>(pane.nStyle) | static_cast<WPARAM>(nIndex), reinterpret_cast<LPARAM>(shownText));
	return TRUE;
}

void CStatusBar::GetPaneInfo(int nIndex, UINT& nID, UINT& nStyle, int& cxWidth) const
{
	const Pane& pane = m_panes[static_cast<size_t>(nIndex)];
	nID = pane.nID;
	nStyle = pane.nStyle;
	cxWidth = pane.cxText;
}

void CStatusBar::SetPaneInfo(int nIndex, UINT nID, UINT nStyle, int cxWidth)
{
	Pane& pane = m_panes[static_cast<size_t>(nIndex)];
	bool changesLayout = false;
	pane.nID = nID;
	if (pane.nStyle != nStyle)
	{
		changesLayout = ((pane.nStyle ^ nStyle) & SBPS_STRETCH) != 0;
		pane.nStyle = nStyle;
		if (!changesLayout)
		{
			pane.nFlags |= kPaneNeedsUpdate;
			SetPaneText(nIndex, pane.strText);
		}
	}
	if (cxWidth != pane.cxText)
	{
		pane.cxText = cxWidth;
		changesLayout = true;
	}
	if (changesLayout)
	{
		UpdateAllPanes(TRUE, FALSE);
	}
}

UINT CStatusBar::GetPaneStyle(int nIndex) const
{
	return m_panes[static_cast<size_t>(nIndex)].nStyle;
}

void CStatusBar::SetPaneStyle(int nIndex, UINT nStyle)
{
	Pane& pane = m_panes[static_cast<size_t>(nIndex)];
	if (pane.nStyle == nStyle)
	{
		return;
	}
	const bool changesStretch = ((pane.nStyle ^ nStyle) & SBPS_STRETCH) != 0;
	pane.nStyle = nStyle;
	if (changesStretch)
	{
		UpdateAllPanes(TRUE, FALSE);
	}
	pane.nFlags |= kPaneNeedsUpdate;
	SetPaneText(nIndex, pane.strText);
}

CSize CStatusBar::CalcFixedLayout(BOOL, BOOL bHorz)
{
	TEXTMETRICA metrics = {};
	{
		CClientDC dc(this);
		const HFONT font = reinterpret_cast<HFONT>(SendMessage(WM_GETFONT));
		const HGDIOBJ oldFont = font != nullptr ? ::SelectObject(dc.m_hDC, font) : nullptr;
		dc.GetTextMetrics(&metrics);
		if (oldFont != nullptr)
		{
			::SelectObject(dc.m_hDC, oldFont);
		}
	}
	CRect inside(0, 0, 0, 0);
	CalcInsideRect(inside, bHorz);
	int borders[3] = {};
	DefWindowProc(SB_GETBORDERS, 0, reinterpret_cast<LPARAM>(borders));
	const int height = metrics.tmHeight - metrics.tmInternalLeading - 1 + borders[1] * 2 + ::GetSystemMetrics(SM_CYBORDER) * 2 - inside.Height();
	return CSize(32767, std::max(height, m_nMinHeight));
}

void CStatusBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	CStatusCmdUI state;
	state.m_pOther = this;
	state.m_nIndexMax = static_cast<UINT>(m_nCount);
	for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax; ++state.m_nIndex)
	{
		state.m_nID = m_panes[state.m_nIndex].nID;
		if (CWnd::OnCmdMsg(state.m_nID, static_cast<int>(CN_UPDATE_COMMAND_UI), &state, nullptr))
		{
			continue;
		}
		state.DoUpdate(pTarget, FALSE);
	}
	UpdateDialogControls(pTarget, bDisableIfNoHndler);
}

void CStatusBar::OnNcCalcSize(BOOL, NCCALCSIZE_PARAMS* lpncsp)
{
	CRect inside(0, 0, 0, 0);
	CControlBar::CalcInsideRect(inside, TRUE);
	lpncsp->rgrc[0].left += inside.left;
	lpncsp->rgrc[0].top += inside.top - 2;
	lpncsp->rgrc[0].right += inside.right;
	lpncsp->rgrc[0].bottom += inside.bottom;
}

void CStatusBar::OnNcPaint()
{
	EraseNonClient();
}

void CStatusBar::OnPaint()
{
	UpdateAllPanes(FALSE, TRUE);
	Default();
}

void CStatusBar::OnSize(UINT nType, int cx, int cy)
{
	CControlBar::OnSize(nType, cx, cy);
	UpdateAllPanes(TRUE, FALSE);
}

LRESULT CStatusBar::OnSetText(WPARAM, LPARAM lParam)
{
	const int index = CommandToIndex(0);
	if (index < 0)
	{
		return -1;
	}
	return SetPaneText(index, reinterpret_cast<LPCSTR>(lParam)) ? 0 : -1;
}

LRESULT CStatusBar::OnGetText(WPARAM wParam, LPARAM lParam)
{
	const int maxLength = static_cast<int>(wParam);
	if (maxLength == 0)
	{
		return 0;
	}
	LPSTR destination = reinterpret_cast<LPSTR>(lParam);
	int length = 0;
	const int index = CommandToIndex(0);
	if (index >= 0)
	{
		const CString& text = m_panes[static_cast<size_t>(index)].strText;
		length = std::min(text.GetLength(), maxLength - 1);
		memcpy(destination, static_cast<LPCSTR>(text), static_cast<size_t>(length));
	}
	destination[length] = '\0';
	return length + 1;
}

LRESULT CStatusBar::OnGetTextLength(WPARAM, LPARAM)
{
	const int index = CommandToIndex(0);
	return index >= 0 ? m_panes[static_cast<size_t>(index)].strText.GetLength() : 0;
}

LRESULT CStatusBar::OnSetMinHeight(WPARAM wParam, LPARAM)
{
	const LRESULT result = Default();
	m_nMinHeight = static_cast<int>(wParam);
	return result;
}
