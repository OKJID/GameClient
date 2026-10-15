#include "AfxInternal.h"

#include "afxcmn.h"
#include "afxdlgs.h"

#include "Win32Resources.h"

IMPLEMENT_DYNAMIC(CPropertyPage, CDialog)
IMPLEMENT_DYNAMIC(CPropertySheet, CWnd)

BEGIN_MESSAGE_MAP(CPropertySheet, CWnd)
	ON_WM_CLOSE()
	ON_WM_SYSCOMMAND()
END_MESSAGE_MAP()

namespace
{

const DWORD kDefaultSheetStyle = DS_MODALFRAME | DS_3DLOOK | DS_SETFONT | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU;

UINT CALLBACK AfxPropPageCallback(HWND, UINT message, LPPROPSHEETPAGEA pPropPage)
{
	switch (message)
	{
	case PSPCB_CREATE:
		AfxHookWindowCreate(reinterpret_cast<CPropertyPage*>(pPropPage->lParam));
		return TRUE;
	case PSPCB_RELEASE:
		AfxUnhookWindowCreate();
		return 0;
	default:
		return 0;
	}
}

bool isPushButton(HWND hWndCtrl)
{
	const LRESULT dialogCode = ::SendMessageA(hWndCtrl, WM_GETDLGCODE, 0, 0);
	if ((dialogCode & (DLGC_BUTTON | DLGC_DEFPUSHBUTTON)) == 0)
	{
		return false;
	}
	const LONG buttonStyle = ::GetWindowLongA(hWndCtrl, GWL_STYLE) & 0x0F;
	return buttonStyle == BS_PUSHBUTTON || buttonStyle == BS_DEFPUSHBUTTON || buttonStyle == BS_USERBUTTON || buttonStyle == BS_OWNERDRAW;
}

}

int CALLBACK AfxPropSheetCallback(HWND, UINT message, LPARAM lParam)
{
	if (message != PSCB_PRECREATE)
	{
		return 0;
	}
	CPropertySheet* pSheet = DYNAMIC_DOWNCAST(CPropertySheet, AfxGetThreadState().pWndInit);
	Win32DialogTemplate* dialogTemplate = reinterpret_cast<Win32DialogTemplate*>(lParam);
	if (pSheet == nullptr || dialogTemplate == nullptr)
	{
		return 0;
	}
	dialogTemplate->style = pSheet->m_dwStyle;
	dialogTemplate->exStyle = pSheet->m_dwExStyle;
	return 0;
}

CPropertyPage::CPropertyPage(UINT nIDTemplate, UINT nIDCaption)
{
	CommonConstruct(MAKEINTRESOURCE(nIDTemplate), nIDCaption);
}

CPropertyPage::CPropertyPage(LPCSTR lpszTemplateName, UINT nIDCaption)
{
	CommonConstruct(lpszTemplateName, nIDCaption);
}

CPropertyPage::~CPropertyPage()
{
}

void CPropertyPage::Construct(UINT nIDTemplate, UINT nIDCaption)
{
	CommonConstruct(MAKEINTRESOURCE(nIDTemplate), nIDCaption);
}

void CPropertyPage::Construct(LPCSTR lpszTemplateName, UINT nIDCaption)
{
	CommonConstruct(lpszTemplateName, nIDCaption);
}

void CPropertyPage::CommonConstruct(LPCSTR lpszTemplateName, UINT nIDCaption)
{
	memset(&m_psp, 0, sizeof(m_psp));
	m_psp.dwSize = sizeof(m_psp);
	m_psp.dwFlags = PSP_USECALLBACK;
	m_psp.hInstance = lpszTemplateName != nullptr ? AfxGetResourceHandle() : nullptr;
	m_psp.pszTemplate = lpszTemplateName;
	m_psp.pfnDlgProc = AfxDlgProc;
	m_psp.lParam = reinterpret_cast<LPARAM>(this);
	m_psp.pfnCallback = AfxPropPageCallback;
	if (nIDCaption != 0 && m_strCaption.LoadString(nIDCaption))
	{
		m_psp.pszTitle = m_strCaption;
		m_psp.dwFlags |= PSP_USETITLE;
	}
	if (IS_INTRESOURCE(lpszTemplateName))
	{
		m_nIDHelp = LOWORD(reinterpret_cast<uintptr_t>(lpszTemplateName));
	}
	m_lpszTemplateName = m_psp.pszTemplate;
	m_bFirstSetActive = TRUE;
}

void CPropertyPage::CancelToClose()
{
	GetParent()->SendMessage(PSM_CANCELTOCLOSE);
}

void CPropertyPage::SetModified(BOOL bChanged)
{
	GetParent()->SendMessage(bChanged ? PSM_CHANGED : PSM_UNCHANGED, reinterpret_cast<WPARAM>(m_hWnd));
}

