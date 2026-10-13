#include "AfxInternal.h"

#include "afxext.h"

#include <algorithm>

namespace
{

struct DockSide
{
	UINT nDockBarID;
	DWORD dwAlignment;
};

const DockSide kDockSides[] = {
	{ AFX_IDW_DOCKBAR_TOP, CBRS_ALIGN_TOP },
	{ AFX_IDW_DOCKBAR_BOTTOM, CBRS_ALIGN_BOTTOM },
	{ AFX_IDW_DOCKBAR_LEFT, CBRS_ALIGN_LEFT },
	{ AFX_IDW_DOCKBAR_RIGHT, CBRS_ALIGN_RIGHT },
};

DWORD dockAlignmentFor(UINT nDockBarID, DWORD dwAllowed)
{
	for (const DockSide& side : kDockSides)
	{
		if ((nDockBarID == 0 || nDockBarID == side.nDockBarID) && (dwAllowed & side.dwAlignment) != 0)
		{
			return side.dwAlignment;
		}
	}
	return 0;
}

CString barSection(LPCSTR lpszProfileName, int nIndex)
{
	CString section;
	section.Format("%s-Bar%d", lpszProfileName, nIndex);
	return section;
}

CString summarySection(LPCSTR lpszProfileName)
{
	CString section;
	section.Format("%s-Summary", lpszProfileName);
	return section;
}

void saveBar(CControlBar* pBar, const CString& section)
{
	CWinApp* app = AfxGetApp();
	app->WriteProfileString(section, nullptr, nullptr);
	app->WriteProfileInt(section, "BarID", ::GetDlgCtrlID(pBar->m_hWnd));
	if (!pBar->IsVisible())
	{
		app->WriteProfileInt(section, "Visible", FALSE);
	}
	if (!pBar->IsFloating())
	{
		return;
	}
	CRect floatingRect;
	pBar->m_pFloatingFrame->GetWindowRect(&floatingRect);
	app->WriteProfileInt(section, "Horz", pBar->IsHorizontal());
	app->WriteProfileInt(section, "Floating", TRUE);
	app->WriteProfileInt(section, "XPos", floatingRect.left);
	app->WriteProfileInt(section, "YPos", floatingRect.top);
}

void loadBar(CFrameWnd* pFrame, const CString& section)
{
	CWinApp* app = AfxGetApp();
	CControlBar* bar = pFrame->GetControlBar(app->GetProfileInt(section, "BarID", 0));
	if (bar == nullptr)
	{
		return;
	}
	if (app->GetProfileInt(section, "Floating", FALSE))
	{
		const CPoint position(static_cast<int>(app->GetProfileInt(section, "XPos", 0)), static_cast<int>(app->GetProfileInt(section, "YPos", 0)));
		pFrame->FloatControlBar(bar, position, app->GetProfileInt(section, "Horz", TRUE) ? CBRS_ALIGN_TOP : CBRS_ALIGN_LEFT);
	}
	else if (bar->IsFloating())
	{
		pFrame->DockControlBar(bar);
	}
	pFrame->ShowControlBar(bar, app->GetProfileInt(section, "Visible", TRUE), FALSE);
}

}

void CFrameWnd::EnableDocking(DWORD dwDockStyle)
{
	m_dwDockStyle = dwDockStyle & CBRS_ALIGN_ANY;
	m_pFloatingFrameClass = RUNTIME_CLASS(CMiniDockFrameWnd);
}

void CFrameWnd::DockControlBar(CControlBar* pBar, UINT nDockBarID, LPCRECT)
{
	const DWORD alignment = dockAlignmentFor(nDockBarID, pBar->m_dwDockStyle & m_dwDockStyle);
	if (alignment == 0 || pBar->m_hWnd == nullptr)
	{
		return;
	}
	CWnd* floatingFrame = pBar->m_pFloatingFrame;
	pBar->m_pFloatingFrame = nullptr;
	if (pBar->GetParent() != this)
	{
		pBar->SetParent(this);
	}
	pBar->m_pDockSite = this;
	pBar->SetBarStyle((pBar->GetBarStyle() & ~(CBRS_ALIGN_ANY | CBRS_FLOATING)) | alignment);
	if (floatingFrame != nullptr)
	{
		floatingFrame->DestroyWindow();
	}
	RecalcLayout();
}

