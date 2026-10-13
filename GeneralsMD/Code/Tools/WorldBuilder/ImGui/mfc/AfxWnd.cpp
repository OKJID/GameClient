#include "AfxInternal.h"

#include <cstdio>
#include <map>
#include <string>

IMPLEMENT_DYNCREATE(CWnd, CCmdTarget)

const CWnd CWnd::wndTop(HWND_TOP);
const CWnd CWnd::wndBottom(HWND_BOTTOM);
const CWnd CWnd::wndTopMost(HWND_TOPMOST);
const CWnd CWnd::wndNoTopMost(HWND_NOTOPMOST);

const char kAfxWndClass[] = "AfxWnd";
const char kAfxFrameOrViewClass[] = "AfxFrameOrView";

namespace
{

bool isStaticPlacementWindow(const CWnd* window)
{
	return window == &CWnd::wndTop || window == &CWnd::wndBottom || window == &CWnd::wndTopMost || window == &CWnd::wndNoTopMost;
}

LRESULT CALLBACK afxCbtFilterHook(int code, WPARAM wParam, LPARAM lParam)
{
	AfxThreadState& state = AfxGetThreadState();
	CWnd* windowInit = state.pWndInit;
	if (code != HCBT_CREATEWND || windowInit == nullptr)
	{
		return CallNextHookEx(state.hHookOldCbtFilter, code, wParam, lParam);
	}

	const HWND hWnd = reinterpret_cast<HWND>(wParam);
	windowInit->Attach(hWnd);
	windowInit->PreSubclassWindow();
	WNDPROC* superProc = windowInit->GetSuperWndProcAddr();
	const WNDPROC oldProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtrA(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(AfxWndProc)));
	if (oldProc != AfxWndProc)
	{
		*superProc = oldProc;
	}
	state.pWndInit = nullptr;
	return CallNextHookEx(state.hHookOldCbtFilter, code, wParam, lParam);
}

void preInitDialog(CWnd* window, CRect& rectOld, DWORD& styleOld)
{
	window->GetWindowRect(&rectOld);
	styleOld = window->GetStyle();
}

void postInitDialog(CWnd* window, const CRect& rectOld, DWORD styleOld)
{
	if ((styleOld & WS_VISIBLE) != 0 || (window->GetStyle() & (WS_VISIBLE | WS_CHILD)) != 0)
	{
		return;
	}
	CRect rect;
	window->GetWindowRect(&rect);
	if (rect.left != rectOld.left || rect.top != rectOld.top)
	{
		return;
	}
	CWnd* owner = window->GetWindow(GW_OWNER);
	if (owner != nullptr && owner->IsWindowEnabled())
	{
		return;
	}
	if (window->CheckAutoCenter())
	{
		window->CenterWindow();
	}
}

BOOL registerClassOnce(LPCSTR className, UINT style, HBRUSH background)
{
	WNDCLASSA windowClass = {};
	if (::GetClassInfoA(AfxGetInstanceHandle(), className, &windowClass))
	{
		return TRUE;
	}
	windowClass.style = style;
	windowClass.lpfnWndProc = ::DefWindowProcA;
	windowClass.hInstance = AfxGetInstanceHandle();
	windowClass.hCursor = ::LoadCursorA(nullptr, IDC_ARROW);
	windowClass.hbrBackground = background;
	windowClass.lpszClassName = className;
	return ::RegisterClassA(&windowClass) != 0 ? TRUE : FALSE;
}

}

BOOL AFXAPI AfxDeferRegisterClass(LONG fToRegister)
{
	BOOL result = TRUE;
	if ((fToRegister & AFX_WND_REG) != 0)
	{
		result = registerClassOnce(kAfxWndClass, CS_DBLCLKS, nullptr) && result;
	}
	if ((fToRegister & AFX_WNDFRAMEORVIEW_REG) != 0)
	{
		const HBRUSH background = reinterpret_cast<HBRUSH>(static_cast<intptr_t>(COLOR_WINDOW + 1));
		result = registerClassOnce(kAfxFrameOrViewClass, CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW, background) && result;
	}
	return result;
}

CWnd::CWnd() :
	m_hWnd(nullptr),
	m_pfnSuper(nullptr),
	m_nFlags(0),
	m_nModalResult(0),
	m_hWndOwner(nullptr)
{
}

CWnd::CWnd(HWND hWnd) :
	m_hWnd(hWnd),
	m_pfnSuper(nullptr),
	m_nFlags(0),
	m_nModalResult(0),
	m_hWndOwner(nullptr)
{
}

