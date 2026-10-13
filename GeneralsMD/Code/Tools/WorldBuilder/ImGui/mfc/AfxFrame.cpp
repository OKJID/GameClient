#include "AfxInternal.h"

#include "Win32Window.h"

#include <algorithm>
#include <cstring>
#include <strings.h>

IMPLEMENT_DYNCREATE(CFrameWnd, CWnd)

const CRect CFrameWnd::rectDefault(CW_USEDEFAULT, CW_USEDEFAULT, 0, 0);

BEGIN_MESSAGE_MAP(CFrameWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_SETFOCUS()
	ON_WM_ACTIVATE()
	ON_WM_ENABLE()
	ON_WM_INITMENUPOPUP()
	ON_WM_MENUSELECT()
	ON_WM_ENTERIDLE()
	ON_MESSAGE(WM_SETMESSAGESTRING, OnSetMessageString)
	ON_MESSAGE(WM_POPMESSAGESTRING, OnPopMessageString)
	ON_MESSAGE(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
	ON_MESSAGE(WM_ACTIVATETOPLEVEL, OnActivateTopLevel)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STATUS_BAR, OnUpdateControlBarMenu)
	ON_COMMAND_EX(ID_VIEW_STATUS_BAR, OnBarCheck)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TOOLBAR, OnUpdateControlBarMenu)
	ON_COMMAND_EX(ID_VIEW_TOOLBAR, OnBarCheck)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_CAPS, OnUpdateKeyIndicator)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_NUM, OnUpdateKeyIndicator)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SCRL, OnUpdateKeyIndicator)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
END_MESSAGE_MAP()

namespace
{

int defaultableExtent(LONG from, LONG to)
{
	return from == CW_USEDEFAULT ? CW_USEDEFAULT : to - from;
}

void setWindowTextIfChanged(HWND hWnd, LPCSTR lpszNew)
{
	char current[256];
	const int newLength = static_cast<int>(strlen(lpszNew));
	const bool fits = newLength < static_cast<int>(sizeof(current));
	if (fits && ::GetWindowTextA(hWnd, current, sizeof(current)) == newLength && strcmp(current, lpszNew) == 0)
	{
		return;
	}
	::SetWindowTextA(hWnd, lpszNew);
}

BOOL isComboBoxWithStyle(HWND hWnd, UINT nStyle)
{
	if (hWnd == nullptr || static_cast<UINT>(::GetWindowLongA(hWnd, GWL_STYLE) & 0x0F) != nStyle)
	{
		return FALSE;
	}
	char className[16] = {};
	::GetClassNameA(hWnd, className, sizeof(className));
	return strcasecmp(className, "combobox") == 0 ? TRUE : FALSE;
}

}

void AFXAPI AfxCancelModes(HWND hWndRcvr)
{
	HWND hWndCancel = ::GetFocus();
	if (hWndCancel == nullptr || hWndCancel == hWndRcvr)
	{
		return;
	}
	if (!isComboBoxWithStyle(hWndCancel, CBS_DROPDOWNLIST))
	{
		hWndCancel = ::GetParent(hWndCancel);
		if (hWndCancel == hWndRcvr || !isComboBoxWithStyle(hWndCancel, CBS_DROPDOWN))
		{
			return;
		}
	}
	::SendMessageA(hWndCancel, CB_SHOWDROPDOWN, FALSE, 0);
}

CFrameWnd::CFrameWnd() :
	m_bAutoMenuEnable(TRUE),
	m_nIDHelp(0),
	m_hAccelTable(nullptr),
	m_hMenuDefault(nullptr),
	m_nWindow(-1),
	m_nIdleFlags(0),
	m_nShowDelay(-1),
	m_rectBorder(0, 0, 0, 0),
	m_pFloatingFrameClass(nullptr),
	m_pViewActive(nullptr),
	m_nIDTracking(0),
	m_nIDLastMessage(0),
	m_cModalStack(0),
	m_dwDockStyle(0),
	m_bInRecalcLayout(FALSE)
{
	AfxGetThreadState().frames.push_back(this);
}

CFrameWnd::~CFrameWnd()
{
	std::vector<CFrameWnd*>& frames = AfxGetThreadState().frames;
	frames.erase(std::remove(frames.begin(), frames.end(), this), frames.end());
}

