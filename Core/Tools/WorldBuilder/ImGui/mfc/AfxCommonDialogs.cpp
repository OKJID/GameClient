#include "AfxInternal.h"

#include "afxdlgs.h"

#include <cstring>

IMPLEMENT_DYNAMIC(CCommonDialog, CDialog)
IMPLEMENT_DYNAMIC(CFileDialog, CCommonDialog)
IMPLEMENT_DYNAMIC(CColorDialog, CCommonDialog)
IMPLEMENT_DYNAMIC(CPrintDialog, CCommonDialog)

namespace
{

COLORREF savedCustomColors[16] = {
	RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255),
	RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255),
	RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255),
	RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255),
};

}

CCommonDialog::CCommonDialog(CWnd* pParentWnd) :
	CDialog(static_cast<UINT>(0), pParentWnd)
{
}

void CCommonDialog::OnOK()
{
	Default();
}

void CCommonDialog::OnCancel()
{
	Default();
}

CFileDialog::CFileDialog(BOOL bOpenFileDialog, LPCSTR lpszDefExt, LPCSTR lpszFileName, DWORD dwFlags, LPCSTR lpszFilter, CWnd* pParentWnd) :
	CCommonDialog(pParentWnd),
	m_ofn(),
	m_bOpenFileDialog(bOpenFileDialog),
	m_szFileTitle(),
	m_szFileName()
{
	m_ofn.lStructSize = sizeof(m_ofn);
	m_ofn.lpstrFile = m_szFileName;
	m_ofn.nMaxFile = sizeof(m_szFileName);
	m_ofn.lpstrDefExt = lpszDefExt;
	m_ofn.lpstrFileTitle = m_szFileTitle;
	m_ofn.nMaxFileTitle = sizeof(m_szFileTitle);
	m_ofn.Flags |= dwFlags | OFN_ENABLESIZING | OFN_EXPLORER;
	m_ofn.hInstance = AfxGetResourceHandle();
	if (lpszFileName != nullptr)
	{
		strlcpy(m_szFileName, lpszFileName, sizeof(m_szFileName));
	}
	if (lpszFilter == nullptr)
	{
		return;
	}
	m_strFilter = lpszFilter;
	LPSTR buffer = m_strFilter.GetBuffer(0);
	for (char* separator = strchr(buffer, '|'); separator != nullptr; separator = strchr(separator + 1, '|'))
	{
		*separator = '\0';
	}
	m_ofn.lpstrFilter = m_strFilter;
}

INT_PTR CFileDialog::DoModal()
{
	const size_t used = strlen(m_ofn.lpstrFile) + 1;
	memset(m_ofn.lpstrFile + used, 0, m_ofn.nMaxFile - used);

	const HWND focus = ::GetFocus();
	m_ofn.hwndOwner = PreModal();
	AfxUnhookWindowCreate();
	const bool enablesParent = m_ofn.hwndOwner != nullptr && ::IsWindowEnabled(m_ofn.hwndOwner);
	if (enablesParent)
	{
		::EnableWindow(m_ofn.hwndOwner, FALSE);
	}
	const BOOL result = m_bOpenFileDialog ? ::GetOpenFileNameA(&m_ofn) : ::GetSaveFileNameA(&m_ofn);
	if (enablesParent)
	{
		::EnableWindow(m_ofn.hwndOwner, TRUE);
	}
	if (::IsWindow(focus))
	{
		::SetFocus(focus);
	}
	PostModal();
	return result ? result : IDCANCEL;
}

CString CFileDialog::GetPathName() const
{
	return m_ofn.lpstrFile;
}

CString CFileDialog::GetFileName() const
{
	return m_ofn.lpstrFileTitle;
}

CString CFileDialog::GetFileExt() const
{
	return m_ofn.nFileExtension != 0 ? CString(m_ofn.lpstrFile + m_ofn.nFileExtension) : CString();
}

CString CFileDialog::GetFileTitle() const
{
	CString title = GetFileName();
	const int dot = title.ReverseFind('.');
	return dot >= 0 ? title.Left(dot) : title;
}

CColorDialog::CColorDialog(COLORREF clrInit, DWORD dwFlags, CWnd* pParentWnd) :
	CCommonDialog(pParentWnd),
	m_cc()
{
	m_cc.lStructSize = sizeof(m_cc);
	m_cc.lpCustColors = GetSavedCustomColors();
	m_cc.Flags = dwFlags;
	m_cc.rgbResult = clrInit;
	if (clrInit != 0)
	{
		m_cc.Flags |= CC_RGBINIT;
	}
}

INT_PTR CColorDialog::DoModal()
{
	m_cc.hwndOwner = PreModal();
	const BOOL result = ::ChooseColorA(&m_cc);
	PostModal();
	return result ? result : IDCANCEL;
}

COLORREF* PASCAL CColorDialog::GetSavedCustomColors()
{
	return savedCustomColors;
}

CPrintDialog::CPrintDialog(BOOL bPrintSetupOnly, DWORD dwFlags, CWnd* pParentWnd) :
	CCommonDialog(pParentWnd),
	m_pd()
{
	m_pd.lStructSize = sizeof(m_pd);
	m_pd.Flags = dwFlags | (bPrintSetupOnly ? PD_PRINTSETUP : PD_RETURNDC);
	m_pd.Flags &= ~PD_RETURNIC;
}

INT_PTR CPrintDialog::DoModal()
{
	m_pd.hwndOwner = PreModal();
	AfxUnhookWindowCreate();
	const BOOL result = ::PrintDlgA(&m_pd);
	PostModal();
	return result ? result : IDCANCEL;
}

CPrintInfo::CPrintInfo() :
	m_pPD(new CPrintDialog(FALSE, PD_ALLPAGES | PD_USEDEVMODECOPIES | PD_NOSELECTION)),
	m_bDirect(FALSE),
	m_bPreview(FALSE),
	m_bContinuePrinting(TRUE),
	m_nCurPage(1),
	m_nNumPreviewPages(0),
	m_lpUserData(nullptr)
{
	SetMinPage(1);
	SetMaxPage(0xFFFF);
}

CPrintInfo::~CPrintInfo()
{
	delete m_pPD;
}

void CPrintInfo::SetMinPage(UINT nMinPage) { m_pPD->m_pd.nMinPage = static_cast<WORD>(nMinPage); }
void CPrintInfo::SetMaxPage(UINT nMaxPage) { m_pPD->m_pd.nMaxPage = static_cast<WORD>(nMaxPage); }
UINT CPrintInfo::GetMinPage() const { return m_pPD->m_pd.nMinPage; }
UINT CPrintInfo::GetMaxPage() const { return m_pPD->m_pd.nMaxPage; }
UINT CPrintInfo::GetFromPage() const { return m_pPD->m_pd.nFromPage; }
UINT CPrintInfo::GetToPage() const { return m_pPD->m_pd.nToPage; }

int CWinApp::DoPrintDialog(CPrintDialog* pPD)
{
	return static_cast<int>(pPD->DoModal());
}

BOOL CWinApp::GetPrinterDeviceDefaults(PRINTDLG* pPrintDlg)
{
	PRINTDLG defaults = {};
	defaults.lStructSize = sizeof(defaults);
	defaults.Flags = PD_RETURNDEFAULT;
	if (!::PrintDlgA(&defaults))
	{
		return FALSE;
	}
	pPrintDlg->hDevMode = defaults.hDevMode;
	pPrintDlg->hDevNames = defaults.hDevNames;
	return TRUE;
}

void CWinApp::OnFilePrintSetup()
{
	CPrintDialog dialog(TRUE);
	DoPrintDialog(&dialog);
}
