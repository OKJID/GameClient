#include "AfxInternal.h"

#include "afxext.h"

#include <algorithm>

IMPLEMENT_DYNAMIC(CControlBar, CWnd)

BEGIN_MESSAGE_MAP(CControlBar, CWnd)
	ON_WM_TIMER()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_MESSAGE(WM_SIZEPARENT, OnSizeParent)
	ON_MESSAGE(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
END_MESSAGE_MAP()

namespace
{

const int kBorder = 1;
const int kDoubleBorder = 2;
const int kGripper = 3;
const int kGripperBorder = 2;
const UINT kWaitTimer = 0xE000;
const UINT kCheckTimer = 0xE001;

bool isMouseMessage(UINT message)
{
	return (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST) || (message >= WM_NCMOUSEMOVE && message <= WM_NCRBUTTONDBLCLK);
}

DWORD win4BorderStyle(DWORD dwStyle)
{
	switch (dwStyle & (CBRS_BORDER_ANY | CBRS_ALIGN_ANY))
	{
	case CBRS_LEFT:
	case CBRS_RIGHT:
		return CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM;
	case CBRS_TOP:
		return CBRS_BORDER_TOP;
	case CBRS_BOTTOM:
		return CBRS_BORDER_BOTTOM;
	default:
		return 0;
	}
}

}

CControlBar::CControlBar() :
	m_bAutoDelete(FALSE),
	m_dwStyle(0),
	m_dwDockStyle(0),
	m_pDockSite(nullptr),
	m_pFloatingFrame(nullptr),
	m_cxLeftBorder(6),
	m_cxRightBorder(6),
	m_cyTopBorder(1),
	m_cyBottomBorder(1),
	m_cxDefaultGap(2),
	m_nStateFlags(0),
	m_nCount(0)
{
}

CControlBar::~CControlBar()
{
	DestroyWindow();
	if (m_pDockSite != nullptr)
	{
		m_pDockSite->RemoveControlBar(this);
	}
	AfxThreadState& state = AfxGetThreadState();
	if (state.pLastStatus == this)
	{
		state.pLastStatus = nullptr;
		state.nLastStatus = -1;
	}
}

BOOL CControlBar::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs))
	{
		return FALSE;
	}
	cs.style |= WS_CLIPSIBLINGS;
	const DWORD borders = (m_dwStyle & CBRS_BORDER_3D) == 0 ? win4BorderStyle(m_dwStyle) : 0;
	if (borders != 0)
	{
		m_dwStyle &= ~CBRS_BORDER_ANY;
		m_dwStyle |= borders | CBRS_BORDER_3D;
	}
	return TRUE;
}

void CControlBar::SetBarStyle(DWORD dwStyle)
{
	if (m_dwStyle == dwStyle)
	{
		return;
	}
	const DWORD oldStyle = m_dwStyle;
	m_dwStyle = dwStyle;
	OnBarStyleChange(oldStyle, dwStyle);
}

void CControlBar::OnBarStyleChange(DWORD, DWORD)
{
}

void CControlBar::EnableDocking(DWORD dwDockStyle)
{
	m_dwDockStyle = dwDockStyle;
	if (m_hWndOwner == nullptr)
	{
		m_hWndOwner = ::GetParent(m_hWnd);
	}
}

CFrameWnd* CControlBar::GetDockingFrame() const
{
	CFrameWnd* frame = GetParentFrame();
	return frame != nullptr ? frame : m_pDockSite;
}

BOOL CControlBar::IsFloating() const
{
	return m_pFloatingFrame != nullptr ? TRUE : FALSE;
}

BOOL CControlBar::IsHorizontal() const
{
	return (m_dwStyle & CBRS_ORIENT_HORZ) != 0 ? TRUE : FALSE;
}

void CControlBar::DelayShow(BOOL bShow)
{
	m_nStateFlags &= ~(delayHide | delayShow);
	const bool isVisible = (GetStyle() & WS_VISIBLE) != 0;
	if (bShow && !isVisible)
	{
		m_nStateFlags |= delayShow;
	}
	else if (!bShow && isVisible)
	{
		m_nStateFlags |= delayHide;
	}
}

BOOL CControlBar::IsVisible() const
{
	if ((m_nStateFlags & delayHide) != 0)
	{
		return FALSE;
	}
	return (m_nStateFlags & delayShow) != 0 || (GetStyle() & WS_VISIBLE) != 0 ? TRUE : FALSE;
}

