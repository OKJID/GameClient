#include "AfxInternal.h"

#include <algorithm>
#include <cstring>
#include <unistd.h>

IMPLEMENT_DYNAMIC(CDocument, CCmdTarget)

BEGIN_MESSAGE_MAP(CDocument, CCmdTarget)
	ON_COMMAND(ID_FILE_CLOSE, OnFileClose)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
END_MESSAGE_MAP()

namespace
{

class CMirrorFile : public CFile
{
public:
	BOOL Open(LPCSTR lpszFileName, UINT nOpenFlags, CFileException* pError = nullptr) override;
	void Close() override;
	void Abort() override;

private:
	CString m_strMirrorName;
};

CString mirrorNameFor(LPCSTR lpszFileName)
{
	CString directory = lpszFileName;
	const int separator = directory.ReverseFind('\\');
	directory = separator >= 0 ? directory.Left(separator + 1) : CString();
	static unsigned int counter = 0;
	CFileStatus status;
	CString candidate;
	do
	{
		candidate.Format("%sMFC%04X.TMP", static_cast<LPCSTR>(directory), (static_cast<unsigned int>(::getpid()) + counter++) & 0xFFFF);
	} while (CFile::GetStatus(candidate, status));
	return candidate;
}

BOOL CMirrorFile::Open(LPCSTR lpszFileName, UINT nOpenFlags, CFileException* pError)
{
	m_strMirrorName.Empty();
	CFileStatus status;
	if ((nOpenFlags & modeCreate) != 0 && CFile::GetStatus(lpszFileName, status))
	{
		m_strMirrorName = mirrorNameFor(lpszFileName);
	}
	if (!m_strMirrorName.IsEmpty() && CFile::Open(m_strMirrorName, nOpenFlags, pError))
	{
		m_strFileName = lpszFileName;
		return TRUE;
	}
	m_strMirrorName.Empty();
	return CFile::Open(lpszFileName, nOpenFlags, pError);
}

void CMirrorFile::Close()
{
	const CString fileName = m_strFileName;
	CFile::Close();
	if (m_strMirrorName.IsEmpty())
	{
		return;
	}
	CFile::Rename(m_strMirrorName, fileName);
}

void CMirrorFile::Abort()
{
	CFile::Abort();
	if (!m_strMirrorName.IsEmpty())
	{
		CFile::Remove(m_strMirrorName);
	}
}

CString titleOfPath(LPCSTR lpszPathName)
{
	char title[_MAX_PATH] = {};
	AfxGetFileTitle(lpszPathName, title, sizeof(title));
	return title;
}

UINT promptForFileException(CFileException* pException, LPCSTR lpszPathName, BOOL bSaving, UINT nIDPDefault, CString& prompt)
{
	if (pException->m_strFileName.IsEmpty())
	{
		pException->m_strFileName = lpszPathName;
	}
	char message[256] = {};
	if (pException->GetErrorMessage(message, sizeof(message)))
	{
		prompt = message;
		return nIDPDefault;
	}
	switch (pException->m_cause)
	{
	case CFileException::fileNotFound:
	case CFileException::badPath:
		return AFX_IDP_FAILED_INVALID_PATH;
	case CFileException::diskFull:
		return AFX_IDP_FAILED_DISK_FULL;
	case CFileException::accessDenied:
		return bSaving ? AFX_IDP_FAILED_ACCESS_WRITE : AFX_IDP_FAILED_ACCESS_READ;
	default:
		return nIDPDefault;
	}
}

}

POSITION AFXAPI AfxPositionFromIndex(size_t index, size_t count)
{
	return index < count ? reinterpret_cast<POSITION>(index + 1) : nullptr;
}

size_t AFXAPI AfxIndexFromPosition(POSITION position)
{
	return reinterpret_cast<size_t>(position) - 1;
}

