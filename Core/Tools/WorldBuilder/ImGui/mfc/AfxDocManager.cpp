#include "AfxInternal.h"

#include "afxdlgs.h"

#include <cstring>
#include <strings.h>

IMPLEMENT_DYNAMIC(CDocManager, CObject)

namespace
{

void appendFilterSuffix(CString& filter, OPENFILENAME& ofn, CDocTemplate* pTemplate, CString* pstrDefaultExt)
{
	CString filterExtension;
	CString filterName;
	if (!pTemplate->GetDocString(filterExtension, CDocTemplate::filterExt) || filterExtension.IsEmpty()
		|| !pTemplate->GetDocString(filterName, CDocTemplate::filterName) || filterName.IsEmpty())
	{
		return;
	}
	if (pstrDefaultExt != nullptr)
	{
		*pstrDefaultExt = static_cast<LPCSTR>(filterExtension) + 1;
		ofn.lpstrDefExt = *pstrDefaultExt;
		ofn.nFilterIndex = ofn.nMaxCustFilter + 1;
	}
	filter += filterName;
	filter += '\0';
	filter += '*';
	filter += filterExtension;
	filter += '\0';
	++ofn.nMaxCustFilter;
}

CString unquotedPath(LPCSTR lpszFileName)
{
	CString path = lpszFileName[0] == '"' ? lpszFileName + 1 : lpszFileName;
	const int quote = path.ReverseFind('"');
	if (quote >= 0)
	{
		path = path.Left(quote);
	}
	return path;
}

}

CCommandLineInfo::CCommandLineInfo() :
	m_bShowSplash(TRUE),
	m_bRunEmbedded(FALSE),
	m_bRunAutomated(FALSE),
	m_nShellCommand(FileNew)
{
}

void CCommandLineInfo::ParseParam(LPCSTR lpszParam, BOOL bFlag, BOOL bLast)
{
	if (bFlag)
	{
		ParseParamFlag(lpszParam);
	}
	else
	{
		ParseParamNotFlag(lpszParam);
	}
	ParseLast(bLast);
}

void CCommandLineInfo::ParseParamFlag(LPCSTR pszParam)
{
	if (strcmp(pszParam, "pt") == 0)
	{
		m_nShellCommand = FilePrintTo;
	}
	else if (strcmp(pszParam, "p") == 0)
	{
		m_nShellCommand = FilePrint;
	}
}

void CCommandLineInfo::ParseParamNotFlag(LPCSTR pszParam)
{
	if (m_strFileName.IsEmpty())
	{
		m_strFileName = pszParam;
	}
	else if (m_nShellCommand == FilePrintTo && m_strPrinterName.IsEmpty())
	{
		m_strPrinterName = pszParam;
	}
	else if (m_nShellCommand == FilePrintTo && m_strDriverName.IsEmpty())
	{
		m_strDriverName = pszParam;
	}
	else if (m_nShellCommand == FilePrintTo && m_strPortName.IsEmpty())
	{
		m_strPortName = pszParam;
	}
}

void CCommandLineInfo::ParseLast(BOOL bLast)
{
	if (!bLast)
	{
		return;
	}
	if (m_nShellCommand == FileNew && !m_strFileName.IsEmpty())
	{
		m_nShellCommand = FileOpen;
	}
	m_bShowSplash = !m_bRunEmbedded && !m_bRunAutomated;
}

CDocManager::~CDocManager()
{
	std::vector<CDocTemplate*> templates;
	templates.swap(m_templateList);
	for (CDocTemplate* docTemplate : templates)
	{
		if (docTemplate->m_bAutoDelete)
		{
			delete docTemplate;
			continue;
		}
		m_templateList.push_back(docTemplate);
	}
}

void CDocManager::AddDocTemplate(CDocTemplate* pTemplate)
{
	pTemplate->LoadTemplate();
	m_templateList.push_back(pTemplate);
}

POSITION CDocManager::GetFirstDocTemplatePosition() const
{
	return AfxPositionFromIndex(0, m_templateList.size());
}

CDocTemplate* CDocManager::GetNextDocTemplate(POSITION& pos) const
{
	const size_t index = AfxIndexFromPosition(pos);
	pos = AfxPositionFromIndex(index + 1, m_templateList.size());
	return m_templateList[index];
}

