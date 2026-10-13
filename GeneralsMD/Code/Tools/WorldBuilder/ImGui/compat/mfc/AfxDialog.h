#pragma once

#include "AfxControls.h"

class CUserException : public CException
{
	DECLARE_DYNAMIC(CUserException)
};

void AFXAPI AfxThrowUserException();

class CDataExchange
{
public:
	CDataExchange(CWnd* pDlgWnd, BOOL bSaveAndValidate);

	HWND PrepareCtrl(int nIDC);
	HWND PrepareEditCtrl(int nIDC);
	void Fail();

	BOOL m_bSaveAndValidate;
	CWnd* m_pDlgWnd;
	HWND m_hWndLastControl;
	BOOL m_bEditLastControl;
};

class CDialog : public CWnd
{
	DECLARE_DYNAMIC(CDialog)

public:
	CDialog();
	explicit CDialog(LPCSTR lpszTemplateName, CWnd* pParentWnd = nullptr);
	explicit CDialog(UINT nIDTemplate, CWnd* pParentWnd = nullptr);
	~CDialog() override;

	virtual BOOL Create(LPCSTR lpszTemplateName, CWnd* pParentWnd = nullptr);
	virtual BOOL Create(UINT nIDTemplate, CWnd* pParentWnd = nullptr);
	virtual INT_PTR DoModal();

	void EndDialog(int nResult);
	DWORD GetDefID() const;
	void SetDefID(UINT nID);
	void NextDlgCtrl() const;
	void PrevDlgCtrl() const;
	void GotoDlgCtrl(CWnd* pWndCtrl);
	void MapDialogRect(LPRECT lpRect) const;

	virtual BOOL OnInitDialog();
	virtual void PreInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	BOOL PreTranslateMessage(MSG* pMsg) override;
	BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) override;
	BOOL CheckAutoCenter() override;

	LPCSTR m_lpszTemplateName;
	UINT m_nIDHelp;
	CWnd* m_pParentWnd;
	HWND m_hWndTop;

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg LRESULT HandleInitDialog(WPARAM wParam, LPARAM lParam);

	BOOL CreateDlgFromTemplate(CWnd* pParentWnd);
	HWND PreModal();
	void PostModal();
	void runModal(HWND parent);
};

INT_PTR CALLBACK AfxDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
