#include "AfxInternal.h"

#include "afxdlgs.h"

namespace
{

BOOL preparePrinterSilently(CWinApp* app, CPrintInfo* pInfo)
{
	PRINTDLG& printDialog = pInfo->m_pPD->m_pd;
	if (printDialog.hDC == nullptr && !app->GetPrinterDeviceDefaults(&printDialog) && app->DoPrintDialog(pInfo->m_pPD) != IDOK)
	{
		return FALSE;
	}
	printDialog.nFromPage = static_cast<WORD>(pInfo->GetMinPage());
	printDialog.nToPage = static_cast<WORD>(pInfo->GetMaxPage());
	return TRUE;
}

BOOL preparePrinterWithDialog(CWinApp* app, CPrintInfo* pInfo)
{
	PRINTDLG& printDialog = pInfo->m_pPD->m_pd;
	printDialog.nFromPage = static_cast<WORD>(pInfo->GetMinPage());
	printDialog.nToPage = static_cast<WORD>(pInfo->GetMaxPage());
	return app->DoPrintDialog(pInfo->m_pPD) == IDOK ? TRUE : FALSE;
}

}

BOOL CView::DoPreparePrinting(CPrintInfo* pInfo)
{
	PRINTDLG& printDialog = pInfo->m_pPD->m_pd;
	if (printDialog.nMinPage > printDialog.nMaxPage)
	{
		printDialog.nMaxPage = printDialog.nMinPage;
	}
	CWinApp* app = AfxGetApp();
	const BOOL isPrepared = pInfo->m_bPreview || pInfo->m_bDirect ? preparePrinterSilently(app, pInfo) : preparePrinterWithDialog(app, pInfo);
	if (!isPrepared || printDialog.hDC == nullptr)
	{
		return FALSE;
	}
	pInfo->m_nNumPreviewPages = static_cast<UINT>(app->m_nNumPreviewPages);
	return TRUE;
}

void CView::OnFilePrint()
{
	CPrintInfo printInfo;
	const MSG* message = AfxGetCurrentMessage();
	printInfo.m_bDirect = LOWORD(message->wParam) == ID_FILE_PRINT_DIRECT ? TRUE : FALSE;
	if (OnPreparePrinting(&printInfo))
	{
		AfxMessageBox(AFX_IDP_FAILED_TO_START_PRINT);
	}
}

void CView::OnFilePrintPreview()
{
	CPrintInfo printInfo;
	printInfo.m_bPreview = TRUE;
	OnPreparePrinting(&printInfo);
	AfxMessageBox(AFX_IDP_COMMAND_FAILURE);
}
