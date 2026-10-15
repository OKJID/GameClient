#include "AfxInternal.h"

BEGIN_MESSAGE_MAP(CWnd, CCmdTarget)
	ON_WM_NCDESTROY()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

namespace
{

class AfxTestCmdUI : public CCmdUI
{
public:
	void Enable(BOOL bOn) override
	{
		m_bEnabled = bOn;
		m_bEnableChanged = TRUE;
	}
	void SetCheck(int) override {}
	void SetRadio(BOOL) override {}
	void SetText(LPCSTR) override {}

	BOOL m_bEnabled = TRUE;
};

void handleActivate(CWnd* window, WPARAM nState, CWnd* other)
{
	if ((window->GetStyle() & WS_CHILD) != 0)
	{
		return;
	}
	CWnd* topLevel = window->GetTopLevelParent();
	const bool changesTopLevel = other == nullptr || !::IsWindow(other->m_hWnd) || topLevel != other->GetTopLevelParent();
	if (topLevel == nullptr || !changesTopLevel)
	{
		return;
	}
	HWND windows[2] = { window->m_hWnd, other != nullptr ? other->m_hWnd : nullptr };
	topLevel->SendMessage(WM_ACTIVATETOPLEVEL, nState, reinterpret_cast<LPARAM>(&windows[0]));
}

struct AfxModalLoopState
{
	DWORD flags = 0;
	HWND parent = nullptr;
	BOOL isIdle = TRUE;
	BOOL showsWhenIdle = FALSE;
	LONG idleCount = 0;
};

void showModalWindow(CWnd* window, AfxModalLoopState& state)
{
	window->ShowWindow(SW_SHOWNORMAL);
	window->UpdateWindow();
	state.showsWhenIdle = FALSE;
}

void runModalIdle(CWnd* window, AfxModalLoopState& state)
{
	if (state.showsWhenIdle)
	{
		showModalWindow(window, state);
	}
	if ((state.flags & MLF_NOIDLEMSG) == 0 && state.parent != nullptr && state.idleCount == 0)
	{
		::SendMessageA(state.parent, WM_ENTERIDLE, MSGF_DIALOGBOX, reinterpret_cast<LPARAM>(window->m_hWnd));
	}
	if ((state.flags & MLF_NOKICKIDLE) != 0 || !window->SendMessage(WM_KICKIDLE, MSGF_DIALOGBOX, state.idleCount++))
	{
		state.isIdle = FALSE;
	}
}

BOOL handleSetCursor(CWnd* window, short nHitTest, UINT nMsg)
{
	const bool isClick = nMsg == WM_LBUTTONDOWN || nMsg == WM_MBUTTONDOWN || nMsg == WM_RBUTTONDOWN;
	if (nHitTest != HTERROR || !isClick)
	{
		return FALSE;
	}
	CWnd* lastActive = window->GetTopLevelParent();
	lastActive = lastActive != nullptr ? lastActive->GetLastActivePopup() : nullptr;
	if (lastActive == nullptr || lastActive == CWnd::GetForegroundWindow() || !lastActive->IsWindowEnabled())
	{
		return FALSE;
	}
	lastActive->SetForegroundWindow();
	return TRUE;
}

}

BOOL CWnd::dispatchMessageMap(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	for (const AFX_MSGMAP* messageMap = GetMessageMap(); messageMap != nullptr;
		messageMap = messageMap->pfnGetBaseMap != nullptr ? messageMap->pfnGetBaseMap() : nullptr)
	{
		const AFX_MSGMAP_ENTRY* entry = AfxFindMessageEntry(messageMap->lpEntries, message, 0, 0);
		if (entry == nullptr)
		{
			continue;
		}
		AfxMessageCall call;
		call.nMessage = message;
		call.wParam = wParam;
		call.lParam = lParam;
		entry->pfn(this, call);
		lResult = call.lResult;
		return TRUE;
	}
	return FALSE;
}

LRESULT CWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	if (!OnWndMsg(message, wParam, lParam, &result))
	{
		result = DefWindowProc(message, wParam, lParam);
	}
	return result;
}

