#include "AfxInternal.h"

IMPLEMENT_DYNAMIC(CCmdTarget, CObject)

namespace
{

const AFX_MSGMAP_ENTRY kCmdTargetEntries[] = {
	{ 0, 0, 0, 0, nullptr },
};

const AFX_MSGMAP kCmdTargetMessageMap = { nullptr, &kCmdTargetEntries[0] };

}

const AFX_MSGMAP_ENTRY* AFXAPI AfxFindMessageEntry(const AFX_MSGMAP_ENTRY* lpEntry, UINT nMsg, UINT nCode, UINT nID)
{
	for (; lpEntry->pfn != nullptr; ++lpEntry)
	{
		if (lpEntry->nMessage == nMsg && lpEntry->nCode == nCode && nID >= lpEntry->nID && nID <= lpEntry->nLastID)
		{
			return lpEntry;
		}
	}
	return nullptr;
}

const AFX_MSGMAP* PASCAL CCmdTarget::GetThisMessageMap()
{
	return &kCmdTargetMessageMap;
}

const AFX_MSGMAP* CCmdTarget::GetMessageMap() const
{
	return GetThisMessageMap();
}

BOOL CCmdTarget::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	UINT nMsg = 0;
	UINT code = static_cast<UINT>(nCode);
	if (code != CN_UPDATE_COMMAND_UI)
	{
		nMsg = HIWORD(code);
		code = LOWORD(code);
	}
	if (nMsg == 0)
	{
		nMsg = WM_COMMAND;
	}

	for (const AFX_MSGMAP* messageMap = GetMessageMap(); messageMap != nullptr;
		messageMap = messageMap->pfnGetBaseMap != nullptr ? messageMap->pfnGetBaseMap() : nullptr)
	{
		const AFX_MSGMAP_ENTRY* entry = AfxFindMessageEntry(messageMap->lpEntries, nMsg, code, nID);
		if (entry == nullptr)
		{
			continue;
		}
		if (pHandlerInfo != nullptr)
		{
			pHandlerInfo->pTarget = this;
			return TRUE;
		}
		AfxMessageCall call;
		call.nMessage = nMsg;
		call.nID = nID;
		call.pExtra = pExtra;
		return entry->pfn(this, call);
	}
	return FALSE;
}

void CCmdTarget::BeginWaitCursor()
{
	AfxGetApp()->DoWaitCursor(1);
}

void CCmdTarget::EndWaitCursor()
{
	AfxGetApp()->DoWaitCursor(-1);
}

void CCmdTarget::RestoreWaitCursor()
{
	AfxGetApp()->DoWaitCursor(0);
}

BOOL AFXAPI AfxFinishUpdateCommandUI(CCmdUI* pCmdUI)
{
	const BOOL handled = !pCmdUI->m_bContinueRouting;
	pCmdUI->m_bContinueRouting = FALSE;
	return handled;
}

void CCmdUI::Enable(BOOL bOn)
{
	if (m_pMenu != nullptr)
	{
		if (m_pSubMenu == nullptr)
		{
			m_pMenu->EnableMenuItem(m_nIndex, MF_BYPOSITION | (bOn ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
		}
		m_bEnableChanged = TRUE;
		return;
	}
	if (!bOn && ::GetFocus() == m_pOther->m_hWnd)
	{
		m_pOther->GetParent()->SendMessage(WM_NEXTDLGCTL, 0, FALSE);
	}
	m_pOther->EnableWindow(bOn);
	m_bEnableChanged = TRUE;
}

void CCmdUI::SetCheck(int nCheck)
{
	if (m_pMenu != nullptr)
	{
		if (m_pSubMenu == nullptr)
		{
			m_pMenu->CheckMenuItem(m_nIndex, MF_BYPOSITION | (nCheck != 0 ? MF_CHECKED : MF_UNCHECKED));
		}
		return;
	}
	if ((m_pOther->SendMessage(WM_GETDLGCODE) & DLGC_BUTTON) != 0)
	{
		m_pOther->SendMessage(BM_SETCHECK, static_cast<WPARAM>(nCheck));
	}
}

void CCmdUI::SetRadio(BOOL bOn)
{
	SetCheck(bOn ? 1 : 0);
}

void CCmdUI::SetText(LPCSTR lpszText)
{
	if (m_pMenu == nullptr)
	{
		CString current;
		m_pOther->GetWindowText(current);
		if (current != lpszText)
		{
			m_pOther->SetWindowText(lpszText);
		}
		return;
	}
	if (m_pSubMenu != nullptr)
	{
		return;
	}
	const UINT state = m_pMenu->GetMenuState(m_nIndex, MF_BYPOSITION);
	m_pMenu->ModifyMenu(m_nIndex, MF_BYPOSITION | MF_STRING | (state & ~(MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR)), m_nID, lpszText);
}

BOOL CCmdUI::DoUpdate(CCmdTarget* pTarget, BOOL bDisableIfNoHndler)
{
	if (m_nID == 0 || LOWORD(m_nID) == 0xFFFF)
	{
		return TRUE;
	}
	m_bEnableChanged = FALSE;
	const BOOL result = pTarget->OnCmdMsg(m_nID, static_cast<int>(CN_UPDATE_COMMAND_UI), this, nullptr);
	if (bDisableIfNoHndler && !m_bEnableChanged)
	{
		AFX_CMDHANDLERINFO info = { nullptr };
		Enable(pTarget->OnCmdMsg(m_nID, CN_COMMAND, this, &info));
	}
	return result;
}

CWaitCursor::CWaitCursor()
{
	AfxGetApp()->BeginWaitCursor();
}

CWaitCursor::~CWaitCursor()
{
	AfxGetApp()->EndWaitCursor();
}

void CWaitCursor::Restore()
{
	AfxGetApp()->RestoreWaitCursor();
}