CWnd::~CWnd()
{
	if (m_hWnd != nullptr && !isStaticPlacementWindow(this))
	{
		DestroyWindow();
	}
}

HWND CWnd::GetSafeHwnd() const
{
	const CWnd* self = this;
	return self != nullptr ? m_hWnd : nullptr;
}

CWnd* PASCAL CWnd::FromHandle(HWND hWnd)
{
	return AfxWindowMap().fromHandle(hWnd);
}

CWnd* PASCAL CWnd::FromHandlePermanent(HWND hWnd)
{
	return hWnd != nullptr ? AfxWindowMap().lookupPermanent(hWnd) : nullptr;
}

void PASCAL CWnd::DeleteTempMap()
{
	AfxWindowMap().deleteTemporaries();
}

BOOL CWnd::Attach(HWND hWndNew)
{
	if (hWndNew == nullptr)
	{
		return FALSE;
	}
	m_hWnd = hWndNew;
	AfxWindowMap().setPermanent(hWndNew, this);
	return TRUE;
}

HWND CWnd::Detach()
{
	const HWND hWnd = m_hWnd;
	if (hWnd != nullptr)
	{
		AfxWindowMap().removeHandle(hWnd);
		m_hWnd = nullptr;
	}
	return hWnd;
}

BOOL CWnd::SubclassWindow(HWND hWnd)
{
	if (!Attach(hWnd))
	{
		return FALSE;
	}
	PreSubclassWindow();
	WNDPROC* superProc = GetSuperWndProcAddr();
	const WNDPROC oldProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtrA(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(AfxWndProc)));
	if (*superProc == nullptr && oldProc != AfxWndProc)
	{
		*superProc = oldProc;
	}
	return TRUE;
}

BOOL CWnd::SubclassDlgItem(UINT nID, CWnd* pParent)
{
	const HWND control = ::GetDlgItem(pParent->m_hWnd, static_cast<int>(nID));
	return control != nullptr ? SubclassWindow(control) : FALSE;
}

HWND CWnd::UnsubclassWindow()
{
	WNDPROC* superProc = GetSuperWndProcAddr();
	::SetWindowLongPtrA(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(*superProc));
	*superProc = nullptr;
	return Detach();
}

WNDPROC* CWnd::GetSuperWndProcAddr()
{
	return &m_pfnSuper;
}

void CWnd::PreSubclassWindow()
{
}

BOOL CWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	if (cs.lpszClass == nullptr)
	{
		AfxDeferRegisterClass(AFX_WND_REG);
		cs.lpszClass = kAfxWndClass;
	}
	return TRUE;
}

BOOL CWnd::Create(LPCSTR lpszClassName, LPCSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
	return CreateEx(0, lpszClassName, lpszWindowName, dwStyle | WS_CHILD, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		pParentWnd->GetSafeHwnd(), reinterpret_cast<HMENU>(static_cast<UINT_PTR>(nID)), pContext);
}

BOOL CWnd::CreateEx(DWORD dwExStyle, LPCSTR lpszClassName, LPCSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd,
	UINT nID, LPVOID lpParam)
{
	return CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		pParentWnd->GetSafeHwnd(), reinterpret_cast<HMENU>(static_cast<UINT_PTR>(nID)), lpParam);
}

BOOL CWnd::CreateEx(DWORD dwExStyle, LPCSTR lpszClassName, LPCSTR lpszWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight,
	HWND hWndParent, HMENU nIDorHMenu, LPVOID lpParam)
{
	CREATESTRUCT cs = {};
	cs.dwExStyle = dwExStyle;
	cs.lpszClass = lpszClassName;
	cs.lpszName = lpszWindowName;
	cs.style = static_cast<LONG>(dwStyle);
	cs.x = x;
	cs.y = y;
	cs.cx = nWidth;
	cs.cy = nHeight;
	cs.hwndParent = hWndParent;
	cs.hMenu = nIDorHMenu;
	cs.hInstance = AfxGetInstanceHandle();
	cs.lpCreateParams = lpParam;
	if (!PreCreateWindow(cs))
	{
		PostNcDestroy();
		return FALSE;
	}

	AfxHookWindowCreate(this);
	const HWND hWnd = ::CreateWindowExA(cs.dwExStyle, cs.lpszClass, cs.lpszName, static_cast<DWORD>(cs.style), cs.x, cs.y, cs.cx, cs.cy,
		cs.hwndParent, cs.hMenu, cs.hInstance, cs.lpCreateParams);
	if (!AfxUnhookWindowCreate())
	{
		PostNcDestroy();
	}
	return hWnd != nullptr ? TRUE : FALSE;
}

