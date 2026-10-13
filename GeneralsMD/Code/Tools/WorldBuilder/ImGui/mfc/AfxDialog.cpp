#include "AfxInternal.h"

#include "Win32Resources.h"

#include <strings.h>

IMPLEMENT_DYNAMIC(CUserException, CException)
IMPLEMENT_DYNAMIC(CDialog, CWnd)

BEGIN_MESSAGE_MAP(CDialog, CWnd)
	ON_COMMAND(IDOK, OnOK)
	ON_COMMAND(IDCANCEL, OnCancel)
	ON_MESSAGE(WM_INITDIALOG, HandleInitDialog)
END_MESSAGE_MAP()

INT_PTR CALLBACK AfxDlgProc(HWND hWnd, UINT message, WPARAM, LPARAM)
{
	if (message != WM_INITDIALOG)
	{
		return 0;
	}
	CDialog* dialog = DYNAMIC_DOWNCAST(CDialog, CWnd::FromHandlePermanent(hWnd));
	return dialog != nullptr ? dialog->OnInitDialog() : 1;
}

void AFXAPI AfxThrowUserException()
{
	throw new CUserException;
}

CDataExchange::CDataExchange(CWnd* pDlgWnd, BOOL bSaveAndValidate) :
	m_bSaveAndValidate(bSaveAndValidate),
	m_pDlgWnd(pDlgWnd),
	m_hWndLastControl(nullptr),
	m_bEditLastControl(FALSE)
{
}

HWND CDataExchange::PrepareCtrl(int nIDC)
{
	m_hWndLastControl = ::GetDlgItem(m_pDlgWnd->m_hWnd, nIDC);
	m_bEditLastControl = FALSE;
	return m_hWndLastControl;
}

HWND CDataExchange::PrepareEditCtrl(int nIDC)
{
	const HWND control = PrepareCtrl(nIDC);
	m_bEditLastControl = TRUE;
	return control;
}

void CDataExchange::Fail()
{
	if (!m_bSaveAndValidate)
	{
		AfxThrowUserException();
	}
	if (m_hWndLastControl != nullptr)
	{
		::SetFocus(m_hWndLastControl);
		if (m_bEditLastControl)
		{
			::SendMessageA(m_hWndLastControl, EM_SETSEL, 0, -1);
		}
	}
	AfxThrowUserException();
}

CDialog::CDialog() :
	m_lpszTemplateName(nullptr),
	m_nIDHelp(0),
	m_pParentWnd(nullptr),
	m_hWndTop(nullptr)
{
}

CDialog::CDialog(LPCSTR lpszTemplateName, CWnd* pParentWnd) :
	m_lpszTemplateName(lpszTemplateName),
	m_nIDHelp(IS_INTRESOURCE(lpszTemplateName) ? LOWORD(reinterpret_cast<uintptr_t>(lpszTemplateName)) : 0),
	m_pParentWnd(pParentWnd),
	m_hWndTop(nullptr)
{
}

CDialog::CDialog(UINT nIDTemplate, CWnd* pParentWnd) :
	m_lpszTemplateName(MAKEINTRESOURCE(nIDTemplate)),
	m_nIDHelp(nIDTemplate),
	m_pParentWnd(pParentWnd),
	m_hWndTop(nullptr)
{
}

CDialog::~CDialog()
{
	if (m_hWnd != nullptr)
	{
		DestroyWindow();
	}
}

BOOL CDialog::CreateDlgFromTemplate(CWnd* pParentWnd)
{
	m_nModalResult = -1;
	m_nFlags |= WF_CONTINUEMODAL;
	AfxHookWindowCreate(this);
	const HWND hWnd = ::CreateDialogParamA(AfxGetResourceHandle(), m_lpszTemplateName, pParentWnd->GetSafeHwnd(), AfxDlgProc, 0);
	if (!AfxUnhookWindowCreate())
	{
		PostNcDestroy();
	}
	if (hWnd != nullptr && (m_nFlags & WF_CONTINUEMODAL) == 0)
	{
		::DestroyWindow(hWnd);
		return FALSE;
	}
	return hWnd != nullptr ? TRUE : FALSE;
}

BOOL CDialog::Create(LPCSTR lpszTemplateName, CWnd* pParentWnd)
{
	m_lpszTemplateName = lpszTemplateName;
	if (IS_INTRESOURCE(lpszTemplateName) && m_nIDHelp == 0)
	{
		m_nIDHelp = LOWORD(reinterpret_cast<uintptr_t>(lpszTemplateName));
	}
	return CreateDlgFromTemplate(pParentWnd);
}

BOOL CDialog::Create(UINT nIDTemplate, CWnd* pParentWnd)
{
	return Create(MAKEINTRESOURCE(nIDTemplate), pParentWnd);
}

HWND CDialog::PreModal()
{
	AfxGetApp()->EnableModeless(FALSE);
	const HWND owner = CWnd::GetSafeOwner_(m_pParentWnd->GetSafeHwnd(), &m_hWndTop);
	AfxHookWindowCreate(this);
	return owner;
}