CSize CControlBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	return CSize(bStretch && bHorz ? 32767 : 0, bStretch && !bHorz ? 32767 : 0);
}

CSize CControlBar::CalcDynamicLayout(int, DWORD nMode)
{
	return CalcFixedLayout((nMode & LM_STRETCH) != 0, (nMode & LM_HORZ) != 0);
}

void CControlBar::CalcInsideRect(CRect& rect, BOOL bHorz) const
{
	rect.left += (m_dwStyle & CBRS_BORDER_LEFT) != 0 ? kDoubleBorder : 0;
	rect.top += (m_dwStyle & CBRS_BORDER_TOP) != 0 ? kDoubleBorder : 0;
	rect.right -= (m_dwStyle & CBRS_BORDER_RIGHT) != 0 ? kDoubleBorder : 0;
	rect.bottom -= (m_dwStyle & CBRS_BORDER_BOTTOM) != 0 ? kDoubleBorder : 0;
	const bool hasGripper = (m_dwStyle & (CBRS_GRIPPER | CBRS_FLOATING)) == CBRS_GRIPPER;
	if (bHorz)
	{
		rect.left += m_cxLeftBorder + (hasGripper ? kGripperBorder + kGripper + kGripperBorder : 0);
		rect.top += m_cyTopBorder;
		rect.right -= m_cxRightBorder;
		rect.bottom -= m_cyBottomBorder;
		return;
	}
	rect.left += m_cyTopBorder;
	rect.top += m_cxLeftBorder + (hasGripper ? kGripperBorder + kGripper + kGripperBorder : 0);
	rect.right -= m_cyBottomBorder;
	rect.bottom -= m_cxRightBorder;
}

void CControlBar::DrawBorders(CDC* pDC, CRect& rect)
{
	const DWORD style = m_dwStyle;
	if ((style & CBRS_BORDER_ANY) == 0)
	{
		return;
	}
	CRect inner = rect;
	CRect outer = rect;
	if ((style & CBRS_BORDER_3D) != 0)
	{
		inner.right -= kBorder;
		inner.bottom -= kBorder;
	}
	outer.top += (style & CBRS_BORDER_TOP) != 0 ? kDoubleBorder : 0;
	outer.bottom -= (style & CBRS_BORDER_BOTTOM) != 0 ? kDoubleBorder : 0;

	const COLORREF shadow = ::GetSysColor(COLOR_BTNSHADOW);
	if ((style & CBRS_BORDER_LEFT) != 0)
	{
		pDC->FillSolidRect(0, outer.top, kBorder, outer.Height(), shadow);
	}
	if ((style & CBRS_BORDER_TOP) != 0)
	{
		pDC->FillSolidRect(0, 0, rect.right, kBorder, shadow);
	}
	if ((style & CBRS_BORDER_RIGHT) != 0)
	{
		pDC->FillSolidRect(inner.right, outer.top, -kBorder, outer.Height(), shadow);
	}
	if ((style & CBRS_BORDER_BOTTOM) != 0)
	{
		pDC->FillSolidRect(0, inner.bottom, rect.right, -kBorder, shadow);
	}

	if ((style & CBRS_BORDER_3D) != 0)
	{
		drawBorderHighlights(pDC, rect, outer);
	}

	rect.left += (style & CBRS_BORDER_LEFT) != 0 ? kDoubleBorder : 0;
	rect.top += (style & CBRS_BORDER_TOP) != 0 ? kDoubleBorder : 0;
	rect.right -= (style & CBRS_BORDER_RIGHT) != 0 ? kDoubleBorder : 0;
	rect.bottom -= (style & CBRS_BORDER_BOTTOM) != 0 ? kDoubleBorder : 0;
}

void CControlBar::drawBorderHighlights(CDC* pDC, const CRect& rect, const CRect& outer) const
{
	const COLORREF highlight = ::GetSysColor(COLOR_BTNHIGHLIGHT);
	if ((m_dwStyle & CBRS_BORDER_LEFT) != 0)
	{
		pDC->FillSolidRect(1, outer.top, kBorder, outer.Height(), highlight);
	}
	if ((m_dwStyle & CBRS_BORDER_TOP) != 0)
	{
		pDC->FillSolidRect(0, 1, rect.right, kBorder, highlight);
	}
	if ((m_dwStyle & CBRS_BORDER_RIGHT) != 0)
	{
		pDC->FillSolidRect(rect.right, outer.top, -kBorder, outer.Height(), highlight);
	}
	if ((m_dwStyle & CBRS_BORDER_BOTTOM) != 0)
	{
		pDC->FillSolidRect(0, rect.bottom, rect.right, -kBorder, highlight);
	}
}

