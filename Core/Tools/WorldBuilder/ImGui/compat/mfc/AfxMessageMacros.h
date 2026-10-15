#pragma once

#define AFX_MSG_POINT(lParam) CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))
#define AFX_MSG_HWND(param) CWnd::FromHandle(reinterpret_cast<HWND>(param))

#define ON_COMMAND(id, memberFxn) \
	AFX_MSGMAP_ENTRY_OF(WM_COMMAND, CN_COMMAND, id, id, pThis->memberFxn(); return TRUE;)
#define ON_COMMAND_EX(id, memberFxn) \
	AFX_MSGMAP_ENTRY_OF(WM_COMMAND, CN_COMMAND, id, id, return pThis->memberFxn(call.nID);)
#define ON_COMMAND_RANGE(id, idLast, memberFxn) \
	AFX_MSGMAP_ENTRY_OF(WM_COMMAND, CN_COMMAND, id, idLast, pThis->memberFxn(call.nID); return TRUE;)
#define ON_COMMAND_EX_RANGE(id, idLast, memberFxn) \
	AFX_MSGMAP_ENTRY_OF(WM_COMMAND, CN_COMMAND, id, idLast, return pThis->memberFxn(call.nID);)
#define ON_UPDATE_COMMAND_UI(id, memberFxn) \
	AFX_MSGMAP_ENTRY_OF(WM_COMMAND, CN_UPDATE_COMMAND_UI, id, id, \
		CCmdUI* pCmdUI = static_cast<CCmdUI*>(call.pExtra); pThis->memberFxn(pCmdUI); return AfxFinishUpdateCommandUI(pCmdUI);)
#define ON_UPDATE_COMMAND_UI_RANGE(id, idLast, memberFxn) \
	AFX_MSGMAP_ENTRY_OF(WM_COMMAND, CN_UPDATE_COMMAND_UI, id, idLast, \
		CCmdUI* pCmdUI = static_cast<CCmdUI*>(call.pExtra); pThis->memberFxn(pCmdUI); return AfxFinishUpdateCommandUI(pCmdUI);)

#define ON_NOTIFY(wNotifyCode, id, memberFxn) \
	AFX_MSGMAP_ENTRY_OF(WM_NOTIFY, static_cast<WORD>(static_cast<int>(wNotifyCode)), id, id, \
		AFX_NOTIFY* pNotify = static_cast<AFX_NOTIFY*>(call.pExtra); pThis->memberFxn(pNotify->pNMHDR, pNotify->pResult); return TRUE;)
#define ON_NOTIFY_EX(wNotifyCode, id, memberFxn) \
	AFX_MSGMAP_ENTRY_OF(WM_NOTIFY, static_cast<WORD>(static_cast<int>(wNotifyCode)), id, id, \
		AFX_NOTIFY* pNotify = static_cast<AFX_NOTIFY*>(call.pExtra); return pThis->memberFxn(call.nID, pNotify->pNMHDR, pNotify->pResult);)
#define ON_NOTIFY_RANGE(wNotifyCode, id, idLast, memberFxn) \
	AFX_MSGMAP_ENTRY_OF(WM_NOTIFY, static_cast<WORD>(static_cast<int>(wNotifyCode)), id, idLast, \
		AFX_NOTIFY* pNotify = static_cast<AFX_NOTIFY*>(call.pExtra); pThis->memberFxn(call.nID, pNotify->pNMHDR, pNotify->pResult); return TRUE;)
#define ON_NOTIFY_EX_RANGE(wNotifyCode, id, idLast, memberFxn) \
	AFX_MSGMAP_ENTRY_OF(WM_NOTIFY, static_cast<WORD>(static_cast<int>(wNotifyCode)), id, idLast, \
		AFX_NOTIFY* pNotify = static_cast<AFX_NOTIFY*>(call.pExtra); return pThis->memberFxn(call.nID, pNotify->pNMHDR, pNotify->pResult);)

#define ON_CONTROL(wNotifyCode, id, memberFxn) \
	AFX_MSGMAP_ENTRY_OF(WM_COMMAND, wNotifyCode, id, id, pThis->memberFxn(); return TRUE;)
#define ON_CONTROL_RANGE(wNotifyCode, id, idLast, memberFxn) \
	AFX_MSGMAP_ENTRY_OF(WM_COMMAND, wNotifyCode, id, idLast, pThis->memberFxn(call.nID); return TRUE;)

