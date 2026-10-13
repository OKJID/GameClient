#pragma once

#include "AfxApp.h"

#define TBBS_BUTTON MAKELONG(TBSTYLE_BUTTON, 0)
#define TBBS_SEPARATOR MAKELONG(TBSTYLE_SEP, 0)
#define TBBS_CHECKBOX MAKELONG(TBSTYLE_CHECK, 0)
#define TBBS_GROUP MAKELONG(TBSTYLE_GROUP, 0)
#define TBBS_CHECKGROUP (TBBS_GROUP | TBBS_CHECKBOX)
#define TBBS_DROPDOWN MAKELONG(TBSTYLE_DROPDOWN, 0)
#define TBBS_AUTOSIZE MAKELONG(TBSTYLE_AUTOSIZE, 0)
#define TBBS_CHECKED MAKELONG(0, TBSTATE_CHECKED)
#define TBBS_PRESSED MAKELONG(0, TBSTATE_PRESSED)
#define TBBS_DISABLED MAKELONG(0, TBSTATE_ENABLED)
#define TBBS_INDETERMINATE MAKELONG(0, TBSTATE_INDETERMINATE)
#define TBBS_HIDDEN MAKELONG(0, TBSTATE_HIDDEN)
#define TBBS_WRAPPED MAKELONG(0, TBSTATE_WRAP)

#define SBPS_NORMAL 0x0000
#define SBPS_NOBORDERS SBT_NOBORDERS
#define SBPS_POPOUT SBT_POPOUT
#define SBPS_OWNERDRAW SBT_OWNERDRAW
#define SBPS_DISABLED 0x04000000
#define SBPS_STRETCH 0x08000000

#define LM_STRETCH 0x01
#define LM_HORZ 0x02
#define LM_MRUWIDTH 0x04
#define LM_HORZDOCK 0x08
#define LM_VERTDOCK 0x10
#define LM_LENGTHY 0x20
#define LM_COMMIT 0x40

class CControlBar : public CWnd
{
	DECLARE_DYNAMIC(CControlBar)

protected:
	CControlBar();

public:
	~CControlBar() override;

	enum StateFlags
	{
		delayHide = 1,
		delayShow = 2,
		tempHide = 4,
		statusSet = 8,
	};

	int GetCount() const { return m_nCount; }
	DWORD GetBarStyle() { return m_dwStyle; }
	void SetBarStyle(DWORD dwStyle);
	void EnableDocking(DWORD dwDockStyle);
	CFrameWnd* GetDockingFrame() const;
	BOOL IsFloating() const;
	BOOL IsHorizontal() const;
	void DelayShow(BOOL bShow);
	virtual BOOL IsVisible() const;
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
	virtual CSize CalcDynamicLayout(int nLength, DWORD nMode);
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler) = 0;
	virtual void OnBarStyleChange(DWORD dwOldStyle, DWORD dwNewStyle);
	virtual void CalcInsideRect(CRect& rect, BOOL bHorz) const;
	virtual void DoPaint(CDC* pDC);
	virtual int OnToolHitTest(CPoint point) const;
	void DrawBorders(CDC* pDC, CRect& rect);
	void DrawGripper(CDC* pDC, const CRect& rect);
	BOOL SetStatusText(int nHit);
	void ResetTimer(UINT nEvent, UINT nTime);

	BOOL PreCreateWindow(CREATESTRUCT& cs) override;
	BOOL PreTranslateMessage(MSG* pMsg) override;
	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	void PostNcDestroy() override;

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg LRESULT OnSizeParent(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam);

	BOOL m_bAutoDelete;
	DWORD m_dwStyle;
	DWORD m_dwDockStyle;
	CFrameWnd* m_pDockSite;
	CWnd* m_pFloatingFrame;
	int m_cxLeftBorder;
	int m_cxRightBorder;
	int m_cyTopBorder;
	int m_cyBottomBorder;
	int m_cxDefaultGap;
	UINT m_nStateFlags;
	int m_nCount;

protected:
	DECLARE_MESSAGE_MAP()

	DWORD RecalcDelayShow(AFX_SIZEPARENTPARAMS* lpLayout);
	void EraseNonClient();
	void drawBorderHighlights(CDC* pDC, const CRect& rect, const CRect& outer) const;
	void updateFlyByStatus(const MSG* pMsg);
	int statusHitAtCursor() const;
};

class CToolBar : public CControlBar
{
	DECLARE_DYNAMIC(CToolBar)

public:
	CToolBar();
	~CToolBar() override;

	BOOL Create(CWnd* pParentWnd, DWORD dwStyle = WS_CHILD | WS_VISIBLE | CBRS_TOP, UINT nID = AFX_IDW_TOOLBAR);
	BOOL CreateEx(CWnd* pParentWnd, DWORD dwCtrlStyle = TBSTYLE_FLAT, DWORD dwStyle = WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP,
		CRect rcBorders = CRect(0, 0, 0, 0), UINT nID = AFX_IDW_TOOLBAR);
	void SetSizes(SIZE sizeButton, SIZE sizeImage);
	BOOL LoadToolBar(LPCSTR lpszResourceName);
	BOOL LoadToolBar(UINT nIDResource);
	BOOL LoadBitmap(LPCSTR lpszResourceName);
	BOOL LoadBitmap(UINT nIDResource);
	BOOL SetBitmap(HBITMAP hbmImageWell);
	BOOL SetButtons(const UINT* lpIDArray, int nIDCount);