void CControlBar::DrawGripper(CDC* pDC, const CRect& rect)
{
	if ((m_dwStyle & (CBRS_GRIPPER | CBRS_FLOATING)) != CBRS_GRIPPER)
	{
		return;
	}
	const COLORREF highlight = ::GetSysColor(COLOR_BTNHIGHLIGHT);
	const COLORREF shadow = ::GetSysColor(COLOR_BTNSHADOW);
	if ((m_dwStyle & CBRS_ORIENT_HORZ) != 0)
	{
		pDC->Draw3dRect(rect.left + kGripperBorder, rect.top + m_cyTopBorder, kGripper, rect.Height() - m_cyTopBorder - m_cyBottomBorder,
			highlight, shadow);
		return;
	}
	pDC->Draw3dRect(rect.left + m_cyTopBorder, rect.top + kGripperBorder, rect.Width() - m_cyTopBorder - m_cyBottomBorder, kGripper,
		highlight, shadow);
}

void CControlBar::EraseNonClient()
{
	CWindowDC dc(this);
	CRect windowRect;
	GetWindowRect(&windowRect);
	windowRect.OffsetRect(-windowRect.left, -windowRect.top);
	DrawBorders(&dc, windowRect);
	dc.IntersectClipRect(&windowRect);
	SendMessage(WM_ERASEBKGND, reinterpret_cast<WPARAM>(dc.m_hDC));
	DrawGripper(&dc, windowRect);
}

void CControlBar::DoPaint(CDC* pDC)
{
	CRect rect;
	GetClientRect(&rect);
	DrawBorders(pDC, rect);
	DrawGripper(pDC, rect);
}

void CControlBar::OnPaint()
{
	CPaintDC dc(this);
	if (IsVisible())
	{
		DoPaint(&dc);
	}
}

int CControlBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}
	CWnd* parent = GetParent();
	if (parent != nullptr && parent->IsFrameWnd())
	{
		m_pDockSite = static_cast<CFrameWnd*>(parent);
		m_pDockSite->AddControlBar(this);
	}
	return 0;
}

void CControlBar::OnDestroy()
{
	if (AfxGetThreadState().pLastStatus == this)
	{
		SetStatusText(-1);
	}
	if (m_pDockSite != nullptr)
	{
		m_pDockSite->RemoveControlBar(this);
		m_pDockSite = nullptr;
	}
	CWnd::OnDestroy();
}

void CControlBar::PostNcDestroy()
{
	if (m_bAutoDelete)
	{
		delete this;
	}
}

LRESULT CControlBar::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	const bool forwardsToOwner = message == WM_NOTIFY || message == WM_COMMAND || message == WM_DRAWITEM || message == WM_MEASUREITEM
		|| message == WM_DELETEITEM || message == WM_COMPAREITEM || message == WM_VKEYTOITEM || message == WM_CHARTOITEM;
	if (!forwardsToOwner)
	{
		return CWnd::WindowProc(message, wParam, lParam);
	}
	LRESULT result = 0;
	if (OnWndMsg(message, wParam, lParam, &result))
	{
		return result;
	}
	result = GetOwner()->SendMessage(message, wParam, lParam);
	if (message != WM_NOTIFY || reinterpret_cast<NMHDR*>(lParam)->code != TTN_NEEDTEXTA)
	{
		return result;
	}
	const TOOLTIPTEXTA* toolTip = reinterpret_cast<TOOLTIPTEXTA*>(lParam);
	const bool isUnanswered = toolTip->hinst == nullptr && (toolTip->lpszText == nullptr || *toolTip->lpszText == '\0');
	return isUnanswered ? CWnd::WindowProc(message, wParam, lParam) : result;
}

