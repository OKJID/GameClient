#include "AfxInternal.h"

#include <cstring>

namespace
{

const UINT kFirstSystemCommand = 0xF000;
const UINT kSystemCommandLimit = 0xF1F0;

UINT commandFromSystemCommand(UINT nItemID)
{
	return ((nItemID - kFirstSystemCommand) >> 4) + AFX_IDS_SCFIRST;
}

CMenu* topLevelMenuContaining(CWnd* frame, HMENU hPopup)
{
	CWnd* topLevel = frame->GetTopLevelParent();
	const HMENU hTopMenu = topLevel != nullptr ? ::GetMenu(topLevel->m_hWnd) : nullptr;
	if (hTopMenu == nullptr)
	{
		return nullptr;
	}
	const int count = ::GetMenuItemCount(hTopMenu);
	for (int index = 0; index < count; ++index)
	{
		if (::GetSubMenu(hTopMenu, index) == hPopup)
		{
			return CMenu::FromHandle(hTopMenu);
		}
	}
	return nullptr;
}

}

void CFrameWnd::OnInitMenuPopup(CMenu* pPopupMenu, UINT, BOOL bSysMenu)
{
	AfxCancelModes(m_hWnd);
	if (bSysMenu)
	{
		return;
	}

	CCmdUI state;
	state.m_pMenu = pPopupMenu;
	if (AfxGetThreadState().hTrackingMenu == pPopupMenu->m_hMenu)
	{
		state.m_pParentMenu = pPopupMenu;
	}
	else if (::GetMenu(m_hWnd) != nullptr)
	{
		state.m_pParentMenu = topLevelMenuContaining(this, pPopupMenu->m_hMenu);
	}

	state.m_nIndexMax = pPopupMenu->GetMenuItemCount();
	for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax; ++state.m_nIndex)
	{
		state.m_nID = pPopupMenu->GetMenuItemID(static_cast<int>(state.m_nIndex));
		if (state.m_nID == 0)
		{
			continue;
		}
		updateMenuPopup(state, pPopupMenu);
	}
}

void CFrameWnd::updateMenuPopup(CCmdUI& state, CMenu* pMenu)
{
	if (state.m_nID == static_cast<UINT>(-1))
	{
		state.m_pSubMenu = pMenu->GetSubMenu(static_cast<int>(state.m_nIndex));
		if (state.m_pSubMenu == nullptr)
		{
			return;
		}
		state.m_nID = state.m_pSubMenu->GetMenuItemID(0);
		if (state.m_nID == 0 || state.m_nID == static_cast<UINT>(-1))
		{
			return;
		}
		state.DoUpdate(this, FALSE);
	}
	else
	{
		state.m_pSubMenu = nullptr;
		state.DoUpdate(this, m_bAutoMenuEnable && state.m_nID < kFirstSystemCommand);
	}

	const UINT count = pMenu->GetMenuItemCount();
	if (count < state.m_nIndexMax)
	{
		state.m_nIndex -= state.m_nIndexMax - count;
		while (state.m_nIndex < count && pMenu->GetMenuItemID(static_cast<int>(state.m_nIndex)) == state.m_nID)
		{
			++state.m_nIndex;
		}
	}
	state.m_nIndexMax = count;
}

void CFrameWnd::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU)
{
	CFrameWnd* topLevelFrame = GetTopLevelFrame();
	if (nFlags == 0xFFFF)
	{
		m_nFlags &= ~WF_NOPOPMSG;
		m_nIDTracking = AFX_IDS_IDLEMESSAGE;
		SendMessage(WM_SETMESSAGESTRING, m_nIDTracking);
		CWnd* messageBar = GetMessageBar();
		if (messageBar != nullptr)
		{
			messageBar->UpdateWindow();
		}
	}
	else
	{
		if (nItemID == 0 || (nFlags & (MF_SEPARATOR | MF_POPUP)) != 0)
		{
			m_nIDTracking = 0;
		}
		else if (nItemID >= kFirstSystemCommand && nItemID < kSystemCommandLimit)
		{
			m_nIDTracking = commandFromSystemCommand(nItemID);
		}
		else
		{
			m_nIDTracking = nItemID;
		}
		topLevelFrame->m_nFlags |= WF_NOPOPMSG;
	}

	if (m_nIDTracking != m_nIDLastMessage && GetParent() != nullptr)
	{
		PostMessage(WM_KICKIDLE);
	}
}

void CFrameWnd::OnEnterIdle(UINT nWhy, CWnd* pWho)
{
	CWnd::OnEnterIdle(nWhy, pWho);
	if (nWhy != MSGF_MENU || m_nIDTracking == m_nIDLastMessage)
	{
		return;
	}
	SetMessageText(m_nIDTracking);
}