BOOL CFrameWnd::Create(LPCSTR lpszClassName, LPCSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, LPCSTR lpszMenuName,
	DWORD dwExStyle, CCreateContext* pContext)
{
	HMENU menu = nullptr;
	if (lpszMenuName != nullptr)
	{
		menu = ::LoadMenuA(AfxGetResourceHandle(), lpszMenuName);
		if (menu == nullptr)
		{
			PostNcDestroy();
			return FALSE;
		}
	}

	m_strTitle = lpszWindowName;
	if (CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle, rect.left, rect.top, defaultableExtent(rect.left, rect.right),
			defaultableExtent(rect.top, rect.bottom), pParentWnd->GetSafeHwnd(), menu, pContext))
	{
		return TRUE;
	}
	if (menu != nullptr)
	{
		::DestroyMenu(menu);
	}
	return FALSE;
}

BOOL CFrameWnd::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext)
{
	m_nIDHelp = nIDResource;
	CString fullString;
	if (fullString.LoadString(nIDResource))
	{
		AfxExtractSubString(m_strTitle, fullString, 0);
	}

	AfxDeferRegisterClass(AFX_WNDFRAMEORVIEW_REG);
	const LPCSTR className = GetIconWndClass(dwDefaultStyle, nIDResource);
	const CString title = m_strTitle;
	if (!Create(className, title, dwDefaultStyle, rectDefault, pParentWnd, MAKEINTRESOURCE(nIDResource), 0, pContext))
	{
		return FALSE;
	}

	m_hMenuDefault = ::GetMenu(m_hWnd);
	m_hAccelTable = ::LoadAcceleratorsA(AfxGetResourceHandle(), MAKEINTRESOURCE(nIDResource));
	if (pContext == nullptr)
	{
		SendMessageToDescendants(WM_INITIALUPDATE, 0, 0, TRUE, TRUE);
	}
	return TRUE;
}

LPCSTR CFrameWnd::GetIconWndClass(DWORD dwDefaultStyle, UINT nIDResource)
{
	const HICON icon = ::LoadIconA(AfxGetResourceHandle(), MAKEINTRESOURCE(nIDResource));
	if (icon == nullptr)
	{
		return nullptr;
	}
	CREATESTRUCT cs = {};
	cs.style = static_cast<LONG>(dwDefaultStyle);
	PreCreateWindow(cs);

	WNDCLASSA windowClass = {};
	if (cs.lpszClass == nullptr || !::GetClassInfoA(AfxGetInstanceHandle(), cs.lpszClass, &windowClass) || windowClass.hIcon == icon)
	{
		return nullptr;
	}
	return AfxRegisterWndClass(windowClass.style, windowClass.hCursor, windowClass.hbrBackground, icon);
}

BOOL CFrameWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	if (cs.lpszClass == nullptr)
	{
		AfxDeferRegisterClass(AFX_WNDFRAMEORVIEW_REG);
		cs.lpszClass = kAfxFrameOrViewClass;
	}
	if ((cs.style & FWS_ADDTOTITLE) != 0)
	{
		cs.style |= FWS_PREFIXTITLE;
	}
	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	return TRUE;
}

int CFrameWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	return OnCreateHelper(lpCreateStruct, static_cast<CCreateContext*>(lpCreateStruct->lpCreateParams));
}

int CFrameWnd::OnCreateHelper(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	if (CWnd::OnCreate(lpcs) == -1 || !OnCreateClient(lpcs, pContext))
	{
		return -1;
	}
	PostMessage(WM_SETMESSAGESTRING, AFX_IDS_IDLEMESSAGE);
	RecalcLayout();
	return 0;
}

BOOL CFrameWnd::OnCreateClient(LPCREATESTRUCT, CCreateContext* pContext)
{
	if (pContext == nullptr || pContext->m_pNewViewClass == nullptr)
	{
		return TRUE;
	}
	return CreateView(pContext, AFX_IDW_PANE_FIRST) != nullptr ? TRUE : FALSE;
}

CWnd* CFrameWnd::CreateView(CCreateContext* pContext, UINT nID)
{
	CWnd* view = static_cast<CWnd*>(pContext->m_pNewViewClass->CreateObject());
	if (view == nullptr)
	{
		return nullptr;
	}
	if (!view->Create(nullptr, nullptr, AFX_WS_DEFAULT_VIEW, CRect(0, 0, 0, 0), this, nID, pContext))
	{
		return nullptr;
	}
	if ((view->GetExStyle() & WS_EX_CLIENTEDGE) != 0)
	{
		ModifyStyleEx(WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED);
	}
	return view;
}

BOOL CFrameWnd::IsFrameWnd() const
{
	return TRUE;
}

void CFrameWnd::PostNcDestroy()
{
	delete this;
}

