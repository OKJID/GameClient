#include "AfxInternal.h"

IMPLEMENT_DYNAMIC(CView, CWnd)

BEGIN_MESSAGE_MAP(CView, CWnd)
	ON_WM_PAINT()
	ON_WM_MOUSEACTIVATE()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_INITIALUPDATE, OnInitialUpdateMessage)
END_MESSAGE_MAP()

CView::CView() :
	m_pDocument(nullptr)
{
}

CView::~CView()
{
	if (m_pDocument != nullptr)
	{
		m_pDocument->RemoveView(this);
	}
}

BOOL CView::PreCreateWindow(CREATESTRUCT& cs)
{
	if (cs.lpszClass == nullptr)
	{
		AfxDeferRegisterClass(AFX_WNDFRAMEORVIEW_REG);
		cs.lpszClass = kAfxFrameOrViewClass;
	}
	if ((cs.style & WS_BORDER) != 0)
	{
		cs.dwExStyle |= WS_EX_CLIENTEDGE;
		cs.style &= ~WS_BORDER;
	}
	return TRUE;
}

int CView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}
	CCreateContext* context = static_cast<CCreateContext*>(lpCreateStruct->lpCreateParams);
	if (context != nullptr && context->m_pCurrentDoc != nullptr)
	{
		context->m_pCurrentDoc->AddView(this);
	}
	return 0;
}

void CView::OnDestroy()
{
	CFrameWnd* frame = GetParentFrame();
	if (frame != nullptr && frame->GetActiveView() == this)
	{
		frame->SetActiveView(nullptr);
	}
	CWnd::OnDestroy();
}

void CView::PostNcDestroy()
{
	delete this;
}

BOOL CView::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	if (CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
	{
		return TRUE;
	}
	return m_pDocument != nullptr ? m_pDocument->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) : FALSE;
}

void CView::OnPaint()
{
	CPaintDC dc(this);
	OnPrepareDC(&dc);
	OnDraw(&dc);
}

int CView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	const int result = CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
	if (result == MA_NOACTIVATE || result == MA_NOACTIVATEANDEAT)
	{
		return result;
	}
	CFrameWnd* frame = GetParentFrame();
	if (frame == nullptr)
	{
		return result;
	}
	const HWND focus = ::GetFocus();
	if (frame->GetActiveView() == this && m_hWnd != focus && !::IsChild(m_hWnd, focus))
	{
		OnActivateView(TRUE, this, this);
	}
	else
	{
		frame->SetActiveView(this);
	}
	return result;
}

LRESULT CView::OnInitialUpdateMessage(WPARAM, LPARAM)
{
	OnInitialUpdate();
	return 0;
}

BOOL CView::IsSelected(const CObject*) const
{
	return FALSE;
}

void CView::OnInitialUpdate()
{
	OnUpdate(nullptr, 0, nullptr);
}

void CView::OnUpdate(CView*, LPARAM, CObject*)
{
	Invalidate(TRUE);
}

void CView::OnActivateView(BOOL bActivate, CView*, CView*)
{
	if (bActivate && IsTopParentActive())
	{
		SetFocus();
	}
}

void CView::OnActivateFrame(UINT, CFrameWnd*)
{
}

BOOL CView::IsTopParentActive() const
{
	CWnd* topLevel = GetTopLevelParent();
	return topLevel != nullptr && CWnd::GetForegroundWindow() == topLevel->GetLastActivePopup() ? TRUE : FALSE;
}

void CView::OnPrepareDC(CDC*, CPrintInfo* pInfo)
{
	if (pInfo != nullptr)
	{
		pInfo->m_bContinuePrinting = pInfo->GetMaxPage() != 0xFFFF || pInfo->m_nCurPage == 1;
	}
}

BOOL CView::OnPreparePrinting(CPrintInfo*)
{
	return TRUE;
}

void CView::OnBeginPrinting(CDC*, CPrintInfo*)
{
}

void CView::OnPrint(CDC* pDC, CPrintInfo*)
{
	OnDraw(pDC);
}

void CView::OnEndPrinting(CDC*, CPrintInfo*)
{
}
