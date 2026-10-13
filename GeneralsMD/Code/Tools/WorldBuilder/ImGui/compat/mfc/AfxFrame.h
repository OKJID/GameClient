#pragma once

#include "AfxDialog.h"
#include "AfxMenu.h"

#include <vector>

class CControlBar;
class CDocTemplate;
class CDocument;
class CView;

#define AFX_IDW_PANE_FIRST 0xE900
#define AFX_IDW_PANE_LAST 0xE9FF
#define AFX_IDW_HSCROLL_FIRST 0xEA00
#define AFX_IDW_SIZE_BOX 0xEA20
#define AFX_IDW_CONTROLBAR_FIRST 0xE800
#define AFX_IDW_CONTROLBAR_LAST 0xE8FF
#define AFX_IDW_TOOLBAR 0xE800
#define AFX_IDW_STATUS_BAR 0xE801
#define AFX_IDW_PREVIEW_BAR 0xE802
#define AFX_IDW_RESIZE_BAR 0xE803
#define AFX_IDW_DIALOGBAR 0xE805
#define AFX_IDW_DOCKBAR_TOP 0xE81B
#define AFX_IDW_DOCKBAR_LEFT 0xE81C
#define AFX_IDW_DOCKBAR_RIGHT 0xE81D
#define AFX_IDW_DOCKBAR_BOTTOM 0xE81E
#define AFX_IDW_DOCKBAR_FLOAT 0xE81F

#define AFX_WS_DEFAULT_VIEW (WS_CHILD | WS_VISIBLE | WS_BORDER)

#define FWS_ADDTOTITLE 0x00008000L
#define FWS_PREFIXTITLE 0x00004000L
#define FWS_SNAPTOBARS 0x00002000L

#define CBRS_ALIGN_LEFT 0x1000L
#define CBRS_ALIGN_TOP 0x2000L
#define CBRS_ALIGN_RIGHT 0x4000L
#define CBRS_ALIGN_BOTTOM 0x8000L
#define CBRS_ALIGN_ANY 0xF000L
#define CBRS_BORDER_LEFT 0x0100L
#define CBRS_BORDER_TOP 0x0200L
#define CBRS_BORDER_RIGHT 0x0400L
#define CBRS_BORDER_BOTTOM 0x0800L
#define CBRS_BORDER_ANY 0x0F00L
#define CBRS_TOOLTIPS 0x0010L
#define CBRS_FLYBY 0x0020L
#define CBRS_FLOAT_MULTI 0x0040L
#define CBRS_BORDER_3D 0x0080L
#define CBRS_HIDE_INPLACE 0x0008L
#define CBRS_SIZE_DYNAMIC 0x0004L
#define CBRS_SIZE_FIXED 0x0002L
#define CBRS_FLOATING 0x0001L
#define CBRS_GRIPPER 0x00400000L
#define CBRS_ORIENT_HORZ (CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM)
#define CBRS_ORIENT_VERT (CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT)
#define CBRS_ORIENT_ANY (CBRS_ORIENT_HORZ | CBRS_ORIENT_VERT)
#define CBRS_ALL 0x0040FFFFL
#define CBRS_NOALIGN 0x00000000L
#define CBRS_LEFT (CBRS_ALIGN_LEFT | CBRS_BORDER_RIGHT)
#define CBRS_TOP (CBRS_ALIGN_TOP | CBRS_BORDER_BOTTOM)
#define CBRS_RIGHT (CBRS_ALIGN_RIGHT | CBRS_BORDER_LEFT)
#define CBRS_BOTTOM (CBRS_ALIGN_BOTTOM | CBRS_BORDER_TOP)

class CCreateContext
{
public:
	CRuntimeClass* m_pNewViewClass = nullptr;
	CDocument* m_pCurrentDoc = nullptr;
	CDocTemplate* m_pNewDocTemplate = nullptr;
	CView* m_pLastView = nullptr;
	CFrameWnd* m_pCurrentFrame = nullptr;
};

struct AFX_SIZEPARENTPARAMS
{
	HDWP hDWP;
	RECT rect;
	SIZE sizeTotal;
	BOOL bStretch;
};

void AFXAPI AfxRepositionWindow(AFX_SIZEPARENTPARAMS* lpLayout, HWND hWnd, LPCRECT lpRect);
void AFXAPI AfxCancelModes(HWND hWndRcvr);

class CFrameWnd : public CWnd
{
	DECLARE_DYNCREATE(CFrameWnd)

public:
	static const CRect rectDefault;

	enum IdleFlags
	{
		idleMenu = 1,
		idleTitle = 2,
		idleNotify = 4,
		idleLayout = 8,
	};

	CFrameWnd();
	~CFrameWnd() override;

