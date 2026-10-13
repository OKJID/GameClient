#include "AfxInternal.h"

BOOL CWnd::CreateDlg(LPCSTR lpszTemplateName, CWnd* pParentWnd)
{
	AfxHookWindowCreate(this);
	const HWND hWnd = ::CreateDialogParamA(AfxGetResourceHandle(), lpszTemplateName, pParentWnd->GetSafeHwnd(), AfxDlgProc, 0);
	if (!AfxUnhookWindowCreate())
	{
		PostNcDestroy();
	}
	return hWnd != nullptr ? TRUE : FALSE;
}

#include "Win32Resources.h"

#include <algorithm>

namespace
{

const UINT kWin16ListBoxAddString = 0x0401;
const UINT kWin16ComboBoxAddString = 0x0403;

HWND centerTargetOf(HWND hWnd, CWnd* pAlternateOwner)
{
	if (pAlternateOwner != nullptr)
	{
		return pAlternateOwner->m_hWnd;
	}
	const bool isChild = (::GetWindowLongA(hWnd, GWL_STYLE) & WS_CHILD) != 0;
	HWND center = isChild ? ::GetParent(hWnd) : ::GetWindow(hWnd, GW_OWNER);
	if (center == nullptr)
	{
		return nullptr;
	}
	const HWND alternate = reinterpret_cast<HWND>(::SendMessageA(center, WM_QUERYCENTERWND, 0, 0));
	return alternate != nullptr ? alternate : center;
}

bool isUsableCenter(HWND hWnd)
{
	const DWORD style = static_cast<DWORD>(::GetWindowLongA(hWnd, GWL_STYLE));
	return (style & WS_VISIBLE) != 0 && (style & WS_MINIMIZE) == 0;
}

UINT win32MessageFor(UINT dialogInitMessage)
{
	if (dialogInitMessage == kWin16ListBoxAddString)
	{
		return LB_ADDSTRING;
	}
	if (dialogInitMessage == kWin16ComboBoxAddString)
	{
		return CB_ADDSTRING;
	}
	return dialogInitMessage;
}

bool isStaticallyEnabledButton(CWnd& control)
{
	if ((control.SendMessage(WM_GETDLGCODE) & DLGC_BUTTON) == 0)
	{
		return true;
	}
	const UINT style = control.GetStyle() & 0x0F;
	return style == BS_AUTOCHECKBOX || style == BS_AUTO3STATE || style == BS_GROUPBOX || style == BS_AUTORADIOBUTTON;
}

}

void CWnd::CenterWindow(CWnd* pAlternateOwner)
{
	const bool isChild = (GetStyle() & WS_CHILD) != 0;
	HWND center = centerTargetOf(m_hWnd, pAlternateOwner);
	CRect dialogRect;
	GetWindowRect(&dialogRect);
	CRect area;
	CRect centerRect;
	if (!isChild)
	{
		center = center != nullptr && isUsableCenter(center) ? center : nullptr;
		::GetWindowRect(::GetDesktopWindow(), &area);
		centerRect = area;
		if (center != nullptr)
		{
			::GetWindowRect(center, &centerRect);
		}
	}
	else
	{
		const HWND parent = ::GetParent(m_hWnd);
		::GetClientRect(parent, &area);
		::GetClientRect(center, &centerRect);
		::MapWindowPoints(center, parent, reinterpret_cast<LPPOINT>(&centerRect), 2);
	}

	int left = (centerRect.left + centerRect.right) / 2 - dialogRect.Width() / 2;
	int top = (centerRect.top + centerRect.bottom) / 2 - dialogRect.Height() / 2;
	left = std::max<int>(area.left, std::min<int>(left, area.right - dialogRect.Width()));
	top = std::max<int>(area.top, std::min<int>(top, area.bottom - dialogRect.Height()));
	SetWindowPos(nullptr, left, top, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

BOOL CWnd::UpdateData(BOOL bSaveAndValidate)
{
	CDataExchange exchange(this, bSaveAndValidate);
	AfxThreadState& state = AfxGetThreadState();
	const HWND previousLockout = state.hLockoutNotifyWindow;
	state.hLockoutNotifyWindow = m_hWnd;
	BOOL isValid = FALSE;
	try
	{
		DoDataExchange(&exchange);
		isValid = TRUE;
	}
	catch (CUserException* exception)
	{
		exception->Delete();
	}
	state.hLockoutNotifyWindow = previousLockout;
	return isValid;
}

void CWnd::UpdateDialogControls(CCmdTarget* pTarget, BOOL bDisableIfNoHndler)
{
	CCmdUI state;
	CWnd control;
	for (HWND child = ::GetTopWindow(m_hWnd); child != nullptr; child = ::GetWindow(child, GW_HWNDNEXT))
	{
		control.m_hWnd = child;
		state.m_nID = static_cast<UINT>(::GetDlgCtrlID(child));
		state.m_pOther = &control;

		CWnd* permanent = FromHandlePermanent(child);
		if (permanent != nullptr && permanent->CCmdTarget::OnCmdMsg(0, static_cast<int>(MAKELONG(0xFFFF, WM_COMMAND + WM_REFLECT_BASE)), &state, nullptr))
		{
			continue;
		}
		if (CCmdTarget::OnCmdMsg(state.m_nID, static_cast<int>(CN_UPDATE_COMMAND_UI), &state, nullptr))
		{
			continue;
		}
		state.DoUpdate(pTarget, bDisableIfNoHndler && !isStaticallyEnabledButton(control));
	}
	control.m_hWnd = nullptr;
}

BOOL CWnd::ExecuteDlgInit(LPCSTR lpszResourceName)
{
	UINT id = 0;
	if (lpszResourceName == nullptr || !Win32Resources::instance().resolveName(lpszResourceName, id))
	{
		return TRUE;
	}
	const std::vector<Win32DialogInitEntry>* entries = Win32Resources::instance().dialogInit(id);
	if (entries == nullptr)
	{
		return TRUE;
	}
	for (const Win32DialogInitEntry& entry : *entries)
	{
		const UINT message = win32MessageFor(entry.message);
		if (message != LB_ADDSTRING && message != CB_ADDSTRING)
		{
			continue;
		}
		if (::SendDlgItemMessageA(m_hWnd, static_cast<int>(entry.controlId), message, 0, reinterpret_cast<LPARAM>(entry.text.c_str())) == -1)
		{
			return FALSE;
		}
	}
	return TRUE;
}