BOOL CWnd::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	LRESULT result = 0;
	if (message == WM_COMMAND)
	{
		if (!OnCommand(wParam, lParam))
		{
			return FALSE;
		}
		result = 1;
	}
	else if (message == WM_NOTIFY)
	{
		const NMHDR* header = reinterpret_cast<const NMHDR*>(lParam);
		if (header->hwndFrom == nullptr || !OnNotify(wParam, lParam, &result))
		{
			return FALSE;
		}
	}
	else if (message == WM_SETCURSOR && handleSetCursor(this, static_cast<short>(LOWORD(lParam)), HIWORD(lParam)))
	{
		result = 1;
	}
	else
	{
		if (message == WM_ACTIVATE)
		{
			handleActivate(this, wParam, CWnd::FromHandle(reinterpret_cast<HWND>(lParam)));
		}
		if (!dispatchMessageMap(message, wParam, lParam, result))
		{
			return FALSE;
		}
	}
	if (pResult != nullptr)
	{
		*pResult = result;
	}
	return TRUE;
}

LRESULT CWnd::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (m_pfnSuper != nullptr)
	{
		return ::CallWindowProcA(m_pfnSuper, m_hWnd, message, wParam, lParam);
	}
	const WNDPROC superProc = *GetSuperWndProcAddr();
	if (superProc == nullptr)
	{
		return ::DefWindowProcA(m_hWnd, message, wParam, lParam);
	}
	return ::CallWindowProcA(superProc, m_hWnd, message, wParam, lParam);
}

LRESULT CWnd::Default()
{
	const MSG& current = AfxGetThreadState().lastSentMsg;
	return DefWindowProc(current.message, current.wParam, current.lParam);
}

BOOL CWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	const UINT nID = LOWORD(wParam);
	const HWND control = reinterpret_cast<HWND>(lParam);
	int nCode = HIWORD(wParam);
	if (control == nullptr)
	{
		if (nID == 0)
		{
			return FALSE;
		}
		AfxTestCmdUI state;
		state.m_nID = nID;
		OnCmdMsg(nID, static_cast<int>(CN_UPDATE_COMMAND_UI), &state, nullptr);
		if (!state.m_bEnabled)
		{
			return TRUE;
		}
		nCode = CN_COMMAND;
	}
	else
	{
		if (AfxGetThreadState().hLockoutNotifyWindow == m_hWnd)
		{
			return TRUE;
		}
		if (ReflectLastMsg(control))
		{
			return TRUE;
		}
		if (nID == 0)
		{
			return FALSE;
		}
	}
	return OnCmdMsg(nID, nCode, nullptr, nullptr);
}

BOOL CWnd::OnNotify(WPARAM, LPARAM lParam, LRESULT* pResult)
{
	NMHDR* header = reinterpret_cast<NMHDR*>(lParam);
	const HWND control = header->hwndFrom;
	const UINT nID = static_cast<UINT>(::GetDlgCtrlID(control));
	if (AfxGetThreadState().hLockoutNotifyWindow == m_hWnd)
	{
		return TRUE;
	}
	if (ReflectLastMsg(control, pResult))
	{
		return TRUE;
	}
	AFX_NOTIFY notify;
	notify.pResult = pResult;
	notify.pNMHDR = header;
	return OnCmdMsg(nID, static_cast<int>(MAKELONG(header->code, WM_NOTIFY)), &notify, nullptr);
}

BOOL PASCAL CWnd::ReflectLastMsg(HWND hWndChild, LRESULT* pResult)
{
	CWnd* child = FromHandlePermanent(hWndChild);
	if (child == nullptr)
	{
		return FALSE;
	}
	const MSG& current = AfxGetThreadState().lastSentMsg;
	return child->OnChildNotify(current.message, current.wParam, current.lParam, pResult);
}

BOOL CWnd::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	return ReflectChildNotify(message, wParam, lParam, pResult);
}