BOOL AFXAPI AfxFullPath(LPSTR lpszPathOut, LPCSTR lpszFileIn)
{
	const DWORD length = ::GetFullPathNameA(lpszFileIn, _MAX_PATH, lpszPathOut, nullptr);
	if (length == 0 || length >= _MAX_PATH)
	{
		strlcpy(lpszPathOut, lpszFileIn, _MAX_PATH);
		return FALSE;
	}
	return TRUE;
}

UINT AFXAPI AfxGetFileTitle(LPCSTR lpszPathName, LPSTR lpszTitle, UINT nMax)
{
	const char* name = lpszPathName;
	for (const char* current = lpszPathName; *current != '\0'; ++current)
	{
		if (*current == '\\' || *current == '/' || *current == ':')
		{
			name = current + 1;
		}
	}
	const UINT required = static_cast<UINT>(strlen(name)) + 1;
	if (lpszTitle == nullptr)
	{
		return required;
	}
	if (nMax < required)
	{
		return required;
	}
	memcpy(lpszTitle, name, required);
	return 0;
}

CDocument::CDocument() :
	m_pDocTemplate(nullptr),
	m_bModified(FALSE),
	m_bAutoDelete(TRUE),
	m_bEmbedded(FALSE)
{
}

CDocument::~CDocument()
{
	for (CView* view : m_viewList)
	{
		view->m_pDocument = nullptr;
	}
	m_viewList.clear();
	if (m_pDocTemplate != nullptr)
	{
		m_pDocTemplate->RemoveDocument(this);
	}
}

void CDocument::SetTitle(LPCSTR lpszTitle)
{
	m_strTitle = lpszTitle;
	UpdateFrameCounts();
}

void CDocument::SetPathName(LPCSTR lpszPathName, BOOL bAddToMRU)
{
	char fullPath[_MAX_PATH];
	AfxFullPath(fullPath, lpszPathName);
	m_strPathName = fullPath;
	m_bEmbedded = FALSE;

	char title[_MAX_PATH];
	if (AfxGetFileTitle(fullPath, title, sizeof(title)) == 0)
	{
		SetTitle(title);
	}
	if (bAddToMRU)
	{
		AfxGetApp()->AddToRecentFileList(m_strPathName);
	}
}

POSITION CDocument::GetFirstViewPosition() const
{
	return AfxPositionFromIndex(0, m_viewList.size());
}

CView* CDocument::GetNextView(POSITION& rPosition) const
{
	if (rPosition == nullptr)
	{
		return nullptr;
	}
	const size_t index = AfxIndexFromPosition(rPosition);
	rPosition = AfxPositionFromIndex(index + 1, m_viewList.size());
	return index < m_viewList.size() ? m_viewList[index] : nullptr;
}

void CDocument::UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint)
{
	POSITION position = GetFirstViewPosition();
	while (position != nullptr)
	{
		CView* view = GetNextView(position);
		if (view != pSender)
		{
			view->OnUpdate(pSender, lHint, pHint);
		}
	}
}

void CDocument::AddView(CView* pView)
{
	m_viewList.push_back(pView);
	pView->m_pDocument = this;
	OnChangedViewList();
}

void CDocument::RemoveView(CView* pView)
{
	m_viewList.erase(std::remove(m_viewList.begin(), m_viewList.end(), pView), m_viewList.end());
	pView->m_pDocument = nullptr;
	OnChangedViewList();
}

void CDocument::OnChangedViewList()
{
	if (m_viewList.empty() && m_bAutoDelete)
	{
		OnCloseDocument();
		return;
	}
	UpdateFrameCounts();
}

void CDocument::UpdateFrameCounts()
{
	std::vector<CFrameWnd*> frames;
	for (CView* view : m_viewList)
	{
		CFrameWnd* frame = view->IsWindowVisible() ? view->GetParentFrame() : nullptr;
		if (frame != nullptr && std::find(frames.begin(), frames.end(), frame) == frames.end())
		{
			frames.push_back(frame);
		}
	}
	int number = 0;
	for (CFrameWnd* frame : frames)
	{
		frame->m_nWindow = frames.size() == 1 ? 0 : ++number;
		frame->OnUpdateFrameTitle(TRUE);
	}
}

