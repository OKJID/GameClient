#include "AfxInternal.h"

#include <cstdlib>
#include <cstring>

IMPLEMENT_DYNAMIC(CMemoryException, CException)
IMPLEMENT_DYNAMIC(CWinThread, CCmdTarget)
IMPLEMENT_DYNAMIC(CWinApp, CWinThread)

BEGIN_MESSAGE_MAP(CWinApp, CCmdTarget)
	ON_COMMAND(ID_APP_EXIT, OnAppExit)
	ON_UPDATE_COMMAND_UI(ID_FILE_MRU_FILE1, OnUpdateRecentFileMenu)
	ON_COMMAND_EX_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE16, OnOpenRecentFile)
END_MESSAGE_MAP()

int __argc = 0;
char** __argv = nullptr;

namespace
{

const char kRecentFileSection[] = "Recent File List";
const char kRecentFileEntry[] = "File%d";
const char kPreviewSection[] = "Settings";
const char kPreviewEntry[] = "PreviewPages";
const UINT kSystemTimerMessage = 0x0118;

LPSTR duplicateString(LPCSTR text)
{
	return text != nullptr ? strdup(text) : nullptr;
}

UINT withDefaultIcon(UINT nType)
{
	if ((nType & MB_ICONMASK) != 0)
	{
		return nType;
	}
	switch (nType & MB_TYPEMASK)
	{
	case MB_OK:
	case MB_OKCANCEL:
	case MB_YESNO:
	case MB_YESNOCANCEL:
		return nType | MB_ICONEXCLAMATION;
	default:
		return nType;
	}
}

CString executableTitle()
{
	char path[_MAX_PATH] = {};
	::GetModuleFileName(nullptr, path, sizeof(path));
	char* name = strrchr(path, '\\');
	name = name != nullptr ? name + 1 : path;
	char* extension = strrchr(name, '.');
	if (extension != nullptr)
	{
		*extension = '\0';
	}
	return name;
}

}

BOOL CException::GetErrorMessage(LPSTR lpszError, UINT nMaxError, UINT* pnHelpContext)
{
	if (pnHelpContext != nullptr)
	{
		*pnHelpContext = 0;
	}
	if (lpszError != nullptr && nMaxError != 0)
	{
		*lpszError = '\0';
	}
	return FALSE;
}

int CException::ReportError(UINT nType, UINT nMessageID)
{
	char message[512];
	UINT helpContext = 0;
	if (GetErrorMessage(message, sizeof(message), &helpContext))
	{
		return AfxMessageBox(message, nType, helpContext);
	}
	return AfxMessageBox(nMessageID != 0 ? nMessageID : AFX_IDP_NO_ERROR_AVAILABLE, nType, helpContext);
}

BOOL CMemoryException::GetErrorMessage(LPSTR lpszError, UINT nMaxError, UINT* pnHelpContext)
{
	if (pnHelpContext != nullptr)
	{
		*pnHelpContext = 0;
	}
	if (lpszError == nullptr || nMaxError == 0)
	{
		return FALSE;
	}
	return ::LoadStringA(AfxGetResourceHandle(), AFX_IDS_MEMORY_EXCEPTION, lpszError, static_cast<int>(nMaxError)) != 0 ? TRUE : FALSE;
}

void AFXAPI AfxThrowMemoryException()
{
	throw new CMemoryException;
}

CWinThread::CWinThread() :
	m_pMainWnd(nullptr),
	m_pActiveWnd(nullptr),
	m_msgCur(),
	m_ptCursorLast(0, 0),
	m_nMsgLast(0)
{
}

CWinThread::~CWinThread()
{
	AfxModuleState& state = AfxGetModuleState();
	if (state.pCurrentWinThread == this)
	{
		state.pCurrentWinThread = nullptr;
	}
}

BOOL CWinThread::InitInstance()
{
	return FALSE;
}

int CWinThread::Run()
{
	BOOL isIdle = TRUE;
	LONG idleCount = 0;
	for (;;)
	{
		while (isIdle && !::PeekMessageA(&m_msgCur, nullptr, 0, 0, PM_NOREMOVE))
		{
			if (!OnIdle(idleCount++))
			{
				isIdle = FALSE;
			}
		}
		do
		{
			if (!PumpMessage())
			{
				return ExitInstance();
			}
			if (IsIdleMessage(&m_msgCur))
			{
				isIdle = TRUE;
				idleCount = 0;
			}
		} while (::PeekMessageA(&m_msgCur, nullptr, 0, 0, PM_NOREMOVE));
	}
}