BOOL CWnd::ReflectChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	switch (message)
	{
	case WM_HSCROLL:
	case WM_VSCROLL:
	case WM_PARENTNOTIFY:
		return CWnd::OnWndMsg(WM_REFLECT_BASE + message, wParam, lParam, pResult);
	case WM_COMMAND:
	{
		const BOOL handled = CCmdTarget::OnCmdMsg(0, static_cast<int>(MAKELONG(HIWORD(wParam), WM_REFLECT_BASE + WM_COMMAND)), nullptr, nullptr);
		if (handled && pResult != nullptr)
		{
			*pResult = 1;
		}
		return handled;
	}
	case WM_NOTIFY:
	{
		NMHDR* header = reinterpret_cast<NMHDR*>(lParam);
		AFX_NOTIFY notify;
		notify.pResult = pResult;
		notify.pNMHDR = header;
		return CCmdTarget::OnCmdMsg(0, static_cast<int>(MAKELONG(header->code, WM_REFLECT_BASE + WM_NOTIFY)), &notify, nullptr);
	}
	default:
		return FALSE;
	}
}

BOOL CWnd::PreTranslateMessage(MSG*)
{
	return FALSE;
}

BOOL PASCAL CWnd::WalkPreTranslateTree(HWND hWndStop, MSG* pMsg)
{
	for (HWND hWnd = pMsg->hwnd; hWnd != nullptr; hWnd = ::GetParent(hWnd))
	{
		CWnd* window = FromHandlePermanent(hWnd);
		if (window != nullptr && window->PreTranslateMessage(pMsg))
		{
			return TRUE;
		}
		if (hWnd == hWndStop)
		{
			break;
		}
	}
	return FALSE;
}

BOOL CWnd::PreTranslateInput(MSG* pMsg)
{
	const bool isKeyboard = pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST;
	const bool isMouse = pMsg->message >= WM_MOUSEFIRST && pMsg->message <= WM_MOUSELAST;
	if (!isKeyboard && !isMouse)
	{
		return FALSE;
	}
	return IsDialogMessage(pMsg);
}

BOOL CWnd::IsDialogMessage(LPMSG lpMsg)
{
	return ::IsDialogMessageA(m_hWnd, lpMsg);
}

int CWnd::RunModalLoop(DWORD dwFlags)
{
	CWinThread* thread = AfxGetThread();
	AfxModalLoopState state;
	state.flags = dwFlags;
	state.parent = ::GetParent(m_hWnd);
	state.showsWhenIdle = (dwFlags & MLF_SHOWONIDLE) != 0 && (GetStyle() & WS_VISIBLE) == 0;
	m_nFlags |= WF_MODALLOOP | WF_CONTINUEMODAL;
	MSG* message = &thread->m_msgCur;

	while (ContinueModal())
	{
		while (state.isIdle && !::PeekMessageA(message, nullptr, 0, 0, PM_NOREMOVE))
		{
			runModalIdle(this, state);
		}
		do
		{
			if (!thread->PumpMessage())
			{
				AfxPostQuitMessage(0);
				return -1;
			}
			if (state.showsWhenIdle && message->message == WM_SYSKEYDOWN)
			{
				showModalWindow(this, state);
			}
			if (!ContinueModal())
			{
				break;
			}
			if (thread->IsIdleMessage(message))
			{
				state.isIdle = TRUE;
				state.idleCount = 0;
			}
		} while (::PeekMessageA(message, nullptr, 0, 0, PM_NOREMOVE));
	}

	m_nFlags &= ~(WF_MODALLOOP | WF_CONTINUEMODAL);
	return m_nModalResult;
}

BOOL CWnd::ContinueModal()
{
	return (m_nFlags & WF_CONTINUEMODAL) != 0 ? TRUE : FALSE;
}

void CWnd::EndModalLoop(int nResult)
{
	m_nModalResult = nResult;
	if ((m_nFlags & WF_CONTINUEMODAL) != 0)
	{
		m_nFlags &= ~WF_CONTINUEMODAL;
		PostMessage(WM_NULL);
	}
}

void CWnd::PostNcDestroy()
{
}

void CWnd::DoDataExchange(CDataExchange*)
{
}

void CWnd::OnNcDestroy()
{
	CWinThread* thread = AfxGetThread();
	if (thread != nullptr && thread->m_pMainWnd == this)
	{
		AfxPostQuitMessage(0);
		thread->m_pMainWnd = nullptr;
	}
	if (thread != nullptr && thread->m_pActiveWnd == this)
	{
		thread->m_pActiveWnd = nullptr;
	}

	const LONG_PTR currentProc = ::GetWindowLongPtrA(m_hWnd, GWLP_WNDPROC);
	Default();
	const WNDPROC superProc = *GetSuperWndProcAddr();
	if (::GetWindowLongPtrA(m_hWnd, GWLP_WNDPROC) == currentProc && superProc != nullptr)
	{
		::SetWindowLongPtrA(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(superProc));
	}
	Detach();
	PostNcDestroy();
}