BOOL CDocManager::DoPromptFileName(CString& fileName, UINT nIDSTitle, DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate)
{
	CFileDialog dialog(bOpenFileDialog, nullptr, nullptr, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, nullptr, nullptr);
	CString title;
	title.LoadString(nIDSTitle);
	dialog.m_ofn.Flags |= lFlags;

	CString filter;
	CString defaultExtension;
	if (pTemplate != nullptr)
	{
		appendFilterSuffix(filter, dialog.m_ofn, pTemplate, &defaultExtension);
	}
	else
	{
		for (size_t index = 0; index < m_templateList.size(); ++index)
		{
			appendFilterSuffix(filter, dialog.m_ofn, m_templateList[index], index == 0 ? &defaultExtension : nullptr);
		}
	}

	CString allFilter;
	allFilter.LoadString(AFX_IDS_ALLFILTER);
	filter += allFilter;
	filter += '\0';
	filter += "*.*";
	filter += '\0';
	++dialog.m_ofn.nMaxCustFilter;

	dialog.m_ofn.lpstrFilter = filter;
	dialog.m_ofn.lpstrTitle = title;
	dialog.m_ofn.lpstrFile = fileName.GetBuffer(_MAX_PATH);
	const INT_PTR result = dialog.DoModal();
	fileName.ReleaseBuffer();
	return result == IDOK ? TRUE : FALSE;
}

CDocument* CDocManager::OpenDocumentFile(LPCSTR lpszFileName)
{
	char path[_MAX_PATH];
	AfxFullPath(path, unquotedPath(lpszFileName));

	CDocTemplate::Confidence bestMatch = CDocTemplate::noAttempt;
	CDocTemplate* bestTemplate = nullptr;
	CDocument* openDocument = nullptr;
	for (CDocTemplate* docTemplate : m_templateList)
	{
		const CDocTemplate::Confidence match = docTemplate->MatchDocType(path, openDocument);
		if (match > bestMatch)
		{
			bestMatch = match;
			bestTemplate = docTemplate;
		}
		if (match == CDocTemplate::yesAlreadyOpen)
		{
			break;
		}
	}

	if (openDocument != nullptr)
	{
		return activateOpenDocument(openDocument);
	}
	if (bestTemplate == nullptr)
	{
		AfxMessageBox(AFX_IDP_FAILED_TO_OPEN_DOC);
		return nullptr;
	}
	return bestTemplate->OpenDocumentFile(path);
}

CDocument* CDocManager::activateOpenDocument(CDocument* pDocument)
{
	POSITION position = pDocument->GetFirstViewPosition();
	if (position == nullptr)
	{
		return pDocument;
	}
	CFrameWnd* frame = pDocument->GetNextView(position)->GetParentFrame();
	if (frame != nullptr)
	{
		frame->ActivateFrame();
	}
	CFrameWnd* appFrame = static_cast<CFrameWnd*>(AfxGetApp()->m_pMainWnd);
	if (frame != appFrame)
	{
		appFrame->ActivateFrame();
	}
	return pDocument;
}

BOOL CDocManager::SaveAllModified()
{
	for (CDocTemplate* docTemplate : m_templateList)
	{
		if (!docTemplate->SaveAllModified())
		{
			return FALSE;
		}
	}
	return TRUE;
}

void CDocManager::CloseAllDocuments(BOOL bEndSession)
{
	for (CDocTemplate* docTemplate : m_templateList)
	{
		docTemplate->CloseAllDocuments(bEndSession);
	}
}

int CDocManager::GetOpenDocumentCount()
{
	int count = 0;
	for (CDocTemplate* docTemplate : m_templateList)
	{
		POSITION position = docTemplate->GetFirstDocPosition();
		while (position != nullptr)
		{
			docTemplate->GetNextDoc(position);
			++count;
		}
	}
	return count;
}

void CDocManager::OnFileNew()
{
	if (m_templateList.empty())
	{
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		return;
	}
	m_templateList.front()->OpenDocumentFile(nullptr);
}