	int CommandToIndex(UINT nIDFind) const;
	UINT GetItemID(int nIndex) const;
	void GetItemRect(int nIndex, LPRECT lpRect) const;
	UINT GetButtonStyle(int nIndex) const;
	void SetButtonStyle(int nIndex, UINT nStyle);
	void GetButtonInfo(int nIndex, UINT& nID, UINT& nStyle, int& iImage) const;
	void SetButtonInfo(int nIndex, UINT nID, UINT nStyle, int iImage);

	CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz) override;
	CSize CalcDynamicLayout(int nLength, DWORD nMode) override;
	void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler) override;
	void OnBarStyleChange(DWORD dwOldStyle, DWORD dwNewStyle) override;
	int OnToolHitTest(CPoint point) const override;

	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg void OnNcPaint();
	afx_msg void OnPaint();

protected:
	DECLARE_MESSAGE_MAP()

	BOOL AddReplaceBitmap(HBITMAP hbmImageWell);
	void _GetButton(int nIndex, TBBUTTON* pButton) const;
	void _SetButton(int nIndex, TBBUTTON* pButton);
	CSize CalcSize(const TBBUTTON* pData, int nCount) const;
	CSize CalcLayout(DWORD dwMode);
	LRESULT toolbarMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) const;

	HBITMAP m_hbmImageWell;
	CSize m_sizeButton;
	CSize m_sizeImage;
};

class CStatusBar : public CControlBar
{
	DECLARE_DYNAMIC(CStatusBar)

public:
	CStatusBar();
	~CStatusBar() override;

	BOOL Create(CWnd* pParentWnd, DWORD dwStyle = WS_CHILD | WS_VISIBLE | CBRS_BOTTOM, UINT nID = AFX_IDW_STATUS_BAR);
	BOOL CreateEx(CWnd* pParentWnd, DWORD dwCtrlStyle = 0, DWORD dwStyle = WS_CHILD | WS_VISIBLE | CBRS_BOTTOM, UINT nID = AFX_IDW_STATUS_BAR);
	BOOL SetIndicators(const UINT* lpIDArray, int nIDCount);

	int CommandToIndex(UINT nIDFind) const;
	UINT GetItemID(int nIndex) const;
	void GetItemRect(int nIndex, LPRECT lpRect) const;
	void GetPaneText(int nIndex, CString& rString) const;
	CString GetPaneText(int nIndex) const;
	BOOL SetPaneText(int nIndex, LPCSTR lpszNewText, BOOL bUpdate = TRUE);
	void GetPaneInfo(int nIndex, UINT& nID, UINT& nStyle, int& cxWidth) const;
	void SetPaneInfo(int nIndex, UINT nID, UINT nStyle, int cxWidth);
	UINT GetPaneStyle(int nIndex) const;
	void SetPaneStyle(int nIndex, UINT nStyle);
	void UpdateAllPanes(BOOL bUpdateRects, BOOL bUpdateText);

	CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz) override;
	void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler) override;
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;

	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg void OnNcPaint();
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnSetText(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetText(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetTextLength(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetMinHeight(WPARAM wParam, LPARAM lParam);

protected:
	DECLARE_MESSAGE_MAP()

	struct Pane
	{
		UINT nID = 0;
		UINT nStyle = SBPS_NORMAL;
		UINT nFlags = 0;
		int cxText = 0;
		CString strText;
	};

	void loadIndicators(const UINT* lpIDArray, BOOL& bResult);
	std::vector<int> paneRightEdges();

	std::vector<Pane> m_panes;
	int m_nMinHeight;
};

class CDialogBar : public CControlBar
{
	DECLARE_DYNAMIC(CDialogBar)

public:
	CDialogBar();
	~CDialogBar() override;

	virtual BOOL Create(CWnd* pParentWnd, LPCSTR lpszTemplateName, UINT nStyle, UINT nID);
	virtual BOOL Create(CWnd* pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID);

	CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz) override;
	void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler) override;

	afx_msg LRESULT HandleInitDialog(WPARAM wParam, LPARAM lParam);

protected:
	DECLARE_MESSAGE_MAP()

	CSize m_sizeDefault;
	LPCSTR m_lpszTemplateName;
};

class CMiniDockFrameWnd : public CFrameWnd
{
	DECLARE_DYNCREATE(CMiniDockFrameWnd)

public:
	CMiniDockFrameWnd();

	BOOL CreateFor(CFrameWnd* pDockSite, CControlBar* pBar, CPoint point);
	void RecalcLayout(BOOL bNotify = TRUE) override;

	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg LRESULT OnFloatStatus(WPARAM wParam, LPARAM lParam);

	CControlBar* m_pBar;

protected:
	DECLARE_MESSAGE_MAP()
};