void CDocument::DeleteContents()
{
}

BOOL CDocument::OnNewDocument()
{
	DeleteContents();
	m_strPathName.Empty();
	SetModifiedFlag(FALSE);
	return TRUE;
}

BOOL CDocument::OnOpenDocument(LPCSTR lpszPathName)
{
	CFileException fileException;
	CFile* file = GetFile(lpszPathName, CFile::modeRead | CFile::shareDenyWrite, &fileException);
	if (file == nullptr)
	{
		ReportSaveLoadException(lpszPathName, &fileException, FALSE, AFX_IDP_FAILED_TO_OPEN_DOC);
		return FALSE;
	}

	DeleteContents();
	SetModifiedFlag();
	CArchive archive(file, CArchive::load | CArchive::bNoFlushOnDelete);
	archive.m_pDocument = this;
	archive.m_bForceFlat = FALSE;
	try
	{
		CWaitCursor wait;
		if (file->GetLength() != 0)
		{
			Serialize(archive);
		}
		archive.Close();
		ReleaseFile(file, FALSE);
	}
	catch (CException* exception)
	{
		ReleaseFile(file, TRUE);
		DeleteContents();
		ReportSaveLoadException(lpszPathName, exception, FALSE, AFX_IDP_FAILED_TO_OPEN_DOC);
		exception->Delete();
		return FALSE;
	}
	SetModifiedFlag(FALSE);
	return TRUE;
}

BOOL CDocument::OnSaveDocument(LPCSTR lpszPathName)
{
	CFileException fileException;
	CFile* file = GetFile(lpszPathName, CFile::modeCreate | CFile::modeReadWrite | CFile::shareExclusive, &fileException);
	if (file == nullptr)
	{
		ReportSaveLoadException(lpszPathName, &fileException, TRUE, AFX_IDP_INVALID_FILENAME);
		return FALSE;
	}

	CArchive archive(file, CArchive::store | CArchive::bNoFlushOnDelete);
	archive.m_pDocument = this;
	archive.m_bForceFlat = FALSE;
	try
	{
		CWaitCursor wait;
		Serialize(archive);
		archive.Close();
		ReleaseFile(file, FALSE);
	}
	catch (CException* exception)
	{
		ReleaseFile(file, TRUE);
		ReportSaveLoadException(lpszPathName, exception, TRUE, AFX_IDP_FAILED_TO_SAVE_DOC);
		exception->Delete();
		return FALSE;
	}
	SetModifiedFlag(FALSE);
	return TRUE;
}

void CDocument::OnCloseDocument()
{
	const BOOL autoDelete = m_bAutoDelete;
	m_bAutoDelete = FALSE;
	while (!m_viewList.empty())
	{
		CView* view = m_viewList.front();
		CFrameWnd* frame = view->GetParentFrame();
		if (frame == nullptr)
		{
			view->DestroyWindow();
			continue;
		}
		PreCloseFrame(frame);
		frame->DestroyWindow();
	}
	m_bAutoDelete = autoDelete;
	DeleteContents();
	if (m_bAutoDelete)
	{
		delete this;
	}
}

void CDocument::ReportSaveLoadException(LPCSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault)
{
	UINT nIDP = nIDPDefault;
	CString prompt;
	if (e != nullptr && e->IsKindOf(RUNTIME_CLASS(CUserException)))
	{
		return;
	}
	if (e != nullptr && e->IsKindOf(RUNTIME_CLASS(CFileException)))
	{
		nIDP = promptForFileException(static_cast<CFileException*>(e), lpszPathName, bSaving, nIDPDefault, prompt);
	}
	if (prompt.IsEmpty())
	{
		AfxFormatString1(prompt, nIDP, titleOfPath(lpszPathName));
	}
	AfxMessageBox(prompt, MB_ICONEXCLAMATION, nIDPDefault);
}

