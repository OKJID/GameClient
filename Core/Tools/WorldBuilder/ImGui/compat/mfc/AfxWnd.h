#pragma once

#include "AfxCmdTarget.h"
#include "AfxGeometry.h"

class CCreateContext;
class CDataExchange;
class CDC;
class CFont;
class CFrameWnd;
class CMenu;
class CRgn;
class CScrollBar;

#define WF_TOOLTIPS 0x0001
#define WF_TEMPHIDE 0x0002
#define WF_STAYDISABLED 0x0004
#define WF_MODALLOOP 0x0008
#define WF_CONTINUEMODAL 0x0010
#define WF_STAYACTIVE 0x0020
#define WF_NOPOPMSG 0x0040
#define WF_MODALDISABLE 0x0080

#define MLF_NOIDLEMSG 0x0001
#define MLF_NOKICKIDLE 0x0002
#define MLF_SHOWONIDLE 0x0004

#define WM_QUERYAFXWNDPROC 0x0360
#define WM_SIZEPARENT 0x0361
#define WM_SETMESSAGESTRING 0x0362
#define WM_IDLEUPDATECMDUI 0x0363
#define WM_INITIALUPDATE 0x0364
#define WM_COMMANDHELP 0x0365
#define WM_HELPHITTEST 0x0366
#define WM_EXITHELPMODE 0x0367
#define WM_RECALCPARENT 0x0368
#define WM_SIZECHILD 0x0369
#define WM_KICKIDLE 0x036A
#define WM_QUERYCENTERWND 0x036B
#define WM_DISABLEMODAL 0x036C
#define WM_FLOATSTATUS 0x036D
#define WM_ACTIVATETOPLEVEL 0x036E
#define WM_POPMESSAGESTRING 0x0375

#define FS_SHOW 0x01
#define FS_HIDE 0x02
#define FS_ACTIVATE 0x04
#define FS_DEACTIVATE 0x08
#define FS_ENABLE 0x10
#define FS_DISABLE 0x20
#define FS_SYNCACTIVE 0x40

typedef void (CALLBACK* AFX_TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);

class CWnd : public CCmdTarget
{
	DECLARE_DYNCREATE(CWnd)

public:
	enum AdjustType
	{
		adjustBorder = 0,
		adjustOutside = 1,
	};

	enum RepositionFlags
	{
		reposDefault = 0,
		reposQuery = 1,
		reposExtra = 2,
		reposNoPosLeftOver = 0x8000,
	};

	CWnd();
	~CWnd() override;

	static const CWnd wndTop;
	static const CWnd wndBottom;
	static const CWnd wndTopMost;
	static const CWnd wndNoTopMost;

	HWND m_hWnd;

	operator HWND() const { return GetSafeHwnd(); }
	BOOL operator==(const CWnd& wnd) const { return wnd.m_hWnd == m_hWnd; }
	BOOL operator!=(const CWnd& wnd) const { return wnd.m_hWnd != m_hWnd; }
	HWND GetSafeHwnd() const;

	static CWnd* PASCAL FromHandle(HWND hWnd);
	static CWnd* PASCAL FromHandlePermanent(HWND hWnd);
	static void PASCAL DeleteTempMap();
	BOOL Attach(HWND hWndNew);
	HWND Detach();
	BOOL SubclassWindow(HWND hWnd);
	BOOL SubclassDlgItem(UINT nID, CWnd* pParent);
	HWND UnsubclassWindow();