void CWnd::OnHScroll(UINT, UINT, CScrollBar* pScrollBar)
{
	if (pScrollBar != nullptr && ReflectLastMsg(pScrollBar->m_hWnd))
	{
		return;
	}
	Default();
}

void CWnd::OnVScroll(UINT, UINT, CScrollBar* pScrollBar)
{
	if (pScrollBar != nullptr && ReflectLastMsg(pScrollBar->m_hWnd))
	{
		return;
	}
	Default();
}

int CWnd::OnCreate(LPCREATESTRUCT)
{
	return static_cast<int>(Default());
}

void CWnd::OnDestroy() { Default(); }
void CWnd::OnClose() { Default(); }
void CWnd::OnPaint() { Default(); }
BOOL CWnd::OnEraseBkgnd(CDC*) { return static_cast<BOOL>(Default()); }
void CWnd::OnSize(UINT, int, int) { Default(); }
void CWnd::OnMove(int, int) { Default(); }
void CWnd::OnShowWindow(BOOL, UINT) { Default(); }
void CWnd::OnTimer(UINT) { Default(); }
void CWnd::OnSetFocus(CWnd*) { Default(); }
void CWnd::OnKillFocus(CWnd*) { Default(); }
void CWnd::OnActivate(UINT, CWnd*, BOOL) { Default(); }
int CWnd::OnMouseActivate(CWnd*, UINT, UINT) { return static_cast<int>(Default()); }
BOOL CWnd::OnSetCursor(CWnd*, UINT, UINT) { return static_cast<BOOL>(Default()); }
void CWnd::OnKeyDown(UINT, UINT, UINT) { Default(); }
void CWnd::OnKeyUp(UINT, UINT, UINT) { Default(); }
void CWnd::OnChar(UINT, UINT, UINT) { Default(); }
void CWnd::OnSysKeyDown(UINT, UINT, UINT) { Default(); }
void CWnd::OnSysKeyUp(UINT, UINT, UINT) { Default(); }
void CWnd::OnMouseMove(UINT, CPoint) { Default(); }
void CWnd::OnLButtonDown(UINT, CPoint) { Default(); }
void CWnd::OnLButtonUp(UINT, CPoint) { Default(); }
void CWnd::OnLButtonDblClk(UINT, CPoint) { Default(); }
void CWnd::OnRButtonDown(UINT, CPoint) { Default(); }
void CWnd::OnRButtonUp(UINT, CPoint) { Default(); }
void CWnd::OnRButtonDblClk(UINT, CPoint) { Default(); }
void CWnd::OnMButtonDown(UINT, CPoint) { Default(); }
void CWnd::OnMButtonUp(UINT, CPoint) { Default(); }
void CWnd::OnMButtonDblClk(UINT, CPoint) { Default(); }
BOOL CWnd::OnMouseWheel(UINT, short, CPoint) { return static_cast<BOOL>(Default()); }
void CWnd::OnCancelMode() { Default(); }
void CWnd::OnContextMenu(CWnd*, CPoint) { Default(); }
void CWnd::OnInitMenuPopup(CMenu*, UINT, BOOL) { Default(); }
void CWnd::OnMenuSelect(UINT, UINT, HMENU) { Default(); }
void CWnd::OnEnterIdle(UINT, CWnd*) { Default(); }
void CWnd::OnNcCalcSize(BOOL, NCCALCSIZE_PARAMS*) { Default(); }
void CWnd::OnNcPaint() { Default(); }
void CWnd::OnSysCommand(UINT, LPARAM) { Default(); }
void CWnd::OnGetMinMaxInfo(MINMAXINFO*) { Default(); }
void CWnd::OnWindowPosChanging(WINDOWPOS*) { Default(); }
void CWnd::OnWindowPosChanged(WINDOWPOS*) { Default(); }
void CWnd::OnEnable(BOOL) { Default(); }
void CWnd::OnCaptureChanged(CWnd*) { Default(); }
UINT CWnd::OnGetDlgCode() { return static_cast<UINT>(Default()); }