void CFrameWnd::SetActiveView(CView* pViewNew, BOOL bNotify)
{
	CView* viewOld = m_pViewActive;
	if (pViewNew == viewOld)
	{
		return;
	}
	m_pViewActive = nullptr;
	if (viewOld != nullptr)
	{
		viewOld->OnActivateView(FALSE, pViewNew, viewOld);
	}
	if (m_pViewActive != nullptr)
	{
		return;
	}
	m_pViewActive = pViewNew;
	if (pViewNew != nullptr && bNotify)
	{
		pViewNew->OnActivateView(TRUE, pViewNew, viewOld);
	}
}

CDocument* CFrameWnd::GetActiveDocument()
{
	CView* view = GetActiveView();
	return view != nullptr ? view->GetDocument() : nullptr;
}

CFrameWnd* CFrameWnd::GetActiveFrame()
{
	return this;
}

void CFrameWnd::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	if ((GetStyle() & FWS_ADDTOTITLE) == 0)
	{
		return;
	}
	CDocument* document = GetActiveDocument();
	UpdateFrameTitleForDocument(bAddToTitle && document != nullptr ? static_cast<LPCSTR>(document->GetTitle()) : nullptr);
}

void CFrameWnd::UpdateFrameTitleForDocument(LPCSTR lpszDocName)
{
	CString windowNumber;
	if (m_nWindow > 0)
	{
		windowNumber.Format(":%d", m_nWindow);
	}
	CString windowText;
	if ((GetStyle() & FWS_PREFIXTITLE) != 0)
	{
		if (lpszDocName != nullptr)
		{
			windowText += lpszDocName;
			windowText += windowNumber;
			windowText += " - ";
		}
		windowText += m_strTitle;
	}
	else
	{
		windowText += m_strTitle;
		if (lpszDocName != nullptr)
		{
			windowText += " - ";
			windowText += lpszDocName;
			windowText += windowNumber;
		}
	}
	setWindowTextIfChanged(m_hWnd, windowText);
}

void CFrameWnd::OnDestroy()
{
	if (m_hMenuDefault != nullptr && ::GetMenu(m_hWnd) != m_hMenuDefault)
	{
		::SetMenu(m_hWnd, m_hMenuDefault);
	}
	CWnd::OnDestroy();
}

void CFrameWnd::OnClose()
{
	CDocument* document = GetActiveDocument();
	if (document != nullptr && !document->CanCloseFrame(this))
	{
		return;
	}
	CWinApp* app = AfxGetApp();
	if (app != nullptr && app->m_pMainWnd == this && !closeApplicationDocuments(document))
	{
		return;
	}
	if (document != nullptr && document->m_bAutoDelete && closeLastDocumentFrame(document))
	{
		return;
	}
	DestroyWindow();
}

BOOL CFrameWnd::closeApplicationDocuments(CDocument* pDocument)
{
	CWinApp* app = AfxGetApp();
	if (pDocument == nullptr && !app->SaveAllModified())
	{
		return FALSE;
	}
	app->HideApplication();
	app->CloseAllDocuments(FALSE);
	if (app->m_pMainWnd == nullptr)
	{
		AfxPostQuitMessage(0);
		return FALSE;
	}
	return TRUE;
}

BOOL CFrameWnd::closeLastDocumentFrame(CDocument* pDocument)
{
	POSITION position = pDocument->GetFirstViewPosition();
	while (position != nullptr)
	{
		if (pDocument->GetNextView(position)->GetParentFrame() != this)
		{
			pDocument->PreCloseFrame(this);
			return FALSE;
		}
	}
	pDocument->OnCloseDocument();
	return TRUE;
}

void CFrameWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	if (nType != SIZE_MINIMIZED)
	{
		RecalcLayout();
	}
}

BOOL CFrameWnd::OnEraseBkgnd(CDC* pDC)
{
	if (m_pViewActive != nullptr)
	{
		return TRUE;
	}
	return CWnd::OnEraseBkgnd(pDC);
}

void CFrameWnd::OnSetFocus(CWnd* pOldWnd)
{
	if (m_pViewActive != nullptr)
	{
		m_pViewActive->SetFocus();
		return;
	}
	CWnd::OnSetFocus(pOldWnd);
}