	virtual BOOL Create(LPCSTR lpszClassName, LPCSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID,
		CCreateContext* pContext = nullptr);
	virtual BOOL CreateEx(DWORD dwExStyle, LPCSTR lpszClassName, LPCSTR lpszWindowName, DWORD dwStyle, int x, int y, int nWidth,
		int nHeight, HWND hWndParent, HMENU nIDorHMenu, LPVOID lpParam = nullptr);
	virtual BOOL CreateEx(DWORD dwExStyle, LPCSTR lpszClassName, LPCSTR lpszWindowName, DWORD dwStyle, const RECT& rect,
		CWnd* pParentWnd, UINT nID, LPVOID lpParam = nullptr);
	virtual BOOL DestroyWindow();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PreSubclassWindow();
	virtual void CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType = adjustBorder);

	DWORD GetStyle() const;
	DWORD GetExStyle() const;
	BOOL ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);
	BOOL ModifyStyleEx(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);
	static BOOL PASCAL ModifyStyle(HWND hWnd, DWORD dwRemove, DWORD dwAdd, UINT nFlags);
	static BOOL PASCAL ModifyStyleEx(HWND hWnd, DWORD dwRemove, DWORD dwAdd, UINT nFlags);

	int GetDlgCtrlID() const;
	int SetDlgCtrlID(int nID);
	CWnd* GetParent() const;
	CWnd* SetParent(CWnd* pWndNewParent);
	CWnd* GetOwner() const;
	void SetOwner(CWnd* pOwnerWnd);
	CWnd* GetTopLevelParent() const;
	CWnd* GetTopLevelOwner() const;
	CFrameWnd* GetParentFrame() const;
	CFrameWnd* GetTopLevelFrame() const;
	CWnd* GetDlgItem(int nID) const;
	void GetDlgItem(int nID, HWND* phWnd) const;
	CWnd* GetDescendantWindow(int nID, BOOL bOnlyPerm = FALSE) const;
	CWnd* GetWindow(UINT nCmd) const;
	CWnd* GetNextWindow(UINT nFlag = GW_HWNDNEXT) const;
	CWnd* GetTopWindow() const;
	CWnd* GetLastActivePopup() const;
	CWnd* ChildWindowFromPoint(POINT point) const;
	BOOL IsChild(const CWnd* pWnd) const;

	static CWnd* PASCAL GetFocus();
	CWnd* SetFocus();
	static CWnd* PASCAL GetActiveWindow();
	CWnd* SetActiveWindow();
	static CWnd* PASCAL GetCapture();
	CWnd* SetCapture();
	static CWnd* PASCAL GetForegroundWindow();
	BOOL SetForegroundWindow();
	static CWnd* PASCAL GetDesktopWindow();
	static CWnd* PASCAL GetSafeOwner(CWnd* pParent = nullptr, HWND* pWndTop = nullptr);
	static HWND PASCAL GetSafeOwner_(HWND hParent, HWND* pWndTop);
	static CWnd* PASCAL WindowFromPoint(POINT point);

	void SetWindowText(LPCSTR lpszString);
	int GetWindowText(LPSTR lpszStringBuf, int nMaxCount) const;
	void GetWindowText(CString& rString) const;
	int GetWindowTextLength() const;
	void SetFont(CFont* pFont, BOOL bRedraw = TRUE);
	CFont* GetFont() const;
	CMenu* GetMenu() const;
	BOOL SetMenu(CMenu* pMenu);
	void DrawMenuBar();

	BOOL IsIconic() const;
	BOOL IsZoomed() const;
	BOOL IsWindowVisible() const;
	BOOL IsWindowEnabled() const;
	BOOL EnableWindow(BOOL bEnable = TRUE);
	BOOL ShowWindow(int nCmdShow);
	void MoveWindow(int x, int y, int nWidth, int nHeight, BOOL bRepaint = TRUE);
	void MoveWindow(LPCRECT lpRect, BOOL bRepaint = TRUE);
	BOOL SetWindowPos(const CWnd* pWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags);
	void BringWindowToTop();
	void GetWindowRect(LPRECT lpRect) const;
	void GetClientRect(LPRECT lpRect) const;
	void ClientToScreen(LPPOINT lpPoint) const;
	void ClientToScreen(LPRECT lpRect) const;
	void ScreenToClient(LPPOINT lpPoint) const;
	void ScreenToClient(LPRECT lpRect) const;
	void MapWindowPoints(CWnd* pwndTo, LPPOINT lpPoint, UINT nCount) const;
	void MapWindowPoints(CWnd* pwndTo, LPRECT lpRect) const;
	void CenterWindow(CWnd* pAlternateOwner = nullptr);
	void RepositionBars(UINT nIDFirst, UINT nIDLast, UINT nIDLeftOver, UINT nFlags = reposDefault, LPRECT lpRectParam = nullptr,
		LPCRECT lpRectClient = nullptr, BOOL bStretch = TRUE);

	CDC* GetDC();
	CDC* GetWindowDC();
	int ReleaseDC(CDC* pDC);
	void UpdateWindow();
	void SetRedraw(BOOL bRedraw = TRUE);
	BOOL GetUpdateRect(LPRECT lpRect, BOOL bErase = FALSE);
	int GetUpdateRgn(CRgn* pRgn, BOOL bErase = FALSE);
	void Invalidate(BOOL bErase = TRUE);
	void InvalidateRect(LPCRECT lpRect, BOOL bErase = TRUE);
	void InvalidateRgn(CRgn* pRgn, BOOL bErase = TRUE);
	void ValidateRect(LPCRECT lpRect);
	BOOL RedrawWindow(LPCRECT lpRectUpdate = nullptr, CRgn* prgnUpdate = nullptr, UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);

	int GetScrollPos(int nBar) const;
	int SetScrollPos(int nBar, int nPos, BOOL bRedraw = TRUE);
	void GetScrollRange(int nBar, LPINT lpMinPos, LPINT lpMaxPos) const;
	void SetScrollRange(int nBar, int nMinPos, int nMaxPos, BOOL bRedraw = TRUE);
	BOOL GetScrollInfo(int nBar, LPSCROLLINFO lpScrollInfo, UINT nMask = SIF_ALL);
	BOOL SetScrollInfo(int nBar, LPSCROLLINFO lpScrollInfo, BOOL bRedraw = TRUE);
	void ScrollWindow(int xAmount, int yAmount, LPCRECT lpRect = nullptr, LPCRECT lpClipRect = nullptr);

	LRESULT SendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) const;
	BOOL PostMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) const;
	void SendMessageToDescendants(UINT message, WPARAM wParam = 0, LPARAM lParam = 0, BOOL bDeep = TRUE, BOOL bOnlyPerm = FALSE);
	static void PASCAL SendMessageToDescendants(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL bDeep, BOOL bOnlyPerm);
	UINT_PTR SetTimer(UINT_PTR nIDEvent, UINT nElapse, AFX_TIMERPROC lpfnTimer);
	BOOL KillTimer(UINT_PTR nIDEvent);

	void CheckDlgButton(int nIDButton, UINT nCheck);
	void CheckRadioButton(int nIDFirstButton, int nIDLastButton, int nIDCheckButton);
	int GetCheckedRadioButton(int nIDFirstButton, int nIDLastButton);
	UINT IsDlgButtonChecked(int nIDButton) const;
	UINT GetDlgItemInt(int nID, BOOL* lpTrans = nullptr, BOOL bSigned = TRUE) const;
	void SetDlgItemInt(int nID, UINT nValue, BOOL bSigned = TRUE);
	int GetDlgItemText(int nID, LPSTR lpStr, int nMaxCount) const;
	int GetDlgItemText(int nID, CString& rString) const;
	void SetDlgItemText(int nID, LPCSTR lpszString);
	LRESULT SendDlgItemMessage(int nID, UINT message, WPARAM wParam = 0, LPARAM lParam = 0) const;
	CWnd* GetNextDlgGroupItem(CWnd* pWndCtl, BOOL bPrevious = FALSE) const;
	CWnd* GetNextDlgTabItem(CWnd* pWndCtl, BOOL bPrevious = FALSE) const;
	BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
	void UpdateDialogControls(CCmdTarget* pTarget, BOOL bDisableIfNoHndler);
	BOOL ExecuteDlgInit(LPCSTR lpszResourceName);
	BOOL CreateDlg(LPCSTR lpszTemplateName, CWnd* pParentWnd);

	int MessageBox(LPCSTR lpszText, LPCSTR lpszCaption = nullptr, UINT nType = MB_OK);

	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PostNcDestroy();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL ContinueModal();
	virtual void EndModalLoop(int nResult);
	virtual WNDPROC* GetSuperWndProcAddr();
	virtual BOOL IsFrameWnd() const;
	virtual BOOL CheckAutoCenter();

	int RunModalLoop(DWORD dwFlags = 0);
	BOOL ReflectChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	static BOOL PASCAL ReflectLastMsg(HWND hWndChild, LRESULT* pResult = nullptr);
	static BOOL PASCAL WalkPreTranslateTree(HWND hWndStop, MSG* pMsg);
	BOOL PreTranslateInput(MSG* pMsg);
	BOOL IsDialogMessage(LPMSG lpMsg);
	LRESULT Default();

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnNcDestroy();
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg void OnNcPaint();
	afx_msg void OnClose();
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnCancelMode();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint pos);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu);
	afx_msg void OnEnterIdle(UINT nWhy, CWnd* pWho);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnEnable(BOOL bEnable);
	afx_msg void OnCaptureChanged(CWnd* pWnd);
	afx_msg UINT OnGetDlgCode();

	WNDPROC m_pfnSuper;
	UINT m_nFlags;
	int m_nModalResult;

protected:
	DECLARE_MESSAGE_MAP()

	BOOL dispatchMessageMap(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& lResult);

	HWND m_hWndOwner;

private:
	explicit CWnd(HWND hWnd);

	friend struct AfxWindowCreation;
};

LRESULT CALLBACK AfxWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);
LRESULT AFXAPI AfxCallWndProc(CWnd* pWnd, HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);
WNDPROC AFXAPI AfxGetAfxWndProc();
void AFXAPI AfxHookWindowCreate(CWnd* pWnd);
BOOL AFXAPI AfxUnhookWindowCreate();
LPCSTR AFXAPI AfxRegisterWndClass(UINT nClassStyle, HCURSOR hCursor = nullptr, HBRUSH hbrBackground = nullptr, HICON hIcon = nullptr);
const MSG* AFXAPI AfxGetCurrentMessage();