#define ON_BN_CLICKED(id, memberFxn) ON_CONTROL(BN_CLICKED, id, memberFxn)
#define ON_BN_DOUBLECLICKED(id, memberFxn) ON_CONTROL(BN_DOUBLECLICKED, id, memberFxn)
#define ON_BN_SETFOCUS(id, memberFxn) ON_CONTROL(BN_SETFOCUS, id, memberFxn)
#define ON_BN_KILLFOCUS(id, memberFxn) ON_CONTROL(BN_KILLFOCUS, id, memberFxn)
#define ON_EN_SETFOCUS(id, memberFxn) ON_CONTROL(EN_SETFOCUS, id, memberFxn)
#define ON_EN_KILLFOCUS(id, memberFxn) ON_CONTROL(EN_KILLFOCUS, id, memberFxn)
#define ON_EN_CHANGE(id, memberFxn) ON_CONTROL(EN_CHANGE, id, memberFxn)
#define ON_EN_UPDATE(id, memberFxn) ON_CONTROL(EN_UPDATE, id, memberFxn)
#define ON_EN_ERRSPACE(id, memberFxn) ON_CONTROL(EN_ERRSPACE, id, memberFxn)
#define ON_EN_MAXTEXT(id, memberFxn) ON_CONTROL(EN_MAXTEXT, id, memberFxn)
#define ON_EN_HSCROLL(id, memberFxn) ON_CONTROL(EN_HSCROLL, id, memberFxn)
#define ON_EN_VSCROLL(id, memberFxn) ON_CONTROL(EN_VSCROLL, id, memberFxn)
#define ON_CBN_ERRSPACE(id, memberFxn) ON_CONTROL(CBN_ERRSPACE, id, memberFxn)
#define ON_CBN_SELCHANGE(id, memberFxn) ON_CONTROL(CBN_SELCHANGE, id, memberFxn)
#define ON_CBN_DBLCLK(id, memberFxn) ON_CONTROL(CBN_DBLCLK, id, memberFxn)
#define ON_CBN_SETFOCUS(id, memberFxn) ON_CONTROL(CBN_SETFOCUS, id, memberFxn)
#define ON_CBN_KILLFOCUS(id, memberFxn) ON_CONTROL(CBN_KILLFOCUS, id, memberFxn)
#define ON_CBN_EDITCHANGE(id, memberFxn) ON_CONTROL(CBN_EDITCHANGE, id, memberFxn)
#define ON_CBN_EDITUPDATE(id, memberFxn) ON_CONTROL(CBN_EDITUPDATE, id, memberFxn)
#define ON_CBN_DROPDOWN(id, memberFxn) ON_CONTROL(CBN_DROPDOWN, id, memberFxn)
#define ON_CBN_CLOSEUP(id, memberFxn) ON_CONTROL(CBN_CLOSEUP, id, memberFxn)
#define ON_CBN_SELENDOK(id, memberFxn) ON_CONTROL(CBN_SELENDOK, id, memberFxn)
#define ON_CBN_SELENDCANCEL(id, memberFxn) ON_CONTROL(CBN_SELENDCANCEL, id, memberFxn)
#define ON_LBN_ERRSPACE(id, memberFxn) ON_CONTROL(LBN_ERRSPACE, id, memberFxn)
#define ON_LBN_SELCHANGE(id, memberFxn) ON_CONTROL(LBN_SELCHANGE, id, memberFxn)
#define ON_LBN_DBLCLK(id, memberFxn) ON_CONTROL(LBN_DBLCLK, id, memberFxn)
#define ON_LBN_SELCANCEL(id, memberFxn) ON_CONTROL(LBN_SELCANCEL, id, memberFxn)
#define ON_LBN_SETFOCUS(id, memberFxn) ON_CONTROL(LBN_SETFOCUS, id, memberFxn)
#define ON_LBN_KILLFOCUS(id, memberFxn) ON_CONTROL(LBN_KILLFOCUS, id, memberFxn)
#define ON_STN_CLICKED(id, memberFxn) ON_CONTROL(STN_CLICKED, id, memberFxn)
#define ON_STN_DBLCLK(id, memberFxn) ON_CONTROL(STN_DBLCLK, id, memberFxn)

#define ON_MESSAGE(message, memberFxn) \
	AFX_MSGMAP_ENTRY_OF(message, 0, 0, 0, call.lResult = pThis->memberFxn(call.wParam, call.lParam); return TRUE;)

#define AFX_WM_ENTRY(message, body) AFX_MSGMAP_ENTRY_OF(message, 0, 0, 0, body)

#define ON_WM_CREATE() \
	AFX_WM_ENTRY(WM_CREATE, call.lResult = pThis->OnCreate(reinterpret_cast<LPCREATESTRUCT>(call.lParam)); return TRUE;)