BOOL CWinThread::PumpMessage()
{
	if (!::GetMessageA(&m_msgCur, nullptr, 0, 0))
	{
		return FALSE;
	}
	if (m_msgCur.message != WM_KICKIDLE && !PreTranslateMessage(&m_msgCur))
	{
		::TranslateMessage(&m_msgCur);
		::DispatchMessageA(&m_msgCur);
	}
	return TRUE;
}

BOOL CWinThread::IsIdleMessage(MSG* pMsg)
{
	if (pMsg->message == WM_MOUSEMOVE || pMsg->message == WM_NCMOUSEMOVE)
	{
		if (m_ptCursorLast == pMsg->pt && pMsg->message == m_nMsgLast)
		{
			return FALSE;
		}
		m_ptCursorLast = pMsg->pt;
		m_nMsgLast = pMsg->message;
		return TRUE;
	}
	return pMsg->message != WM_PAINT && pMsg->message != kSystemTimerMessage ? TRUE : FALSE;
}

BOOL CWinThread::PreTranslateMessage(MSG* pMsg)
{
	CWnd* mainWnd = AfxGetMainWnd();
	if (CWnd::WalkPreTranslateTree(mainWnd->GetSafeHwnd(), pMsg))
	{
		return TRUE;
	}
	if (mainWnd == nullptr)
	{
		return FALSE;
	}
	CWnd* target = CWnd::FromHandle(pMsg->hwnd);
	if (target != nullptr && target->GetTopLevelParent() != mainWnd)
	{
		return mainWnd->PreTranslateMessage(pMsg);
	}
	return FALSE;
}

BOOL CWinThread::OnIdle(LONG lCount)
{
	if (lCount <= 0)
	{
		updateFrameCommandUI(m_pMainWnd);
	}
	else
	{
		AfxDeleteTemporaryObjects();
	}
	return lCount < 0;
}

void CWinThread::updateFrameCommandUI(CWnd* pMainWnd)
{
	if (pMainWnd != nullptr && pMainWnd->m_hWnd != nullptr && pMainWnd->IsWindowVisible())
	{
		AfxCallWndProc(pMainWnd, pMainWnd->m_hWnd, WM_IDLEUPDATECMDUI, TRUE, 0);
		pMainWnd->SendMessageToDescendants(WM_IDLEUPDATECMDUI, TRUE, 0, TRUE, TRUE);
	}

	const std::vector<CFrameWnd*> frames = AfxGetThreadState().frames;
	for (CFrameWnd* frame : frames)
	{
		if (frame->m_hWnd == nullptr || frame == pMainWnd)
		{
			continue;
		}
		if (frame->m_nShowDelay == SW_HIDE)
		{
			frame->ShowWindow(SW_HIDE);
		}
		if (frame->IsWindowVisible() || frame->m_nShowDelay >= 0)
		{
			AfxCallWndProc(frame, frame->m_hWnd, WM_IDLEUPDATECMDUI, TRUE, 0);
			frame->SendMessageToDescendants(WM_IDLEUPDATECMDUI, TRUE, 0, TRUE, TRUE);
		}
		if (frame->m_nShowDelay > SW_HIDE)
		{
			frame->ShowWindow(frame->m_nShowDelay);
		}
		frame->m_nShowDelay = -1;
	}
}

int CWinThread::ExitInstance()
{
	return static_cast<int>(m_msgCur.wParam);
}

CWnd* CWinThread::GetMainWnd()
{
	if (m_pActiveWnd != nullptr)
	{
		return m_pActiveWnd;
	}
	if (m_pMainWnd != nullptr)
	{
		return m_pMainWnd;
	}
	return CWnd::GetActiveWindow();
}

CWinApp::CWinApp(LPCSTR lpszAppName) :
	m_hInstance(nullptr),
	m_lpCmdLine(nullptr),
	m_nCmdShow(SW_SHOWNORMAL),
	m_pszAppName(duplicateString(lpszAppName)),
	m_pszExeName(nullptr),
	m_pszProfileName(nullptr),
	m_pDocManager(nullptr),
	m_pRecentFileList(nullptr),
	m_pCmdInfo(nullptr),
	m_nNumPreviewPages(0),
	m_nWaitCursorCount(0),
	m_hcurWaitCursorRestore(nullptr)
{
	AfxModuleState& state = AfxGetModuleState();
	state.pCurrentWinThread = this;
	state.pCurrentWinApp = this;
}