LRESULT CControlBar::OnSizeParent(WPARAM, LPARAM lParam)
{
	AFX_SIZEPARENTPARAMS* layout = reinterpret_cast<AFX_SIZEPARENTPARAMS*>(lParam);
	const DWORD style = RecalcDelayShow(layout);
	if ((style & WS_VISIBLE) == 0 || (style & CBRS_ALIGN_ANY) == 0)
	{
		return 0;
	}

	CRect rect = layout->rect;
	DWORD mode = layout->bStretch ? LM_STRETCH : 0;
	mode |= (style & CBRS_ORIENT_HORZ) != 0 ? LM_HORZ | LM_HORZDOCK : LM_VERTDOCK;
	CSize size = CalcDynamicLayout(-1, mode);
	size.cx = std::min<LONG>(size.cx, rect.Width());
	size.cy = std::min<LONG>(size.cy, rect.Height());

	if ((style & CBRS_ORIENT_HORZ) != 0)
	{
		layout->sizeTotal.cy += size.cy;
		layout->sizeTotal.cx = std::max<LONG>(layout->sizeTotal.cx, size.cx);
		if ((style & CBRS_ALIGN_TOP) != 0)
		{
			layout->rect.top += size.cy;
		}
		else if ((style & CBRS_ALIGN_BOTTOM) != 0)
		{
			rect.top = rect.bottom - size.cy;
			layout->rect.bottom -= size.cy;
		}
	}
	else
	{
		layout->sizeTotal.cx += size.cx;
		layout->sizeTotal.cy = std::max<LONG>(layout->sizeTotal.cy, size.cy);
		if ((style & CBRS_ALIGN_LEFT) != 0)
		{
			layout->rect.left += size.cx;
		}
		else if ((style & CBRS_ALIGN_RIGHT) != 0)
		{
			rect.left = rect.right - size.cx;
			layout->rect.right -= size.cx;
		}
	}
	rect.right = rect.left + size.cx;
	rect.bottom = rect.top + size.cy;
	if (layout->hDWP != nullptr)
	{
		AfxRepositionWindow(layout, m_hWnd, &rect);
	}
	return 0;
}