#define ON_WM_DESTROY() AFX_WM_ENTRY(WM_DESTROY, pThis->OnDestroy(); return TRUE;)
#define ON_WM_NCDESTROY() AFX_WM_ENTRY(WM_NCDESTROY, pThis->OnNcDestroy(); return TRUE;)
#define ON_WM_CLOSE() AFX_WM_ENTRY(WM_CLOSE, pThis->OnClose(); return TRUE;)
#define ON_WM_PAINT() AFX_WM_ENTRY(WM_PAINT, pThis->OnPaint(); return TRUE;)
#define ON_WM_ERASEBKGND() \
	AFX_WM_ENTRY(WM_ERASEBKGND, call.lResult = pThis->OnEraseBkgnd(CDC::FromHandle(reinterpret_cast<HDC>(call.wParam))); return TRUE;)
#define ON_WM_SIZE() \
	AFX_WM_ENTRY(WM_SIZE, pThis->OnSize(static_cast<UINT>(call.wParam), static_cast<short>(LOWORD(call.lParam)), static_cast<short>(HIWORD(call.lParam))); return TRUE;)
#define ON_WM_MOVE() \
	AFX_WM_ENTRY(WM_MOVE, pThis->OnMove(static_cast<short>(LOWORD(call.lParam)), static_cast<short>(HIWORD(call.lParam))); return TRUE;)
#define ON_WM_SHOWWINDOW() \
	AFX_WM_ENTRY(WM_SHOWWINDOW, pThis->OnShowWindow(static_cast<BOOL>(call.wParam), static_cast<UINT>(call.lParam)); return TRUE;)
#define ON_WM_TIMER() AFX_WM_ENTRY(WM_TIMER, pThis->OnTimer(static_cast<UINT>(call.wParam)); return TRUE;)
#define ON_WM_SETFOCUS() AFX_WM_ENTRY(WM_SETFOCUS, pThis->OnSetFocus(AFX_MSG_HWND(call.wParam)); return TRUE;)
#define ON_WM_KILLFOCUS() AFX_WM_ENTRY(WM_KILLFOCUS, pThis->OnKillFocus(AFX_MSG_HWND(call.wParam)); return TRUE;)
#define ON_WM_ACTIVATE() \
	AFX_WM_ENTRY(WM_ACTIVATE, pThis->OnActivate(LOWORD(call.wParam), AFX_MSG_HWND(call.lParam), static_cast<BOOL>(HIWORD(call.wParam))); return TRUE;)
#define ON_WM_MOUSEACTIVATE() \
	AFX_WM_ENTRY(WM_MOUSEACTIVATE, call.lResult = pThis->OnMouseActivate(AFX_MSG_HWND(call.wParam), LOWORD(call.lParam), HIWORD(call.lParam)); return TRUE;)
#define ON_WM_SETCURSOR() \
	AFX_WM_ENTRY(WM_SETCURSOR, call.lResult = pThis->OnSetCursor(AFX_MSG_HWND(call.wParam), LOWORD(call.lParam), HIWORD(call.lParam)); return TRUE;)
#define AFX_KEY_ENTRY(message, memberFxn) \
	AFX_WM_ENTRY(message, pThis->memberFxn(static_cast<UINT>(call.wParam), LOWORD(call.lParam), HIWORD(call.lParam)); return TRUE;)
#define ON_WM_KEYDOWN() AFX_KEY_ENTRY(WM_KEYDOWN, OnKeyDown)
#define ON_WM_KEYUP() AFX_KEY_ENTRY(WM_KEYUP, OnKeyUp)
#define ON_WM_CHAR() AFX_KEY_ENTRY(WM_CHAR, OnChar)
#define ON_WM_SYSKEYDOWN() AFX_KEY_ENTRY(WM_SYSKEYDOWN, OnSysKeyDown)
#define ON_WM_SYSKEYUP() AFX_KEY_ENTRY(WM_SYSKEYUP, OnSysKeyUp)
#define AFX_MOUSE_ENTRY(message, memberFxn) \
	AFX_WM_ENTRY(message, pThis->memberFxn(static_cast<UINT>(call.wParam), AFX_MSG_POINT(call.lParam)); return TRUE;)
#define ON_WM_MOUSEMOVE() AFX_MOUSE_ENTRY(WM_MOUSEMOVE, OnMouseMove)
#define ON_WM_LBUTTONDOWN() AFX_MOUSE_ENTRY(WM_LBUTTONDOWN, OnLButtonDown)
#define ON_WM_LBUTTONUP() AFX_MOUSE_ENTRY(WM_LBUTTONUP, OnLButtonUp)
#define ON_WM_LBUTTONDBLCLK() AFX_MOUSE_ENTRY(WM_LBUTTONDBLCLK, OnLButtonDblClk)
#define ON_WM_RBUTTONDOWN() AFX_MOUSE_ENTRY(WM_RBUTTONDOWN, OnRButtonDown)
#define ON_WM_RBUTTONUP() AFX_MOUSE_ENTRY(WM_RBUTTONUP, OnRButtonUp)
#define ON_WM_RBUTTONDBLCLK() AFX_MOUSE_ENTRY(WM_RBUTTONDBLCLK, OnRButtonDblClk)
#define ON_WM_MBUTTONDOWN() AFX_MOUSE_ENTRY(WM_MBUTTONDOWN, OnMButtonDown)
#define ON_WM_MBUTTONUP() AFX_MOUSE_ENTRY(WM_MBUTTONUP, OnMButtonUp)
#define ON_WM_MBUTTONDBLCLK() AFX_MOUSE_ENTRY(WM_MBUTTONDBLCLK, OnMButtonDblClk)
#define ON_WM_MOUSEWHEEL() \
	AFX_WM_ENTRY(WM_MOUSEWHEEL, call.lResult = pThis->OnMouseWheel(LOWORD(call.wParam), static_cast<short>(HIWORD(call.wParam)), AFX_MSG_POINT(call.lParam)); return TRUE;)