void CFrameWnd::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CWnd::OnActivate(nState, pWndOther, bMinimized);

	CFrameWnd* topLevel = (GetStyle() & WS_CHILD) != 0 ? this : GetTopLevelFrame();
	CWnd* active = nState == WA_INACTIVE ? pWndOther : this;
	const bool staysActive = active != nullptr
		&& (topLevel == active || (topLevel == active->GetTopLevelFrame() && active->SendMessage(WM_FLOATSTATUS, FS_SYNCACTIVE) != 0));
	topLevel->NotifyFloatingWindows(staysActive ? FS_ACTIVATE : FS_DEACTIVATE);
	NotifyFloatingWindows(staysActive ? FS_ACTIVATE : FS_DEACTIVATE);

	CView* activeView = GetActiveView();
	if (activeView == nullptr)
	{
		activeView = GetActiveFrame()->GetActiveView();
	}
	if (activeView == nullptr)
	{
		return;
	}
	if (nState != WA_INACTIVE && !bMinimized)
	{
		activeView->OnActivateView(TRUE, activeView, activeView);
	}
	activeView->OnActivateFrame(nState, this);
}

void CFrameWnd::OnEnable(BOOL bEnable)
{
	if (bEnable && (m_nFlags & WF_STAYDISABLED) != 0)
	{
		EnableWindow(FALSE);
		::SetFocus(nullptr);
		return;
	}
	if (GetParent() != nullptr)
	{
		return;
	}
	if (!bEnable && !InModalState())
	{
		m_nFlags |= WF_MODALDISABLE;
		BeginModalState();
	}
	else if (bEnable && (m_nFlags & WF_MODALDISABLE) != 0)
	{
		m_nFlags &= ~WF_MODALDISABLE;
		EndModalState();
		if (::GetActiveWindow() == m_hWnd)
		{
			SendMessage(WM_ACTIVATE, WA_ACTIVE);
		}
	}
	NotifyFloatingWindows(bEnable ? FS_ENABLE : FS_DISABLE);
}

void CFrameWnd::NotifyFloatingWindows(DWORD dwFlags)
{
	CFrameWnd* parent = (GetStyle() & WS_CHILD) != 0 ? this : GetTopLevelFrame();
	for (HWND hWnd = ::GetWindow(::GetDesktopWindow(), GW_CHILD); hWnd != nullptr; hWnd = ::GetWindow(hWnd, GW_HWNDNEXT))
	{
		if (AfxIsDescendant(parent->m_hWnd, hWnd))
		{
			::SendMessageA(hWnd, WM_FLOATSTATUS, dwFlags, 0);
		}
	}
}

void CFrameWnd::BeginModalState()
{
	if (++m_cModalStack > 1)
	{
		return;
	}
	CWnd* topLevel = GetTopLevelParent();
	disableModelessWindows(topLevel != nullptr ? topLevel->m_hWnd : m_hWnd);
}

void CFrameWnd::disableModelessWindows(HWND hWndTopLevel)
{
	std::vector<HWND> windows;
	for (HWND hWnd = ::GetWindow(::GetDesktopWindow(), GW_CHILD); hWnd != nullptr; hWnd = ::GetWindow(hWnd, GW_HWNDNEXT))
	{
		if (::IsWindowEnabled(hWnd) && CWnd::FromHandlePermanent(hWnd) != nullptr && AfxIsDescendant(hWndTopLevel, hWnd)
			&& ::SendMessageA(hWnd, WM_DISABLEMODAL, 0, 0) == 0)
		{
			windows.push_back(hWnd);
		}
	}
	for (HWND hWnd : windows)
	{
		::EnableWindow(hWnd, FALSE);
		m_disabledWindows.push_back(hWnd);
	}
}

void CFrameWnd::EndModalState()
{
	if (m_cModalStack == 0 || --m_cModalStack > 0)
	{
		return;
	}
	std::vector<HWND> windows;
	windows.swap(m_disabledWindows);
	for (HWND hWnd : windows)
	{
		if (::IsWindow(hWnd))
		{
			::EnableWindow(hWnd, TRUE);
		}
	}
}

void CFrameWnd::InitialUpdateFrame(CDocument* pDoc, BOOL bMakeVisible)
{
	CView* view = nullptr;
	if (GetActiveView() == nullptr)
	{
		CWnd* pane = GetDescendantWindow(AFX_IDW_PANE_FIRST, TRUE);
		if (pane != nullptr && pane->IsKindOf(RUNTIME_CLASS(CView)))
		{
			view = static_cast<CView*>(pane);
			SetActiveView(view, FALSE);
		}
	}

	if (bMakeVisible)
	{
		SendMessageToDescendants(WM_INITIALUPDATE, 0, 0, TRUE, TRUE);
		if (view != nullptr)
		{
			view->OnActivateFrame(WA_INACTIVE, this);
		}
		int nCmdShow = -1;
		CWinApp* app = AfxGetApp();
		if (app != nullptr && app->m_pMainWnd == nullptr)
		{
			nCmdShow = app->m_nCmdShow;
			app->m_nCmdShow = -1;
		}
		ActivateFrame(nCmdShow);
		if (view != nullptr)
		{
			view->OnActivateView(TRUE, view, view);
		}
	}

	if (pDoc != nullptr)
	{
		pDoc->UpdateFrameCounts();
	}
	OnUpdateFrameTitle(TRUE);
}