BOOL CWnd::DestroyWindow()
{
	if (m_hWnd == nullptr)
	{
		return FALSE;
	}
	const HWND original = m_hWnd;
	const bool isPermanent = FromHandlePermanent(original) == this;
	const BOOL result = ::DestroyWindow(original);
	if (!isPermanent && m_hWnd == original)
	{
		Detach();
	}
	return result;
}

void CWnd::CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType)
{
	const DWORD exStyle = nAdjustType == adjustBorder ? GetExStyle() & ~WS_EX_CLIENTEDGE : GetExStyle();
	::AdjustWindowRectEx(lpClientRect, GetStyle(), FALSE, exStyle);
}

BOOL CWnd::IsFrameWnd() const
{
	return FALSE;
}

BOOL CWnd::CheckAutoCenter()
{
	return TRUE;
}

void AFXAPI AfxHookWindowCreate(CWnd* pWnd)
{
	AfxThreadState& state = AfxGetThreadState();
	if (state.pWndInit == pWnd)
	{
		return;
	}
	if (state.hHookOldCbtFilter == nullptr)
	{
		state.hHookOldCbtFilter = ::SetWindowsHookExA(WH_CBT, afxCbtFilterHook, nullptr, 0);
	}
	state.pWndInit = pWnd;
}

BOOL AFXAPI AfxUnhookWindowCreate()
{
	AfxThreadState& state = AfxGetThreadState();
	if (state.hHookOldCbtFilter != nullptr)
	{
		::UnhookWindowsHookEx(state.hHookOldCbtFilter);
		state.hHookOldCbtFilter = nullptr;
	}
	if (state.pWndInit != nullptr)
	{
		state.pWndInit = nullptr;
		return FALSE;
	}
	return TRUE;
}

WNDPROC AFXAPI AfxGetAfxWndProc()
{
	return AfxWndProc;
}

LRESULT CALLBACK AfxWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	if (nMsg == WM_QUERYAFXWNDPROC)
	{
		return 1;
	}
	CWnd* window = CWnd::FromHandlePermanent(hWnd);
	if (window == nullptr)
	{
		return ::DefWindowProcA(hWnd, nMsg, wParam, lParam);
	}
	return AfxCallWndProc(window, hWnd, nMsg, wParam, lParam);
}

LRESULT AFXAPI AfxCallWndProc(CWnd* pWnd, HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	AfxThreadState& state = AfxGetThreadState();
	const MSG previous = state.lastSentMsg;
	state.lastSentMsg.hwnd = hWnd;
	state.lastSentMsg.message = nMsg;
	state.lastSentMsg.wParam = wParam;
	state.lastSentMsg.lParam = lParam;

	CRect rectOld;
	DWORD styleOld = 0;
	if (nMsg == WM_INITDIALOG)
	{
		preInitDialog(pWnd, rectOld, styleOld);
	}
	const LRESULT result = pWnd->WindowProc(nMsg, wParam, lParam);
	if (nMsg == WM_INITDIALOG && ::IsWindow(hWnd))
	{
		postInitDialog(pWnd, rectOld, styleOld);
	}
	state.lastSentMsg = previous;
	return result;
}

const MSG* AFXAPI AfxGetCurrentMessage()
{
	return &AfxGetThreadState().lastSentMsg;
}

LPCSTR AFXAPI AfxRegisterWndClass(UINT nClassStyle, HCURSOR hCursor, HBRUSH hbrBackground, HICON hIcon)
{
	static std::map<std::string, std::string> registeredNames;
	char name[96];
	snprintf(name, sizeof(name), "Afx:%x:%p:%p:%p", nClassStyle, static_cast<void*>(hCursor), static_cast<void*>(hbrBackground),
		static_cast<void*>(hIcon));
	std::string& stored = registeredNames[name];
	if (!stored.empty())
	{
		return stored.c_str();
	}
	stored = name;

	WNDCLASSA windowClass = {};
	windowClass.style = nClassStyle;
	windowClass.lpfnWndProc = ::DefWindowProcA;
	windowClass.hInstance = AfxGetInstanceHandle();
	windowClass.hIcon = hIcon;
	windowClass.hCursor = hCursor;
	windowClass.hbrBackground = hbrBackground;
	windowClass.lpszClassName = stored.c_str();
	::RegisterClassA(&windowClass);
	return stored.c_str();
}