LRESULT CFrameWnd::OnSetMessageString(WPARAM wParam, LPARAM lParam)
{
	const UINT previousID = m_nIDLastMessage;
	m_nFlags &= ~WF_NOPOPMSG;

	CWnd* messageBar = GetMessageBar();
	if (messageBar != nullptr)
	{
		CString message;
		LPCSTR text = nullptr;
		if (lParam != 0)
		{
			text = reinterpret_cast<LPCSTR>(lParam);
		}
		else if (wParam != 0)
		{
			GetMessageString(static_cast<UINT>(wParam), message);
			text = message;
		}
		messageBar->SetWindowText(text);

		CFrameWnd* barFrame = messageBar->GetParentFrame();
		if (barFrame != nullptr)
		{
			barFrame->m_nIDLastMessage = static_cast<UINT>(wParam);
			barFrame->m_nIDTracking = static_cast<UINT>(wParam);
		}
	}

	m_nIDLastMessage = static_cast<UINT>(wParam);
	m_nIDTracking = static_cast<UINT>(wParam);
	return previousID;
}

LRESULT CFrameWnd::OnPopMessageString(WPARAM wParam, LPARAM lParam)
{
	if ((m_nFlags & WF_NOPOPMSG) != 0)
	{
		return 0;
	}
	return SendMessage(WM_SETMESSAGESTRING, wParam, lParam);
}

void CFrameWnd::GetMessageString(UINT nID, CString& rMessage) const
{
	CString fullString;
	if (!fullString.LoadString(nID))
	{
		rMessage.Empty();
		return;
	}
	AfxExtractSubString(rMessage, fullString, 0);
}

void CFrameWnd::SetMessageText(LPCSTR lpszText)
{
	SendMessage(WM_SETMESSAGESTRING, 0, reinterpret_cast<LPARAM>(lpszText));
}

void CFrameWnd::SetMessageText(UINT nID)
{
	SendMessage(WM_SETMESSAGESTRING, nID);
}

CWnd* CFrameWnd::GetMessageBar()
{
	return GetDescendantWindow(AFX_IDW_STATUS_BAR, TRUE);
}

LRESULT CFrameWnd::OnIdleUpdateCmdUI(WPARAM, LPARAM)
{
	if ((m_nIdleFlags & idleTitle) != 0)
	{
		OnUpdateFrameTitle(TRUE);
	}
	if ((m_nIdleFlags & idleLayout) != 0)
	{
		RecalcLayout((m_nIdleFlags & idleNotify) != 0);
		UpdateWindow();
	}
	if (m_nIDTracking != m_nIDLastMessage)
	{
		SetMessageText(m_nIDTracking);
	}
	m_nIdleFlags = 0;
	return 0;
}

LRESULT CFrameWnd::OnActivateTopLevel(WPARAM, LPARAM)
{
	CWinThread* thread = AfxGetThread();
	if (thread != nullptr && thread->m_pMainWnd == this)
	{
		CView* activeView = GetActiveView();
		if (activeView == nullptr)
		{
			activeView = GetActiveFrame()->GetActiveView();
		}
		if (activeView != nullptr)
		{
			activeView->OnActivateView(FALSE, activeView, activeView);
		}
	}
	PostMessage(WM_KICKIDLE);
	return 0;
}

void CFrameWnd::OnUpdateKeyIndicator(CCmdUI* pCmdUI)
{
	int virtualKey = 0;
	switch (pCmdUI->m_nID)
	{
	case ID_INDICATOR_CAPS:
		virtualKey = VK_CAPITAL;
		break;
	case ID_INDICATOR_NUM:
		virtualKey = VK_NUMLOCK;
		break;
	case ID_INDICATOR_SCRL:
		virtualKey = VK_SCROLL;
		break;
	default:
		pCmdUI->ContinueRouting();
		return;
	}
	pCmdUI->Enable((::GetKeyState(virtualKey) & 0x0001) != 0);
}

BOOL CFrameWnd::OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
	TOOLTIPTEXTA* toolTip = reinterpret_cast<TOOLTIPTEXTA*>(pNMHDR);
	UINT nID = static_cast<UINT>(pNMHDR->idFrom);
	if ((toolTip->uFlags & TTF_IDISHWND) != 0)
	{
		nID = static_cast<UINT>(::GetDlgCtrlID(reinterpret_cast<HWND>(pNMHDR->idFrom)));
	}

	CString tipText;
	if (nID != 0)
	{
		CString fullString;
		if (!fullString.LoadString(nID))
		{
			return FALSE;
		}
		AfxExtractSubString(tipText, fullString, 1);
	}
	strlcpy(toolTip->szText, tipText, sizeof(toolTip->szText));
	*pResult = 0;
	return TRUE;
}