CWinApp::~CWinApp()
{
	delete m_pDocManager;
	delete m_pRecentFileList;
	AfxModuleState& state = AfxGetModuleState();
	if (state.lpszCurrentAppName == m_pszAppName)
	{
		state.lpszCurrentAppName = nullptr;
	}
	if (state.pCurrentWinApp == this)
	{
		state.pCurrentWinApp = nullptr;
	}
	free(const_cast<LPSTR>(m_pszAppName));
	free(const_cast<LPSTR>(m_pszExeName));
	free(const_cast<LPSTR>(m_pszProfileName));
}

void CWinApp::SetCurrentHandles()
{
	AfxModuleState& state = AfxGetModuleState();
	state.hCurrentInstanceHandle = m_hInstance;
	state.hCurrentResourceHandle = m_hInstance;

	const CString exeName = executableTitle();
	if (m_pszExeName == nullptr)
	{
		m_pszExeName = duplicateString(exeName);
	}
	if (m_pszAppName == nullptr)
	{
		CString title;
		m_pszAppName = duplicateString(title.LoadString(AFX_IDS_APP_TITLE) ? static_cast<LPCSTR>(title) : m_pszExeName);
	}
	state.lpszCurrentAppName = m_pszAppName;
	if (m_pszProfileName == nullptr)
	{
		m_pszProfileName = duplicateString(exeName + ".INI");
	}
}

BOOL CWinApp::InitApplication()
{
	return TRUE;
}

BOOL CWinApp::InitInstance()
{
	return TRUE;
}

int CWinApp::Run()
{
	if (m_pMainWnd == nullptr)
	{
		AfxPostQuitMessage(0);
	}
	return CWinThread::Run();
}

BOOL CWinApp::OnIdle(LONG lCount)
{
	if (lCount <= 1)
	{
		CWinThread::OnIdle(lCount);
	}
	return lCount < 1;
}

int CWinApp::ExitInstance()
{
	SaveStdProfileSettings();
	return static_cast<int>(m_msgCur.wParam);
}

int CWinApp::DoMessageBox(LPCSTR lpszPrompt, UINT nType, UINT)
{
	EnableModeless(FALSE);
	HWND hWndTop = nullptr;
	const HWND owner = CWnd::GetSafeOwner_(nullptr, &hWndTop);
	const int result = ::MessageBoxA(owner, lpszPrompt, m_pszAppName, withDefaultIcon(nType) | MB_TASKMODAL);
	if (hWndTop != nullptr)
	{
		::EnableWindow(hWndTop, TRUE);
	}
	EnableModeless(TRUE);
	return result;
}

void CWinApp::DoWaitCursor(int nCode)
{
	m_nWaitCursorCount += nCode;
	if (m_nWaitCursorCount <= 0)
	{
		m_nWaitCursorCount = 0;
		::SetCursor(m_hcurWaitCursorRestore);
		return;
	}
	const HCURSOR previous = ::SetCursor(::LoadCursorA(nullptr, IDC_WAIT));
	if (nCode > 0 && m_nWaitCursorCount == 1)
	{
		m_hcurWaitCursorRestore = previous;
	}
}

