#include "AfxInternal.h"

namespace
{

UINT repositionMode(UINT nFlags)
{
	return nFlags & ~CWnd::reposNoPosLeftOver;
}

HWND sizeParentChildren(HWND hWndParent, UINT nIDFirst, UINT nIDLast, UINT nIDLeftOver, AFX_SIZEPARENTPARAMS& layout)
{
	HWND hWndLeftOver = nullptr;
	for (HWND hWndChild = ::GetTopWindow(hWndParent); hWndChild != nullptr; hWndChild = ::GetWindow(hWndChild, GW_HWNDNEXT))
	{
		const UINT nIDC = static_cast<UINT>(::GetDlgCtrlID(hWndChild));
		if (nIDC == nIDLeftOver)
		{
			hWndLeftOver = hWndChild;
			continue;
		}
		if (nIDC >= nIDFirst && nIDC <= nIDLast && CWnd::FromHandlePermanent(hWndChild) != nullptr)
		{
			::SendMessageA(hWndChild, WM_SIZEPARENT, 0, reinterpret_cast<LPARAM>(&layout));
		}
	}
	return hWndLeftOver;
}

void returnQueriedRect(const AFX_SIZEPARENTPARAMS& layout, LPRECT lpRectParam, BOOL bStretch)
{
	if (bStretch)
	{
		::CopyRect(lpRectParam, &layout.rect);
		return;
	}
	::SetRect(lpRectParam, 0, 0, layout.sizeTotal.cx, layout.sizeTotal.cy);
}

}

void AFXAPI AfxRepositionWindow(AFX_SIZEPARENTPARAMS* lpLayout, HWND hWnd, LPCRECT lpRect)
{
	if (lpLayout != nullptr && lpLayout->hDWP == nullptr)
	{
		return;
	}
	CRect rectOld;
	::GetWindowRect(hWnd, &rectOld);
	const HWND hWndParent = ::GetParent(hWnd);
	::MapWindowPoints(nullptr, hWndParent, reinterpret_cast<LPPOINT>(&rectOld), 2);
	if (::EqualRect(&rectOld, lpRect))
	{
		return;
	}
	const int width = lpRect->right - lpRect->left;
	const int height = lpRect->bottom - lpRect->top;
	if (lpLayout != nullptr)
	{
		lpLayout->hDWP = ::DeferWindowPos(lpLayout->hDWP, hWnd, nullptr, lpRect->left, lpRect->top, width, height, SWP_NOACTIVATE | SWP_NOZORDER);
		return;
	}
	::SetWindowPos(hWnd, nullptr, lpRect->left, lpRect->top, width, height, SWP_NOACTIVATE | SWP_NOZORDER);
}

void CWnd::RepositionBars(UINT nIDFirst, UINT nIDLast, UINT nIDLeftOver, UINT nFlags, LPRECT lpRectParam, LPCRECT lpRectClient,
	BOOL bStretch)
{
	AFX_SIZEPARENTPARAMS layout = {};
	layout.bStretch = bStretch;
	if (lpRectClient != nullptr)
	{
		layout.rect = *lpRectClient;
	}
	else
	{
		GetClientRect(&layout.rect);
	}
	const UINT mode = repositionMode(nFlags);
	layout.hDWP = mode != reposQuery ? ::BeginDeferWindowPos(8) : nullptr;

	const HWND hWndLeftOver = sizeParentChildren(m_hWnd, nIDFirst, nIDLast, nIDLeftOver, layout);
	if (mode == reposQuery)
	{
		returnQueriedRect(layout, lpRectParam, bStretch);
		return;
	}

	if (nIDLeftOver != 0 && hWndLeftOver != nullptr)
	{
		if (mode == reposExtra)
		{
			layout.rect.left += lpRectParam->left;
			layout.rect.top += lpRectParam->top;
			layout.rect.right -= lpRectParam->right;
			layout.rect.bottom -= lpRectParam->bottom;
		}
		if ((nFlags & reposNoPosLeftOver) == 0)
		{
			CWnd::FromHandle(hWndLeftOver)->CalcWindowRect(&layout.rect);
			AfxRepositionWindow(&layout, hWndLeftOver, &layout.rect);
		}
	}
	if (layout.hDWP != nullptr)
	{
		::EndDeferWindowPos(layout.hDWP);
	}
}