CFile* CDocument::GetFile(LPCSTR lpszFileName, UINT nOpenFlags, CFileException* pError)
{
	CMirrorFile* file = new CMirrorFile;
	if (!file->Open(lpszFileName, nOpenFlags, pError))
	{
		delete file;
		return nullptr;
	}
	return file;
}

void CDocument::ReleaseFile(CFile* pFile, BOOL bAbort)
{
	if (bAbort)
	{
		pFile->Abort();
	}
	else
	{
		pFile->Close();
	}
	delete pFile;
}

BOOL CDocument::CanCloseFrame(CFrameWnd*)
{
	for (CView* view : m_viewList)
	{
		CFrameWnd* frame = view->GetParentFrame();
		if (frame != nullptr && frame->m_nWindow > 0)
		{
			return TRUE;
		}
	}
	return SaveModified();
}

void CDocument::PreCloseFrame(CFrameWnd*)
{
}

BOOL CDocument::SaveModified()
{
	if (!IsModified())
	{
		return TRUE;
	}
	CString name = m_strPathName.IsEmpty() ? m_strTitle : titleOfPath(m_strPathName);
	if (name.IsEmpty())
	{
		name.LoadString(AFX_IDS_UNTITLED);
	}
	CString prompt;
	AfxFormatString1(prompt, AFX_IDP_ASK_TO_SAVE, name);
	switch (AfxMessageBox(prompt, MB_YESNOCANCEL, AFX_IDP_ASK_TO_SAVE))
	{
	case IDCANCEL:
		return FALSE;
	case IDYES:
		return DoFileSave();
	default:
		return TRUE;
	}
}

BOOL CDocument::DoSave(LPCSTR lpszPathName, BOOL bReplace)
{
	CString newName = lpszPathName;
	if (newName.IsEmpty())
	{
		CDocTemplate* docTemplate = GetDocTemplate();
		newName = m_strPathName;
		if (bReplace && newName.IsEmpty())
		{
			newName = m_strTitle;
			const int bad = newName.FindOneOf(" #%;/\\");
			if (bad != -1)
			{
				newName.ReleaseBuffer(bad);
			}
			CString extension;
			if (docTemplate->GetDocString(extension, CDocTemplate::filterExt) && !extension.IsEmpty())
			{
				newName += extension;
			}
		}
		if (!AfxGetApp()->DoPromptFileName(newName, bReplace ? AFX_IDS_SAVEFILE : AFX_IDS_SAVEFILECOPY, OFN_HIDEREADONLY | OFN_PATHMUSTEXIST,
				FALSE, docTemplate))
		{
			return FALSE;
		}
	}

	CWaitCursor wait;
	if (!OnSaveDocument(newName))
	{
		if (lpszPathName == nullptr)
		{
			try
			{
				CFile::Remove(newName);
			}
			catch (CException* exception)
			{
				exception->Delete();
			}
		}
		return FALSE;
	}
	if (bReplace)
	{
		SetPathName(newName);
	}
	return TRUE;
}

BOOL CDocument::DoFileSave()
{
	const DWORD attributes = ::GetFileAttributesA(m_strPathName);
	if ((attributes & FILE_ATTRIBUTE_READONLY) != 0)
	{
		return DoSave(nullptr);
	}
	return DoSave(m_strPathName);
}

HACCEL CDocument::GetDefaultAccelerator()
{
	return nullptr;
}

BOOL CDocument::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	if (CCmdTarget::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
	{
		return TRUE;
	}
	return m_pDocTemplate != nullptr && m_pDocTemplate->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo) ? TRUE : FALSE;
}

void CDocument::OnFileClose()
{
	if (!SaveModified())
	{
		return;
	}
	OnCloseDocument();
}

void CDocument::OnFileSave()
{
	DoFileSave();
}

void CDocument::OnFileSaveAs()
{
	DoSave(nullptr);
}