	BOOL Create(LPCSTR lpszClassName, LPCSTR lpszWindowName, DWORD dwStyle = WS_OVERLAPPEDWINDOW, const RECT& rect = rectDefault,
		CWnd* pParentWnd = nullptr, LPCSTR lpszMenuName = nullptr, DWORD dwExStyle = 0, CCreateContext* pContext = nullptr);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = nullptr,
		CCreateContext* pContext = nullptr);

	CView* GetActiveView() const { return m_pViewActive; }
	void SetActiveView(CView* pViewNew, BOOL bNotify = TRUE);
	virtual CDocument* GetActiveDocument();
	virtual CFrameWnd* GetActiveFrame();

	void SetTitle(LPCSTR lpszTitle) { m_strTitle = lpszTitle; }
	CString GetTitle() const { return m_strTitle; }
	virtual void OnUpdateFrameTitle(BOOL bAddToTitle);
	void UpdateFrameTitleForDocument(LPCSTR lpszDocName);
	void DelayUpdateFrameTitle() { m_nIdleFlags |= idleTitle; }
	void DelayRecalcLayout(BOOL bNotify = TRUE) { m_nIdleFlags |= idleLayout | (bNotify ? idleNotify : 0); }

	virtual void GetMessageString(UINT nID, CString& rMessage) const;
	void SetMessageText(LPCSTR lpszText);
	void SetMessageText(UINT nID);
	virtual CWnd* GetMessageBar();

	void EnableDocking(DWORD dwDockStyle);
	void DockControlBar(CControlBar* pBar, UINT nDockBarID = 0, LPCRECT lpRect = nullptr);
	void FloatControlBar(CControlBar* pBar, CPoint point, DWORD dwStyle = CBRS_ALIGN_TOP);
	CControlBar* GetControlBar(UINT nID);
	void ShowControlBar(CControlBar* pBar, BOOL bShow, BOOL bDelay);
	void AddControlBar(CControlBar* pBar);
	void RemoveControlBar(CControlBar* pBar);
	void SaveBarState(LPCSTR lpszProfileName) const;
	void LoadBarState(LPCSTR lpszProfileName);
	virtual void RecalcLayout(BOOL bNotify = TRUE);
	virtual void ActivateFrame(int nCmdShow = -1);
	void InitialUpdateFrame(CDocument* pDoc, BOOL bMakeVisible);
	void BringToTop(int nCmdShow);
	void BeginModalState();
	void EndModalState();
	BOOL InModalState() const { return m_cModalStack != 0; }
	void NotifyFloatingWindows(DWORD dwFlags);

	virtual CWnd* CreateView(CCreateContext* pContext, UINT nID = AFX_IDW_PANE_FIRST);
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual HACCEL GetDefaultAccelerator();

	BOOL IsFrameWnd() const override;
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;
	BOOL PreTranslateMessage(MSG* pMsg) override;
	BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) override;
	void PostNcDestroy() override;

	BOOL m_bAutoMenuEnable;
	UINT m_nIDHelp;
	HACCEL m_hAccelTable;
	HMENU m_hMenuDefault;
	CString m_strTitle;
	int m_nWindow;
	int m_nIdleFlags;
	int m_nShowDelay;
	CRect m_rectBorder;
	CRuntimeClass* m_pFloatingFrameClass;
	std::vector<CControlBar*> m_listControlBars;

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnEnable(BOOL bEnable);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	afx_msg void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu);
	afx_msg void OnEnterIdle(UINT nWhy, CWnd* pWho);
	afx_msg LRESULT OnSetMessageString(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPopMessageString(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnActivateTopLevel(WPARAM wParam, LPARAM lParam);
	afx_msg void OnUpdateControlBarMenu(CCmdUI* pCmdUI);
	afx_msg BOOL OnBarCheck(UINT nID);
	afx_msg void OnUpdateKeyIndicator(CCmdUI* pCmdUI);
	afx_msg BOOL OnToolTipText(UINT nID, NMHDR* pNMHDR, LRESULT* pResult);

protected:
	DECLARE_MESSAGE_MAP()

	int OnCreateHelper(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	LPCSTR GetIconWndClass(DWORD dwDefaultStyle, UINT nIDResource);
	void recalcHostLayout();
	void updateMenuPopup(CCmdUI& state, CMenu* pMenu);
	BOOL closeApplicationDocuments(CDocument* pDocument);
	BOOL closeLastDocumentFrame(CDocument* pDocument);
	void disableModelessWindows(HWND hWndTopLevel);

	CView* m_pViewActive;
	UINT m_nIDTracking;
	UINT m_nIDLastMessage;
	int m_cModalStack;
	DWORD m_dwDockStyle;
	BOOL m_bInRecalcLayout;
	std::vector<HWND> m_disabledWindows;
};