void CWinApp::HideApplication()
{
	m_pMainWnd->ShowWindow(SW_HIDE);
	::ShowOwnedPopups(m_pMainWnd->m_hWnd, FALSE);
	m_pMainWnd->SetWindowPos(&CWnd::wndBottom, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void CWinApp::EnableModeless(BOOL)
{
}

BOOL CWinApp::Enable3dControls()
{
	return TRUE;
}

BOOL CWinApp::Enable3dControlsStatic()
{
	return TRUE;
}

void CWinApp::LoadStdProfileSettings(UINT nMaxMRU)
{
	if (nMaxMRU != 0)
	{
		m_pRecentFileList = new CRecentFileList(0, kRecentFileSection, kRecentFileEntry, static_cast<int>(nMaxMRU));
		m_pRecentFileList->ReadList();
	}
	m_nNumPreviewPages = static_cast<int>(GetProfileInt(kPreviewSection, kPreviewEntry, 0));
}

void CWinApp::SaveStdProfileSettings()
{
	if (m_pRecentFileList != nullptr)
	{
		m_pRecentFileList->WriteList();
	}
	if (m_nNumPreviewPages != 0)
	{
		WriteProfileInt(kPreviewSection, kPreviewEntry, m_nNumPreviewPages);
	}
}

UINT CWinApp::GetProfileInt(LPCSTR lpszSection, LPCSTR lpszEntry, int nDefault)
{
	return ::GetPrivateProfileIntA(lpszSection, lpszEntry, nDefault, m_pszProfileName);
}

BOOL CWinApp::WriteProfileInt(LPCSTR lpszSection, LPCSTR lpszEntry, int nValue)
{
	char value[16];
	snprintf(value, sizeof(value), "%d", nValue);
	return ::WritePrivateProfileStringA(lpszSection, lpszEntry, value, m_pszProfileName);
}

CString CWinApp::GetProfileString(LPCSTR lpszSection, LPCSTR lpszEntry, LPCSTR lpszDefault)
{
	char value[4096];
	::GetPrivateProfileStringA(lpszSection, lpszEntry, lpszDefault != nullptr ? lpszDefault : "", value, sizeof(value), m_pszProfileName);
	return value;
}

BOOL CWinApp::WriteProfileString(LPCSTR lpszSection, LPCSTR lpszEntry, LPCSTR lpszValue)
{
	return ::WritePrivateProfileStringA(lpszSection, lpszEntry, lpszValue, m_pszProfileName);
}

HCURSOR CWinApp::LoadCursor(LPCSTR lpszResourceName) const { return ::LoadCursorA(AfxGetResourceHandle(), lpszResourceName); }
HCURSOR CWinApp::LoadCursor(UINT nIDResource) const { return ::LoadCursorA(AfxGetResourceHandle(), MAKEINTRESOURCE(nIDResource)); }
HCURSOR CWinApp::LoadStandardCursor(LPCSTR lpszCursorName) const { return ::LoadCursorA(nullptr, lpszCursorName); }
HICON CWinApp::LoadIcon(LPCSTR lpszResourceName) const { return ::LoadIconA(AfxGetResourceHandle(), lpszResourceName); }
HICON CWinApp::LoadIcon(UINT nIDResource) const { return ::LoadIconA(AfxGetResourceHandle(), MAKEINTRESOURCE(nIDResource)); }
HICON CWinApp::LoadStandardIcon(LPCSTR lpszIconName) const { return ::LoadIconA(nullptr, lpszIconName); }

void CWinApp::OnAppExit()
{
	m_pMainWnd->SendMessage(WM_CLOSE);
}

CWinApp* AFXAPI AfxGetApp()
{
	return AfxGetModuleState().pCurrentWinApp;
}

CWinThread* AFXAPI AfxGetThread()
{
	return AfxGetModuleState().pCurrentWinThread;
}

CWnd* AFXAPI AfxGetMainWnd()
{
	CWinThread* thread = AfxGetThread();
	return thread != nullptr ? thread->GetMainWnd() : nullptr;
}

HINSTANCE AFXAPI AfxGetInstanceHandle()
{
	return AfxGetModuleState().hCurrentInstanceHandle;
}

HINSTANCE AFXAPI AfxGetResourceHandle()
{
	return AfxGetModuleState().hCurrentResourceHandle;
}

LPCSTR AFXAPI AfxGetAppName()
{
	return AfxGetModuleState().lpszCurrentAppName;
}

void AFXAPI AfxPostQuitMessage(int nExitCode)
{
	::PostQuitMessage(nExitCode);
}

int AFXAPI AfxMessageBox(LPCSTR lpszText, UINT nType, UINT nIDHelp)
{
	CWinApp* app = AfxGetApp();
	if (app == nullptr)
	{
		return ::MessageBoxA(nullptr, lpszText, nullptr, withDefaultIcon(nType) | MB_TASKMODAL);
	}
	return app->DoMessageBox(lpszText, nType, nIDHelp);
}

int AFXAPI AfxMessageBox(UINT nIDPrompt, UINT nType, UINT nIDHelp)
{
	CString prompt;
	if (!prompt.LoadString(nIDPrompt))
	{
		return 0;
	}
	return AfxMessageBox(prompt, nType, nIDHelp == static_cast<UINT>(-1) ? nIDPrompt : nIDHelp);
}

BOOL AFXAPI AfxWinInit(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow)
{
	AfxModuleState& state = AfxGetModuleState();
	state.hCurrentInstanceHandle = hInstance;
	state.hCurrentResourceHandle = hInstance;
	CWinApp* app = AfxGetApp();
	if (app != nullptr)
	{
		app->m_hInstance = hInstance;
		app->m_lpCmdLine = lpCmdLine;
		app->m_nCmdShow = nCmdShow;
		app->SetCurrentHandles();
	}
	return TRUE;
}

int AFXAPI AfxWinMain(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow)
{
	CWinThread* thread = AfxGetThread();
	CWinApp* app = AfxGetApp();
	if (!AfxWinInit(hInstance, lpCmdLine, nCmdShow) || (app != nullptr && !app->InitApplication()))
	{
		return -1;
	}
	if (thread->InitInstance())
	{
		return thread->Run();
	}
	if (thread->m_pMainWnd != nullptr)
	{
		thread->m_pMainWnd->DestroyWindow();
	}
	return thread->ExitInstance();
}