LRESULT CPropertyPage::QuerySiblings(WPARAM wParam, LPARAM lParam)
{
	return GetParent()->SendMessage(PSM_QUERYSIBLINGS, wParam, lParam);
}

BOOL CPropertyPage::OnApply()
{
	OnOK();
	return TRUE;
}

void CPropertyPage::OnReset()
{
	OnCancel();
}

BOOL CPropertyPage::OnSetActive()
{
	if (m_bFirstSetActive)
	{
		m_bFirstSetActive = FALSE;
		return TRUE;
	}
	UpdateData(FALSE);
	return TRUE;
}

BOOL CPropertyPage::OnKillActive()
{
	return UpdateData() ? TRUE : FALSE;
}

BOOL CPropertyPage::OnQueryCancel()
{
	return TRUE;
}

void CPropertyPage::OnOK()
{
}

void CPropertyPage::OnCancel()
{
}

BOOL CPropertyPage::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (CDialog::OnNotify(wParam, lParam, pResult))
	{
		return TRUE;
	}
	const NMHDR* pNMHDR = reinterpret_cast<const NMHDR*>(lParam);
	if (pNMHDR->hwndFrom != m_hWnd && pNMHDR->hwndFrom != ::GetParent(m_hWnd))
	{
		return FALSE;
	}

	switch (pNMHDR->code)
	{
	case PSN_SETACTIVE:
	{
		CPropertySheet* pSheet = GetParentSheet();
		const bool isSheetClosing = pSheet != nullptr && (pSheet->m_nFlags & WF_CONTINUEMODAL) == 0 && !pSheet->m_bModeless;
		*pResult = isSheetClosing || !OnSetActive() ? -1 : 0;
		return TRUE;
	}
	case PSN_KILLACTIVE:
		*pResult = OnKillActive() ? FALSE : TRUE;
		return TRUE;
	case PSN_APPLY:
		*pResult = OnApply() ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
		return TRUE;
	case PSN_RESET:
		OnReset();
		return TRUE;
	case PSN_QUERYCANCEL:
		*pResult = OnQueryCancel() ? FALSE : TRUE;
		return TRUE;
	case PSN_HELP:
		SendMessage(WM_COMMAND, ID_HELP);
		return TRUE;
	default:
		return FALSE;
	}
}

BOOL CPropertyPage::PreTranslateMessage(MSG* pMsg)
{
	CWnd::PreTranslateMessage(pMsg);
	return FALSE;
}

CPropertySheet* CPropertyPage::GetParentSheet()
{
	return DYNAMIC_DOWNCAST(CPropertySheet, GetParent());
}

CPropertySheet::CPropertySheet()
{
	CommonConstruct(nullptr, 0);
}

CPropertySheet::CPropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
{
	m_strCaption.LoadString(nIDCaption);
	CommonConstruct(pParentWnd, iSelectPage);
}

