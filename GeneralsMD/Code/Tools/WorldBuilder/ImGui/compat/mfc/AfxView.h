#pragma once

#include "AfxFrame.h"

class CPrintDialog;

class CPrintInfo
{
public:
	CPrintInfo();
	~CPrintInfo();

	void SetMinPage(UINT nMinPage);
	void SetMaxPage(UINT nMaxPage);
	UINT GetMinPage() const;
	UINT GetMaxPage() const;
	UINT GetFromPage() const;
	UINT GetToPage() const;

	CPrintDialog* m_pPD;
	BOOL m_bDirect;
	BOOL m_bPreview;
	BOOL m_bContinuePrinting;
	UINT m_nCurPage;
	UINT m_nNumPreviewPages;
	CString m_strPageDesc;
	LPVOID m_lpUserData;
	CRect m_rectDraw;
};

class CView : public CWnd
{
	DECLARE_DYNAMIC(CView)

protected:
	CView();

public:
	~CView() override;

	CDocument* GetDocument() const { return m_pDocument; }

	virtual BOOL IsSelected(const CObject* pDocItem) const;
	virtual void OnDraw(CDC* pDC) = 0;
	virtual void OnInitialUpdate();
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	virtual void OnActivateFrame(UINT nState, CFrameWnd* pFrameWnd);
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = nullptr);
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	BOOL DoPreparePrinting(CPrintInfo* pInfo);

	BOOL PreCreateWindow(CREATESTRUCT& cs) override;
	BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) override;
	void PostNcDestroy() override;

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg LRESULT OnInitialUpdateMessage(WPARAM wParam, LPARAM lParam);
	afx_msg void OnFilePrint();
	afx_msg void OnFilePrintPreview();

	CDocument* m_pDocument;

protected:
	DECLARE_MESSAGE_MAP()

	BOOL IsTopParentActive() const;

	friend class CDocument;
};