void CDialog::PostModal()
{
	AfxUnhookWindowCreate();
	Detach();
	if (::IsWindow(m_hWndTop))
	{
		::EnableWindow(m_hWndTop, TRUE);
	}
	m_hWndTop = nullptr;
	AfxGetApp()->EnableModeless(TRUE);
}

void CDialog::runModal(HWND parent)
{
	if (!CreateDlgFromTemplate(CWnd::FromHandle(parent)))
	{
		return;
	}
	if ((m_nFlags & WF_CONTINUEMODAL) != 0)
	{
		const DWORD flags = MLF_SHOWONIDLE | ((GetStyle() & DS_NOIDLEMSG) != 0 ? MLF_NOIDLEMSG : 0);
		RunModalLoop(flags);
	}
	if (m_hWnd != nullptr)
	{
		SetWindowPos(nullptr, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
	}
}

INT_PTR CDialog::DoModal()
{
	const HWND parent = PreModal();
	AfxUnhookWindowCreate();
	const bool disablesParent = parent != nullptr && parent != ::GetDesktopWindow() && ::IsWindowEnabled(parent);
	if (disablesParent)
	{
		::EnableWindow(parent, FALSE);
	}

	try
	{
		runModal(parent);
	}
	catch (CException* exception)
	{
		exception->Delete();
		m_nModalResult = -1;
	}

	if (disablesParent)
	{
		::EnableWindow(parent, TRUE);
	}
	if (parent != nullptr && ::GetActiveWindow() == m_hWnd)
	{
		::SetActiveWindow(parent);
	}
	DestroyWindow();
	PostModal();
	return m_nModalResult;
}

void CDialog::EndDialog(int nResult)
{
	if ((m_nFlags & (WF_MODALLOOP | WF_CONTINUEMODAL)) != 0)
	{
		EndModalLoop(nResult);
	}
	::EndDialog(m_hWnd, nResult);
}

DWORD CDialog::GetDefID() const { return static_cast<DWORD>(SendMessage(DM_GETDEFID)); }
void CDialog::SetDefID(UINT nID) { SendMessage(DM_SETDEFID, nID); }
void CDialog::NextDlgCtrl() const { SendMessage(WM_NEXTDLGCTL, 0, 0); }
void CDialog::PrevDlgCtrl() const { SendMessage(WM_NEXTDLGCTL, 1, 0); }
void CDialog::GotoDlgCtrl(CWnd* pWndCtrl) { SendMessage(WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(pWndCtrl->m_hWnd), 1); }
void CDialog::MapDialogRect(LPRECT lpRect) const { ::MapDialogRect(m_hWnd, lpRect); }

BOOL CDialog::OnInitDialog()
{
	if (!ExecuteDlgInit(m_lpszTemplateName) || !UpdateData(FALSE))
	{
		EndDialog(-1);
		return FALSE;
	}
	return TRUE;
}

void CDialog::PreInitDialog()
{
}

LRESULT CDialog::HandleInitDialog(WPARAM, LPARAM)
{
	PreInitDialog();
	return Default();
}

void CDialog::OnOK()
{
	if (!UpdateData(TRUE))
	{
		return;
	}
	EndDialog(IDOK);
}

void CDialog::OnCancel()
{
	EndDialog(IDCANCEL);
}

BOOL CDialog::PreTranslateMessage(MSG* pMsg)
{
	if (CWnd::PreTranslateMessage(pMsg))
	{
		return TRUE;
	}
	const bool isEscape = pMsg->message == WM_KEYDOWN && (pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_CANCEL);
	const bool isMultilineEdit = (::GetWindowLongA(pMsg->hwnd, GWL_STYLE) & ES_MULTILINE) != 0 && ::IsChild(m_hWnd, pMsg->hwnd);
	if (isEscape && isMultilineEdit)
	{
		char className[16] = {};
		::GetClassNameA(pMsg->hwnd, className, sizeof(className));
		const HWND cancel = ::GetDlgItem(m_hWnd, IDCANCEL);
		if (strcasecmp(className, "Edit") == 0 && (cancel == nullptr || ::IsWindowEnabled(cancel)))
		{
			SendMessage(WM_COMMAND, IDCANCEL, 0);
			return TRUE;
		}
	}
	return PreTranslateInput(pMsg);
}

BOOL CDialog::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
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

BOOL CDialog::CheckAutoCenter()
{
	UINT id = 0;
	if (!Win32Resources::instance().resolveName(m_lpszTemplateName, id))
	{
		return TRUE;
	}
	const Win32DialogTemplate* dialogTemplate = Win32Resources::instance().dialog(id);
	return dialogTemplate == nullptr || (dialogTemplate->style & (DS_CENTER | DS_CENTERMOUSE | DS_ABSALIGN)) == 0 ? TRUE : FALSE;
}