void CDocManager::OnFileOpen()
{
	CString newName;
	if (!DoPromptFileName(newName, AFX_IDS_OPENFILE, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, TRUE, nullptr))
	{
		return;
	}
	AfxGetApp()->OpenDocumentFile(newName);
}

void CWinApp::AddDocTemplate(CDocTemplate* pTemplate)
{
	if (m_pDocManager == nullptr)
	{
		m_pDocManager = new CDocManager;
	}
	m_pDocManager->AddDocTemplate(pTemplate);
}

POSITION CWinApp::GetFirstDocTemplatePosition() const
{
	return m_pDocManager != nullptr ? m_pDocManager->GetFirstDocTemplatePosition() : nullptr;
}

CDocTemplate* CWinApp::GetNextDocTemplate(POSITION& pos) const
{
	return m_pDocManager->GetNextDocTemplate(pos);
}

BOOL CWinApp::DoPromptFileName(CString& fileName, UINT nIDSTitle, DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate)
{
	return m_pDocManager->DoPromptFileName(fileName, nIDSTitle, lFlags, bOpenFileDialog, pTemplate);
}

CDocument* CWinApp::OpenDocumentFile(LPCSTR lpszFileName)
{
	return m_pDocManager->OpenDocumentFile(lpszFileName);
}

BOOL CWinApp::SaveAllModified()
{
	return m_pDocManager != nullptr ? m_pDocManager->SaveAllModified() : TRUE;
}

void CWinApp::CloseAllDocuments(BOOL bEndSession)
{
	if (m_pDocManager != nullptr)
	{
		m_pDocManager->CloseAllDocuments(bEndSession);
	}
}

void CWinApp::AddToRecentFileList(LPCSTR lpszPathName)
{
	if (m_pRecentFileList != nullptr)
	{
		m_pRecentFileList->Add(lpszPathName);
	}
}

void CWinApp::OnFileNew()
{
	if (m_pDocManager != nullptr)
	{
		m_pDocManager->OnFileNew();
	}
}

void CWinApp::OnFileOpen()
{
	m_pDocManager->OnFileOpen();
}

void CWinApp::OnUpdateRecentFileMenu(CCmdUI* pCmdUI)
{
	if (m_pRecentFileList == nullptr)
	{
		pCmdUI->Enable(FALSE);
		return;
	}
	m_pRecentFileList->UpdateMenu(pCmdUI);
}

BOOL CWinApp::OnOpenRecentFile(UINT nID)
{
	const int index = static_cast<int>(nID - ID_FILE_MRU_FILE1);
	if (OpenDocumentFile((*m_pRecentFileList)[index]) == nullptr)
	{
		m_pRecentFileList->Remove(index);
	}
	return TRUE;
}

void CWinApp::ParseCommandLine(CCommandLineInfo& rCmdInfo)
{
	for (int index = 1; index < __argc; ++index)
	{
		LPCSTR param = __argv[index];
		const BOOL isFlag = param[0] == '-' || param[0] == '/';
		rCmdInfo.ParseParam(isFlag ? param + 1 : param, isFlag, index + 1 == __argc);
	}
}

BOOL CWinApp::ProcessShellCommand(CCommandLineInfo& rCmdInfo)
{
	switch (rCmdInfo.m_nShellCommand)
	{
	case CCommandLineInfo::FileNew:
		if (!OnCmdMsg(ID_FILE_NEW, 0, nullptr, nullptr))
		{
			OnFileNew();
		}
		return m_pMainWnd != nullptr ? TRUE : FALSE;
	case CCommandLineInfo::FileOpen:
		return OpenDocumentFile(rCmdInfo.m_strFileName) != nullptr ? TRUE : FALSE;
	case CCommandLineInfo::FilePrintTo:
	case CCommandLineInfo::FilePrint:
		m_nCmdShow = SW_HIDE;
		OpenDocumentFile(rCmdInfo.m_strFileName);
		m_pCmdInfo = &rCmdInfo;
		m_pMainWnd->SendMessage(WM_COMMAND, ID_FILE_PRINT_DIRECT);
		m_pCmdInfo = nullptr;
		return FALSE;
	default:
		return TRUE;
	}
}