void CFrameWnd::ActivateFrame(int nCmdShow)
{
	if (nCmdShow == -1)
	{
		if (!IsWindowVisible())
		{
			nCmdShow = SW_SHOWNORMAL;
		}
		else if (IsIconic())
		{
			nCmdShow = SW_RESTORE;
		}
	}
	BringToTop(nCmdShow);
	if (nCmdShow == -1)
	{
		return;
	}
	ShowWindow(nCmdShow);
	BringToTop(nCmdShow);
}

void CFrameWnd::BringToTop(int nCmdShow)
{
	const bool keepsPlace = nCmdShow == SW_HIDE || nCmdShow == SW_MINIMIZE || nCmdShow == SW_SHOWMINNOACTIVE || nCmdShow == SW_SHOWNA
		|| nCmdShow == SW_SHOWNOACTIVATE;
	if (keepsPlace)
	{
		return;
	}
	::BringWindowToTop(::GetLastActivePopup(m_hWnd));
}

void CFrameWnd::RecalcLayout(BOOL)
{
	if (m_bInRecalcLayout)
	{
		return;
	}
	m_bInRecalcLayout = TRUE;
	m_nIdleFlags &= ~(idleLayout | idleNotify);

	if (Win32IsHostWindow(m_hWnd))
	{
		recalcHostLayout();
	}
	else if ((GetStyle() & FWS_SNAPTOBARS) != 0)
	{
		CRect rect(0, 0, 32767, 32767);
		RepositionBars(0, 0xFFFF, AFX_IDW_PANE_FIRST, reposQuery, &rect, &rect, FALSE);
		RepositionBars(0, 0xFFFF, AFX_IDW_PANE_FIRST, reposExtra, &m_rectBorder, &rect, TRUE);
		CalcWindowRect(&rect);
		SetWindowPos(nullptr, 0, 0, rect.Width(), rect.Height(), SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
	}
	else
	{
		RepositionBars(0, 0xFFFF, AFX_IDW_PANE_FIRST, reposExtra, &m_rectBorder);
	}
	m_bInRecalcLayout = FALSE;
}

void CFrameWnd::recalcHostLayout()
{
	CRect client;
	GetClientRect(&client);
	CRect barArea = client;
	if (::GetMenu(m_hWnd) != nullptr)
	{
		barArea.top += ::GetSystemMetrics(SM_CYMENU);
	}
	RepositionBars(0, 0xFFFF, AFX_IDW_PANE_FIRST, reposNoPosLeftOver, nullptr, &barArea);

	CWnd* pane = GetDlgItem(AFX_IDW_PANE_FIRST);
	if (pane == nullptr)
	{
		return;
	}
	pane->CalcWindowRect(&client);
	AfxRepositionWindow(nullptr, pane->m_hWnd, &client);
	::SetWindowPos(pane->m_hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

BOOL CFrameWnd::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_NCLBUTTONDOWN)
	{
		AfxCancelModes(pMsg->hwnd);
	}
	if (CWnd::PreTranslateMessage(pMsg))
	{
		return TRUE;
	}
	if (pMsg->message < WM_KEYFIRST || pMsg->message > WM_KEYLAST)
	{
		return FALSE;
	}
	const HACCEL accelerator = GetDefaultAccelerator();
	return accelerator != nullptr && ::TranslateAcceleratorA(m_hWnd, accelerator, pMsg) ? TRUE : FALSE;
}

HACCEL CFrameWnd::GetDefaultAccelerator()
{
	CDocument* document = GetActiveDocument();
	const HACCEL documentAccelerator = document != nullptr ? document->GetDefaultAccelerator() : nullptr;
	return documentAccelerator != nullptr ? documentAccelerator : m_hAccelTable;
}

BOOL CFrameWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	CView* view = GetActiveView();
	if (view != nullptr && view->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
	{
		return TRUE;
	}
	if (CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
	{
		return TRUE;
	}
	CWinApp* app = AfxGetApp();
	return app != nullptr && app->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) ? TRUE : FALSE;
}