void CFrameWnd::FloatControlBar(CControlBar* pBar, CPoint point, DWORD dwStyle)
{
	if (pBar->m_hWnd == nullptr)
	{
		return;
	}
	if (pBar->IsFloating() && (dwStyle & pBar->GetBarStyle() & CBRS_ALIGN_ANY) != 0)
	{
		CFrameWnd* floatingFrame = static_cast<CFrameWnd*>(pBar->m_pFloatingFrame);
		floatingFrame->SetWindowPos(nullptr, point.x, point.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		floatingFrame->RecalcLayout(TRUE);
		floatingFrame->UpdateWindow();
		return;
	}
	if (m_pFloatingFrameClass == nullptr)
	{
		return;
	}

	CMiniDockFrameWnd* floatingFrame = static_cast<CMiniDockFrameWnd*>(m_pFloatingFrameClass->CreateObject());
	if (floatingFrame == nullptr || !floatingFrame->CreateFor(this, pBar, point))
	{
		return;
	}
	pBar->SetBarStyle((pBar->GetBarStyle() & ~CBRS_ALIGN_ANY) | CBRS_FLOATING | (dwStyle & CBRS_ALIGN_ANY));
	floatingFrame->RecalcLayout(TRUE);
	RecalcLayout();
	if ((pBar->GetStyle() & WS_VISIBLE) != 0)
	{
		floatingFrame->ShowWindow(SW_SHOWNA);
		floatingFrame->UpdateWindow();
	}
}

CControlBar* CFrameWnd::GetControlBar(UINT nID)
{
	if (nID == 0)
	{
		return nullptr;
	}
	for (CControlBar* bar : m_listControlBars)
	{
		if (static_cast<UINT>(::GetDlgCtrlID(bar->m_hWnd)) == nID)
		{
			return bar;
		}
	}
	return nullptr;
}

void CFrameWnd::ShowControlBar(CControlBar* pBar, BOOL bShow, BOOL bDelay)
{
	CFrameWnd* parentFrame = pBar->GetDockingFrame();
	if (bDelay)
	{
		pBar->DelayShow(bShow);
		parentFrame->DelayRecalcLayout();
	}
	else
	{
		pBar->SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | (bShow ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
		pBar->DelayShow(bShow);
		if (bShow || !pBar->IsFloating())
		{
			parentFrame->RecalcLayout(FALSE);
		}
	}

	if (!pBar->IsFloating())
	{
		return;
	}
	parentFrame->m_nShowDelay = -1;
	if (!bDelay)
	{
		parentFrame->ShowWindow(bShow ? SW_SHOWNA : SW_HIDE);
		return;
	}
	parentFrame->m_nShowDelay = bShow ? SW_SHOWNA : SW_HIDE;
	if (bShow)
	{
		parentFrame->RecalcLayout(FALSE);
	}
}

void CFrameWnd::AddControlBar(CControlBar* pBar)
{
	m_listControlBars.push_back(pBar);
}

void CFrameWnd::RemoveControlBar(CControlBar* pBar)
{
	m_listControlBars.erase(std::remove(m_listControlBars.begin(), m_listControlBars.end(), pBar), m_listControlBars.end());
}

void CFrameWnd::SaveBarState(LPCSTR lpszProfileName) const
{
	int index = 0;
	for (CControlBar* bar : m_listControlBars)
	{
		saveBar(bar, barSection(lpszProfileName, index++));
	}
	CWinApp* app = AfxGetApp();
	const CString summary = summarySection(lpszProfileName);
	app->WriteProfileInt(summary, "Bars", index);
	app->WriteProfileInt(summary, "ScreenCX", ::GetSystemMetrics(SM_CXSCREEN));
	app->WriteProfileInt(summary, "ScreenCY", ::GetSystemMetrics(SM_CYSCREEN));
}

void CFrameWnd::LoadBarState(LPCSTR lpszProfileName)
{
	const int count = static_cast<int>(AfxGetApp()->GetProfileInt(summarySection(lpszProfileName), "Bars", 0));
	for (int index = 0; index < count; ++index)
	{
		loadBar(this, barSection(lpszProfileName, index));
	}
}

void CFrameWnd::OnUpdateControlBarMenu(CCmdUI* pCmdUI)
{
	CControlBar* bar = GetControlBar(pCmdUI->m_nID);
	if (bar == nullptr)
	{
		pCmdUI->ContinueRouting();
		return;
	}
	pCmdUI->SetCheck((bar->GetStyle() & WS_VISIBLE) != 0 ? 1 : 0);
}

BOOL CFrameWnd::OnBarCheck(UINT nID)
{
	CControlBar* bar = GetControlBar(nID);
	if (bar == nullptr)
	{
		return FALSE;
	}
	ShowControlBar(bar, (bar->GetStyle() & WS_VISIBLE) == 0, FALSE);
	return TRUE;
}