CPropertySheet::CPropertySheet(LPCSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
{
	m_strCaption = pszCaption;
	CommonConstruct(pParentWnd, iSelectPage);
}

CPropertySheet::~CPropertySheet()
{
}

void CPropertySheet::Construct(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
{
	m_strCaption.LoadString(nIDCaption);
	CommonConstruct(pParentWnd, iSelectPage);
}

void CPropertySheet::Construct(LPCSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
{
	m_strCaption = pszCaption;
	CommonConstruct(pParentWnd, iSelectPage);
}

void CPropertySheet::CommonConstruct(CWnd* pParentWnd, UINT iSelectPage)
{
	memset(&m_psh, 0, sizeof(m_psh));
	m_psh.dwSize = sizeof(m_psh);
	m_psh.dwFlags = PSH_PROPSHEETPAGE;
	m_psh.pszCaption = m_strCaption;
	m_psh.nStartPage = iSelectPage;
	m_pParentWnd = pParentWnd;
	m_bModeless = FALSE;
	m_dwStyle = 0;
	m_dwExStyle = 0;
}

void CPropertySheet::BuildPropPageArray()
{
	m_pageArray.clear();
	for (CPropertyPage* pPage : m_pages)
	{
		m_pageArray.push_back(pPage->m_psp);
	}
	m_psh.ppsp = m_pageArray.empty() ? nullptr : m_pageArray.data();
	m_psh.nPages = static_cast<UINT>(m_pageArray.size());
	m_psh.pszCaption = m_strCaption;
}

INT_PTR CPropertySheet::DoModal()
{
	BuildPropPageArray();
	CWinApp* pApp = AfxGetApp();
	if (pApp != nullptr)
	{
		pApp->EnableModeless(FALSE);
	}

	HWND hWndTop = nullptr;
	const HWND hWndParent = CWnd::GetSafeOwner_(m_pParentWnd->GetSafeHwnd(), &hWndTop);
	m_psh.hwndParent = hWndParent;
	const bool enablesParent = hWndParent != nullptr && ::IsWindowEnabled(hWndParent);
	if (enablesParent)
	{
		::EnableWindow(hWndParent, FALSE);
	}
	const HWND hWndCapture = ::GetCapture();
	if (hWndCapture != nullptr)
	{
		::SendMessageA(hWndCapture, WM_CANCELMODE, 0, 0);
	}

	m_nModalResult = 0;
	m_nFlags |= WF_CONTINUEMODAL;
	AfxHookWindowCreate(this);
	m_psh.dwFlags |= PSH_MODELESS;
	const HWND hWnd = reinterpret_cast<HWND>(::PropertySheetA(&m_psh));
	m_psh.dwFlags &= ~PSH_MODELESS;
	AfxUnhookWindowCreate();
	if (hWnd == nullptr || hWnd == reinterpret_cast<HWND>(-1))
	{
		m_nFlags &= ~WF_CONTINUEMODAL;
		m_nModalResult = -1;
	}

	INT_PTR nResult = m_nModalResult;
	if (ContinueModal())
	{
		const DWORD flags = MLF_SHOWONIDLE | ((GetStyle() & DS_NOIDLEMSG) != 0 ? MLF_NOIDLEMSG : 0);
		nResult = RunModalLoop(flags);
	}
	if (m_hWnd != nullptr)
	{
		SetWindowPos(nullptr, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
	}
	if (enablesParent)
	{
		::EnableWindow(hWndParent, TRUE);
	}
	if (hWndParent != nullptr && ::GetActiveWindow() == m_hWnd)
	{
		::SetActiveWindow(hWndParent);
	}
	DestroyWindow();
	if (pApp != nullptr)
	{
		pApp->EnableModeless(TRUE);
	}
	if (hWndTop != nullptr)
	{
		::EnableWindow(hWndTop, TRUE);
	}
	return nResult;
}

BOOL CPropertySheet::Create(CWnd* pParentWnd, DWORD dwStyle, DWORD dwExStyle)
{
	m_dwStyle = dwStyle == static_cast<DWORD>(-1) ? kDefaultSheetStyle : dwStyle;
	m_dwExStyle = dwExStyle;
	BuildPropPageArray();
	m_bModeless = TRUE;
	m_psh.dwFlags |= PSH_MODELESS | PSH_USECALLBACK;
	m_psh.pfnCallback = AfxPropSheetCallback;
	m_psh.hwndParent = pParentWnd->GetSafeHwnd();

	AfxHookWindowCreate(this);
	const HWND hWnd = reinterpret_cast<HWND>(::PropertySheetA(&m_psh));
	if (!AfxUnhookWindowCreate())
	{
		PostNcDestroy();
	}
	return hWnd != nullptr && hWnd != reinterpret_cast<HWND>(-1) ? TRUE : FALSE;
}

void CPropertySheet::AddPage(CPropertyPage* pPage)
{
	m_pages.push_back(pPage);
	if (m_hWnd == nullptr)
	{
		return;
	}
	const HPROPSHEETPAGE hPSP = ::CreatePropertySheetPageA(&pPage->m_psp);
	if (hPSP == nullptr)
	{
		AfxThrowMemoryException();
	}
	if (!SendMessage(PSM_ADDPAGE, 0, reinterpret_cast<LPARAM>(hPSP)))
	{
		::DestroyPropertySheetPage(hPSP);
		AfxThrowMemoryException();
	}
}

void CPropertySheet::RemovePage(CPropertyPage* pPage)
{
	RemovePage(GetPageIndex(pPage));
}

void CPropertySheet::RemovePage(int nPage)
{
	if (nPage < 0 || nPage >= static_cast<int>(m_pages.size()))
	{
		return;
	}
	if (m_hWnd != nullptr)
	{
		SendMessage(PSM_REMOVEPAGE, static_cast<WPARAM>(nPage));
	}
	m_pages.erase(m_pages.begin() + nPage);
}

int CPropertySheet::GetPageCount() const
{
	if (m_hWnd == nullptr)
	{
		return static_cast<int>(m_pages.size());
	}
	return GetTabControl()->GetItemCount();
}

CPropertyPage* CPropertySheet::GetPage(int nPage) const
{
	return nPage >= 0 && nPage < static_cast<int>(m_pages.size()) ? m_pages[static_cast<size_t>(nPage)] : nullptr;
}

int CPropertySheet::GetPageIndex(CPropertyPage* pPage)
{
	for (size_t index = 0; index < m_pages.size(); ++index)
	{
		if (m_pages[index] == pPage)
		{
			return static_cast<int>(index);
		}
	}
	return -1;
}

CPropertyPage* CPropertySheet::GetActivePage() const
{
	if (m_hWnd == nullptr)
	{
		return GetPage(GetActiveIndex());
	}
	const HWND hWndPage = reinterpret_cast<HWND>(::SendMessageA(m_hWnd, PSM_GETCURRENTPAGEHWND, 0, 0));
	return DYNAMIC_DOWNCAST(CPropertyPage, CWnd::FromHandle(hWndPage));
}

int CPropertySheet::GetActiveIndex() const
{
	if (m_hWnd == nullptr)
	{
		return static_cast<int>(m_psh.nStartPage);
	}
	return GetTabControl()->GetCurSel();
}

BOOL CPropertySheet::SetActivePage(int nPage)
{
	if (m_hWnd == nullptr)
	{
		m_psh.nStartPage = static_cast<UINT>(nPage);
		return TRUE;
	}
	return static_cast<BOOL>(SendMessage(PSM_SETCURSEL, static_cast<WPARAM>(nPage)));
}

BOOL CPropertySheet::SetActivePage(CPropertyPage* pPage)
{
	const int nPage = GetPageIndex(pPage);
	return nPage >= 0 ? SetActivePage(nPage) : FALSE;
}

void CPropertySheet::SetTitle(LPCSTR lpszText, UINT nStyle)
{
	m_psh.dwFlags = (m_psh.dwFlags & ~PSH_PROPTITLE) | (nStyle & PSH_PROPTITLE);
	m_strCaption = lpszText;
	m_psh.pszCaption = m_strCaption;
	if (m_hWnd != nullptr)
	{
		SendMessage(PSM_SETTITLEA, nStyle, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(m_strCaption)));
	}
}

CTabCtrl* CPropertySheet::GetTabControl() const
{
	return static_cast<CTabCtrl*>(CWnd::FromHandle(reinterpret_cast<HWND>(::SendMessageA(m_hWnd, PSM_GETTABCONTROL, 0, 0))));
}

void CPropertySheet::EndDialog(int nEndID)
{
	EndModalLoop(nEndID);
	if (m_bModeless)
	{
		DestroyWindow();
		return;
	}
	PostMessage(PSM_PRESSBUTTON, PSBTN_CANCEL);
}

void CPropertySheet::PressButton(int nButton)
{
	SendMessage(PSM_PRESSBUTTON, static_cast<WPARAM>(nButton));
}

BOOL CPropertySheet::ContinueModal()
{
	if (!CWnd::ContinueModal())
	{
		return FALSE;
	}
	return SendMessage(PSM_GETCURRENTPAGEHWND) != 0 ? TRUE : FALSE;
}

BOOL CPropertySheet::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	if (CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
	{
		return TRUE;
	}
	const UINT code = static_cast<UINT>(nCode);
	const bool isCommandId = (nID & 0x8000) != 0;
	if ((code != CN_COMMAND && code != CN_UPDATE_COMMAND_UI) || !isCommandId || nID >= 0xF000)
	{
		return FALSE;
	}
	CWinThread* thread = AfxGetThread();
	return thread != nullptr ? thread->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) : FALSE;
}

BOOL CPropertySheet::PreTranslateMessage(MSG* pMsg)
{
	if (CWnd::PreTranslateMessage(pMsg))
	{
		return TRUE;
	}
	const bool isPageKey = pMsg->message == WM_KEYDOWN && ::GetKeyState(VK_CONTROL) < 0
		&& (pMsg->wParam == VK_TAB || pMsg->wParam == VK_PRIOR || pMsg->wParam == VK_NEXT);
	if (isPageKey && SendMessage(PSM_ISDIALOGMESSAGE, 0, reinterpret_cast<LPARAM>(pMsg)))
	{
		return TRUE;
	}
	return PreTranslateInput(pMsg);
}

BOOL CPropertySheet::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (CWnd::OnCommand(wParam, lParam))
	{
		return TRUE;
	}
	const HWND hWndCtrl = reinterpret_cast<HWND>(lParam);
	if (hWndCtrl != nullptr && HIWORD(wParam) == BN_CLICKED && isPushButton(hWndCtrl))
	{
		m_nModalResult = LOWORD(wParam);
	}
	return FALSE;
}

void CPropertySheet::OnClose()
{
	m_nModalResult = IDCANCEL;
	if (m_bModeless)
	{
		DestroyWindow();
		return;
	}
	Default();
}

void CPropertySheet::OnSysCommand(UINT nID, LPARAM)
{
	m_nModalResult = IDCANCEL;
	if ((nID & 0xFFF0) == SC_CLOSE && m_bModeless)
	{
		SendMessage(WM_CLOSE);
		return;
	}
	Default();
}
