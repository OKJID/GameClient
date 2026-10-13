#pragma once

#include "AfxBars.h"

class CCommonDialog : public CDialog
{
	DECLARE_DYNAMIC(CCommonDialog)

public:
	explicit CCommonDialog(CWnd* pParentWnd);

	void OnOK() override;
	void OnCancel() override;
};

class CFileDialog : public CCommonDialog
{
	DECLARE_DYNAMIC(CFileDialog)

public:
	explicit CFileDialog(BOOL bOpenFileDialog, LPCSTR lpszDefExt = nullptr, LPCSTR lpszFileName = nullptr,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, LPCSTR lpszFilter = nullptr, CWnd* pParentWnd = nullptr);

	INT_PTR DoModal() override;

	CString GetPathName() const;
	CString GetFileName() const;
	CString GetFileExt() const;
	CString GetFileTitle() const;
	BOOL GetReadOnlyPref() const { return (m_ofn.Flags & OFN_READONLY) != 0 ? TRUE : FALSE; }

	OPENFILENAME m_ofn;

protected:
	BOOL m_bOpenFileDialog;
	CString m_strFilter;
	char m_szFileTitle[64];
	char m_szFileName[_MAX_PATH];
};

class CColorDialog : public CCommonDialog
{
	DECLARE_DYNAMIC(CColorDialog)

public:
	explicit CColorDialog(COLORREF clrInit = 0, DWORD dwFlags = 0, CWnd* pParentWnd = nullptr);

	INT_PTR DoModal() override;

	COLORREF GetColor() const { return m_cc.rgbResult; }
	void SetCurrentColor(COLORREF clr) { m_cc.rgbResult = clr; }
	static COLORREF* PASCAL GetSavedCustomColors();

	CHOOSECOLOR m_cc;
};

class CPrintDialog : public CCommonDialog
{
	DECLARE_DYNAMIC(CPrintDialog)

public:
	explicit CPrintDialog(BOOL bPrintSetupOnly, DWORD dwFlags = PD_ALLPAGES | PD_USEDEVMODECOPIES | PD_NOPAGENUMS | PD_HIDEPRINTTOFILE | PD_NOSELECTION,
		CWnd* pParentWnd = nullptr);

	INT_PTR DoModal() override;

	PRINTDLG m_pd;
};

class CPropertySheet;
class CTabCtrl;

class CPropertyPage : public CDialog
{
	DECLARE_DYNAMIC(CPropertyPage)

public:
	explicit CPropertyPage(UINT nIDTemplate, UINT nIDCaption = 0);
	explicit CPropertyPage(LPCSTR lpszTemplateName, UINT nIDCaption = 0);
	~CPropertyPage() override;

	void Construct(UINT nIDTemplate, UINT nIDCaption = 0);
	void Construct(LPCSTR lpszTemplateName, UINT nIDCaption = 0);

	void CancelToClose();
	void SetModified(BOOL bChanged = TRUE);
	LRESULT QuerySiblings(WPARAM wParam, LPARAM lParam);

	virtual BOOL OnApply();
	virtual void OnReset();
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual BOOL OnQueryCancel();
	void OnOK() override;
	void OnCancel() override;

	BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
	BOOL PreTranslateMessage(MSG* pMsg) override;

	PROPSHEETPAGE m_psp;

protected:
	CPropertySheet* GetParentSheet();

	CString m_strCaption;
	BOOL m_bFirstSetActive;

private:
	void CommonConstruct(LPCSTR lpszTemplateName, UINT nIDCaption);
};

class CPropertySheet : public CWnd
{
	DECLARE_DYNAMIC(CPropertySheet)

public:
	CPropertySheet();
	explicit CPropertySheet(UINT nIDCaption, CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
	explicit CPropertySheet(LPCSTR pszCaption, CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
	~CPropertySheet() override;

	void Construct(UINT nIDCaption, CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
	void Construct(LPCSTR pszCaption, CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);

	virtual INT_PTR DoModal();
	virtual BOOL Create(CWnd* pParentWnd = nullptr, DWORD dwStyle = static_cast<DWORD>(-1), DWORD dwExStyle = 0);

	void AddPage(CPropertyPage* pPage);
	void RemovePage(CPropertyPage* pPage);
	void RemovePage(int nPage);
	int GetPageCount() const;
	CPropertyPage* GetPage(int nPage) const;
	int GetPageIndex(CPropertyPage* pPage);
	CPropertyPage* GetActivePage() const;
	int GetActiveIndex() const;
	BOOL SetActivePage(int nPage);
	BOOL SetActivePage(CPropertyPage* pPage);
	void SetTitle(LPCSTR lpszText, UINT nStyle = 0);
	CTabCtrl* GetTabControl() const;
	void EndDialog(int nEndID);
	void PressButton(int nButton);

	BOOL ContinueModal() override;
	BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) override;
	BOOL PreTranslateMessage(MSG* pMsg) override;

	PROPSHEETHEADER m_psh;

protected:
	DECLARE_MESSAGE_MAP()

	BOOL OnCommand(WPARAM wParam, LPARAM lParam) override;
	afx_msg void OnClose();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);

	void CommonConstruct(CWnd* pParentWnd, UINT iSelectPage);
	void BuildPropPageArray();

	std::vector<CPropertyPage*> m_pages;
	std::vector<PROPSHEETPAGE> m_pageArray;
	CString m_strCaption;
	CWnd* m_pParentWnd;
	BOOL m_bModeless;
	DWORD m_dwStyle;
	DWORD m_dwExStyle;

	friend class CPropertyPage;
	friend int CALLBACK AfxPropSheetCallback(HWND hWnd, UINT message, LPARAM lParam);
};