DWORD CControlBar::RecalcDelayShow(AFX_SIZEPARENTPARAMS* lpLayout)
{
	DWORD style = (m_dwStyle & (CBRS_ALIGN_ANY | CBRS_BORDER_ANY)) | (GetStyle() & WS_VISIBLE);
	if ((m_nStateFlags & (delayHide | delayShow)) == 0)
	{
		return style;
	}
	UINT flags = 0;
	if ((m_nStateFlags & delayHide) != 0 && (style & WS_VISIBLE) != 0)
	{
		flags = SWP_HIDEWINDOW;
	}
	else if ((m_nStateFlags & delayShow) != 0 && (style & WS_VISIBLE) == 0)
	{
		flags = SWP_SHOWWINDOW;
	}
	if (flags == 0)
	{
		m_nStateFlags &= ~(delayShow | delayHide);
		return style;
	}
	style ^= WS_VISIBLE;
	if (lpLayout->hDWP != nullptr)
	{
		m_nStateFlags &= ~(delayShow | delayHide);
		lpLayout->hDWP = ::DeferWindowPos(lpLayout->hDWP, m_hWnd, nullptr, 0, 0, 0, 0,
			flags | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
	return style;
}

LRESULT CControlBar::OnIdleUpdateCmdUI(WPARAM wParam, LPARAM)
{
	const bool isVisible = (GetStyle() & WS_VISIBLE) != 0;
	UINT flags = 0;
	if ((m_nStateFlags & delayHide) != 0 && isVisible)
	{
		flags = SWP_HIDEWINDOW;
	}
	else if ((m_nStateFlags & delayShow) != 0 && !isVisible)
	{
		flags = SWP_SHOWWINDOW;
	}
	m_nStateFlags &= ~(delayShow | delayHide);
	if (flags != 0)
	{
		SetWindowPos(nullptr, 0, 0, 0, 0, flags | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
	if ((GetStyle() & WS_VISIBLE) == 0)
	{
		return 0;
	}
	CWnd* owner = GetOwner();
	CFrameWnd* target = owner != nullptr && owner->IsFrameWnd() ? static_cast<CFrameWnd*>(owner) : GetParentFrame();
	if (target != nullptr)
	{
		OnUpdateCmdUI(target, static_cast<BOOL>(wParam));
	}
	return 0;
}

int CControlBar::OnToolHitTest(CPoint point) const
{
	const HWND child = ::ChildWindowFromPoint(m_hWnd, point);
	if (child == nullptr || child == m_hWnd)
	{
		return -1;
	}
	return ::GetDlgCtrlID(child);
}

BOOL CControlBar::SetStatusText(int nHit)
{
	CWnd* owner = GetOwner();
	AfxThreadState& state = AfxGetThreadState();
	if (nHit == -1)
	{
		state.pLastStatus = nullptr;
		if ((m_nStateFlags & statusSet) != 0)
		{
			owner->SendMessage(WM_POPMESSAGESTRING, AFX_IDS_IDLEMESSAGE);
			m_nStateFlags &= ~statusSet;
			return TRUE;
		}
		KillTimer(kWaitTimer);
		return FALSE;
	}
	if ((m_nStateFlags & statusSet) != 0 && state.nLastStatus == nHit)
	{
		return FALSE;
	}
	state.pLastStatus = this;
	owner->SendMessage(WM_SETMESSAGESTRING, static_cast<WPARAM>(nHit));
	m_nStateFlags |= statusSet;
	ResetTimer(kCheckTimer, 200);
	return TRUE;
}

void CControlBar::ResetTimer(UINT nEvent, UINT nTime)
{
	KillTimer(kWaitTimer);
	KillTimer(kCheckTimer);
	SetTimer(nEvent, nTime, nullptr);
}

int CControlBar::statusHitAtCursor() const
{
	CPoint point;
	::GetCursorPos(&point);
	ScreenToClient(&point);
	int hit = OnToolHitTest(point);
	if (hit < 0)
	{
		return hit;
	}
	CWnd* topLevel = GetTopLevelParent();
	CWnd* capture = CWnd::GetCapture();
	const bool isBlocked = topLevel == nullptr || !topLevel->IsWindowEnabled() || CWnd::GetForegroundWindow() != topLevel->GetLastActivePopup();
	if (isBlocked || (capture != nullptr && capture != this && capture->GetTopLevelParent() == topLevel))
	{
		return -1;
	}
	ClientToScreen(&point);
	const HWND under = ::WindowFromPoint(point);
	return under == m_hWnd || ::IsChild(m_hWnd, under) ? hit : -1;
}

void CControlBar::OnTimer(UINT)
{
	if (::GetKeyState(VK_LBUTTON) < 0)
	{
		return;
	}
	const int hit = statusHitAtCursor();
	if (hit >= 0)
	{
		SetStatusText(hit);
		return;
	}
	AfxGetThreadState().nLastStatus = -1;
	KillTimer(kCheckTimer);
	SetStatusText(-1);
}

void CControlBar::updateFlyByStatus(const MSG* pMsg)
{
	AfxThreadState& state = AfxGetThreadState();
	CPoint point = pMsg->pt;
	ScreenToClient(&point);
	int hit = OnToolHitTest(point);
	const bool isButtonDown = ::GetKeyState(VK_LBUTTON) < 0;
	if (pMsg->message != WM_LBUTTONDOWN && isButtonDown)
	{
		hit = state.nLastStatus;
	}

	if (hit < 0)
	{
		if (!isButtonDown)
		{
			SetStatusText(-1);
			KillTimer(kCheckTimer);
		}
	}
	else if (pMsg->message == WM_LBUTTONUP)
	{
		SetStatusText(-1);
		ResetTimer(kCheckTimer, 200);
	}
	else if ((m_nStateFlags & statusSet) != 0 || isButtonDown)
	{
		SetStatusText(hit);
	}
	else if (hit != state.nLastStatus)
	{
		ResetTimer(kWaitTimer, 300);
	}
	state.nLastStatus = hit;
}

BOOL CControlBar::PreTranslateMessage(MSG* pMsg)
{
	if (CWnd::PreTranslateMessage(pMsg))
	{
		return TRUE;
	}
	const UINT message = pMsg->message;
	const bool tracksStatus = (m_dwStyle & CBRS_FLYBY) != 0 || message == WM_LBUTTONDOWN || message == WM_LBUTTONUP;
	if (tracksStatus && isMouseMessage(message))
	{
		updateFlyByStatus(pMsg);
	}
	for (CWnd* owner = GetOwner(); owner != nullptr; owner = owner->GetParentFrame())
	{
		if (owner->PreTranslateMessage(pMsg))
		{
			return TRUE;
		}
	}
	return PreTranslateInput(pMsg);
}