#define ON_WM_HSCROLL() \
	AFX_WM_ENTRY(WM_HSCROLL, pThis->OnHScroll(LOWORD(call.wParam), static_cast<short>(HIWORD(call.wParam)), static_cast<CScrollBar*>(AFX_MSG_HWND(call.lParam))); return TRUE;)
#define ON_WM_VSCROLL() \
	AFX_WM_ENTRY(WM_VSCROLL, pThis->OnVScroll(LOWORD(call.wParam), static_cast<short>(HIWORD(call.wParam)), static_cast<CScrollBar*>(AFX_MSG_HWND(call.lParam))); return TRUE;)
#define ON_WM_CANCELMODE() AFX_WM_ENTRY(WM_CANCELMODE, pThis->OnCancelMode(); return TRUE;)
#define ON_WM_CONTEXTMENU() AFX_WM_ENTRY(WM_CONTEXTMENU, pThis->OnContextMenu(AFX_MSG_HWND(call.wParam), AFX_MSG_POINT(call.lParam)); return TRUE;)
#define ON_WM_INITMENUPOPUP() \
	AFX_WM_ENTRY(WM_INITMENUPOPUP, pThis->OnInitMenuPopup(CMenu::FromHandle(reinterpret_cast<HMENU>(call.wParam)), LOWORD(call.lParam), static_cast<BOOL>(HIWORD(call.lParam))); return TRUE;)
#define ON_WM_MENUSELECT() \
	AFX_WM_ENTRY(WM_MENUSELECT, pThis->OnMenuSelect(LOWORD(call.wParam), HIWORD(call.wParam), reinterpret_cast<HMENU>(call.lParam)); return TRUE;)
#define ON_WM_NCCALCSIZE() \
	AFX_WM_ENTRY(WM_NCCALCSIZE, pThis->OnNcCalcSize(static_cast<BOOL>(call.wParam), reinterpret_cast<NCCALCSIZE_PARAMS*>(call.lParam)); return TRUE;)
#define ON_WM_NCPAINT() AFX_WM_ENTRY(WM_NCPAINT, pThis->OnNcPaint(); return TRUE;)
#define ON_WM_ENTERIDLE()AFX_WM_ENTRY(WM_ENTERIDLE, pThis->OnEnterIdle(static_cast<UINT>(call.wParam), AFX_MSG_HWND(call.lParam)); return TRUE;)
#define ON_WM_SYSCOMMAND()AFX_WM_ENTRY(WM_SYSCOMMAND, pThis->OnSysCommand(static_cast<UINT>(call.wParam), call.lParam); return TRUE;)
#define ON_WM_GETMINMAXINFO() AFX_WM_ENTRY(WM_GETMINMAXINFO, pThis->OnGetMinMaxInfo(reinterpret_cast<MINMAXINFO*>(call.lParam)); return TRUE;)
#define ON_WM_WINDOWPOSCHANGING() \
	AFX_WM_ENTRY(WM_WINDOWPOSCHANGING, pThis->OnWindowPosChanging(reinterpret_cast<WINDOWPOS*>(call.lParam)); return TRUE;)
#define ON_WM_WINDOWPOSCHANGED() \
	AFX_WM_ENTRY(WM_WINDOWPOSCHANGED, pThis->OnWindowPosChanged(reinterpret_cast<WINDOWPOS*>(call.lParam)); return TRUE;)
#define ON_WM_ENABLE() AFX_WM_ENTRY(WM_ENABLE, pThis->OnEnable(static_cast<BOOL>(call.wParam)); return TRUE;)
#define ON_WM_CAPTURECHANGED() AFX_WM_ENTRY(WM_CAPTURECHANGED, pThis->OnCaptureChanged(AFX_MSG_HWND(call.lParam)); return TRUE;)
#define ON_WM_GETDLGCODE() AFX_WM_ENTRY(WM_GETDLGCODE, call.lResult = static_cast<LRESULT>(pThis->OnGetDlgCode()); return TRUE;)
