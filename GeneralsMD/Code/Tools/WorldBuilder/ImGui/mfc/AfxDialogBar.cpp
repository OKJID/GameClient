#include "AfxInternal.h"

#include "afxext.h"

IMPLEMENT_DYNAMIC(CDialogBar, CControlBar)
IMPLEMENT_DYNCREATE(CMiniDockFrameWnd, CFrameWnd)

BEGIN_MESSAGE_MAP(CDialogBar, CControlBar)
	ON_MESSAGE(WM_INITDIALOG, HandleInitDialog)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CMiniDockFrameWnd, CFrameWnd)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_FLOATSTATUS, OnFloatStatus)
END_MESSAGE_MAP()

CDialogBar::CDialogBar() :
	m_sizeDefault(0, 0),
	m_lpszTemplateName(nullptr)
{
}

CDialogBar::~CDialogBar()
{
	DestroyWindow();
}

BOOL CDialogBar::Create(CWnd* pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID)
{
	return Create(pParentWnd, MAKEINTRESOURCE(nIDTemplate), nStyle, nID);
}

BOOL CDialogBar::Create(CWnd* pParentWnd, LPCSTR lpszTemplateName, UINT nStyle, UINT nID)
{
	m_dwStyle = nStyle & CBRS_ALL;
	CREATESTRUCT cs = {};
	cs.lpszClass = kAfxWndClass;
	cs.style = static_cast<LONG>(nStyle | WS_CHILD);
	cs.hMenu = reinterpret_cast<HMENU>(static_cast<UINT_PTR>(nID));
	cs.hInstance = AfxGetInstanceHandle();
	cs.hwndParent = pParentWnd->GetSafeHwnd();
	if (!PreCreateWindow(cs))
	{
		return FALSE;
	}

	m_lpszTemplateName = lpszTemplateName;
	const BOOL isCreated = CreateDlg(lpszTemplateName, pParentWnd);
	m_lpszTemplateName = nullptr;
	if (!isCreated)
	{
		return FALSE;
	}

	SetDlgCtrlID(static_cast<int>(nID));
	CRect rect;
	GetWindowRect(&rect);
	m_sizeDefault = rect.Size();
	ModifyStyle(0, WS_CLIPSIBLINGS);
	if (!ExecuteDlgInit(lpszTemplateName))
	{
		return FALSE;
	}
	SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
	return TRUE;
}

CSize CDialogBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	if (!bStretch)
	{
		return m_sizeDefault;
	}
	return CSize(bHorz ? 32767 : m_sizeDefault.cx, bHorz ? m_sizeDefault.cy : 32767);
}

void CDialogBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	UpdateDialogControls(pTarget, bDisableIfNoHndler);
}

LRESULT CDialogBar::HandleInitDialog(WPARAM, LPARAM)
{
	Default();
	return TRUE;
}

CMiniDockFrameWnd::CMiniDockFrameWnd() :
	m_pBar(nullptr)
{
}

BOOL CMiniDockFrameWnd::CreateFor(CFrameWnd* pDockSite, CControlBar* pBar, CPoint point)
{
	CString title;
	pBar->GetWindowText(title);
	const DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU | FWS_SNAPTOBARS;
	if (!Create(nullptr, title, style, CRect(point, point), pDockSite, nullptr, WS_EX_TOOLWINDOW))
	{
		return FALSE;
	}
	m_pBar = pBar;
	pBar->SetParent(this);
	pBar->m_pFloatingFrame = this;
	return TRUE;
}

void CMiniDockFrameWnd::RecalcLayout(BOOL bNotify)
{
	if (m_bInRecalcLayout)
	{
		return;
	}
	CFrameWnd::RecalcLayout(bNotify);
	if (m_pBar == nullptr)
	{
		return;
	}
	CString title;
	m_pBar->GetWindowText(title);
	CString current;
	GetWindowText(current);
	if (current != title)
	{
		SetWindowText(title);
	}
}

void CMiniDockFrameWnd::OnClose()
{
	if (m_pBar != nullptr)
	{
		m_pBar->GetDockingFrame()->ShowControlBar(m_pBar, FALSE, FALSE);
	}
}

void CMiniDockFrameWnd::OnDestroy()
{
	if (m_pBar != nullptr && m_pBar->m_pFloatingFrame == this)
	{
		m_pBar->m_pFloatingFrame = nullptr;
	}
	CFrameWnd::OnDestroy();
}

LRESULT CMiniDockFrameWnd::OnFloatStatus(WPARAM wParam, LPARAM)
{
	if ((wParam & (FS_SHOW | FS_HIDE)) != 0)
	{
		const UINT visibility = (wParam & FS_SHOW) != 0 ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;
		SetWindowPos(nullptr, 0, 0, 0, 0, visibility | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
	if ((wParam & (FS_ENABLE | FS_DISABLE)) != 0)
	{
		EnableWindow((wParam & FS_ENABLE) != 0);
	}
	return (wParam & FS_SYNCACTIVE) != 0 ? 1 : 0;
}
