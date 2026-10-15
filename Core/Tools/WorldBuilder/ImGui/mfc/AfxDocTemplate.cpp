#include "AfxInternal.h"

#include <strings.h>

IMPLEMENT_DYNAMIC(CDocTemplate, CCmdTarget)
IMPLEMENT_DYNAMIC(CSingleDocTemplate, CDocTemplate)

CDocTemplate::CDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass) :
	m_nIDResource(nIDResource),
	m_pDocClass(pDocClass),
	m_pFrameClass(pFrameClass),
	m_pViewClass(pViewClass),
	m_bAutoDelete(TRUE)
{
	LoadTemplate();
}

CDocTemplate::~CDocTemplate()
{
}

void CDocTemplate::LoadTemplate()
{
	if (m_strDocStrings.IsEmpty())
	{
		m_strDocStrings.LoadString(m_nIDResource);
	}
}

BOOL CDocTemplate::GetDocString(CString& rString, DocStringIndex index) const
{
	return AfxExtractSubString(rString, m_strDocStrings, static_cast<int>(index));
}

void CDocTemplate::AddDocument(CDocument* pDoc)
{
	pDoc->m_pDocTemplate = this;
}

void CDocTemplate::RemoveDocument(CDocument* pDoc)
{
	pDoc->m_pDocTemplate = nullptr;
}

CDocTemplate::Confidence CDocTemplate::MatchDocType(LPCSTR lpszPathName, CDocument*& rpDocMatch)
{
	rpDocMatch = nullptr;
	POSITION position = GetFirstDocPosition();
	while (position != nullptr)
	{
		CDocument* document = GetNextDoc(position);
		if (strcasecmp(document->GetPathName(), lpszPathName) == 0)
		{
			rpDocMatch = document;
			return yesAlreadyOpen;
		}
	}

	CString filterExtension;
	if (GetDocString(filterExtension, filterExt) && !filterExtension.IsEmpty())
	{
		const char* dot = strrchr(lpszPathName, '.');
		if (dot != nullptr && strchr(dot, '\\') == nullptr && strcasecmp(dot, filterExtension) == 0)
		{
			return yesAttemptNative;
		}
	}
	return yesAttemptForeign;
}

CDocument* CDocTemplate::CreateNewDocument()
{
	if (m_pDocClass == nullptr)
	{
		return nullptr;
	}
	CDocument* document = static_cast<CDocument*>(m_pDocClass->CreateObject());
	if (document == nullptr)
	{
		return nullptr;
	}
	AddDocument(document);
	return document;
}

CFrameWnd* CDocTemplate::CreateNewFrame(CDocument* pDoc, CFrameWnd* pOther)
{
	if (m_pFrameClass == nullptr)
	{
		return nullptr;
	}
	CCreateContext context;
	context.m_pCurrentFrame = pOther;
	context.m_pCurrentDoc = pDoc;
	context.m_pNewViewClass = m_pViewClass;
	context.m_pNewDocTemplate = this;

	CFrameWnd* frame = static_cast<CFrameWnd*>(m_pFrameClass->CreateObject());
	if (frame == nullptr || !frame->LoadFrame(m_nIDResource, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, nullptr, &context))
	{
		return nullptr;
	}
	return frame;
}

void CDocTemplate::InitialUpdateFrame(CFrameWnd* pFrame, CDocument* pDoc, BOOL bMakeVisible)
{
	pFrame->InitialUpdateFrame(pDoc, bMakeVisible);
}

BOOL CDocTemplate::SaveAllModified()
{
	POSITION position = GetFirstDocPosition();
	while (position != nullptr)
	{
		if (!GetNextDoc(position)->SaveModified())
		{
			return FALSE;
		}
	}
	return TRUE;
}

void CDocTemplate::CloseAllDocuments(BOOL)
{
	POSITION position = GetFirstDocPosition();
	while (position != nullptr)
	{
		GetNextDoc(position)->OnCloseDocument();
	}
}

CSingleDocTemplate::CSingleDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass) :
	CDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass),
	m_pOnlyDoc(nullptr)
{
}

CSingleDocTemplate::~CSingleDocTemplate()
{
}

void CSingleDocTemplate::AddDocument(CDocument* pDoc)
{
	CDocTemplate::AddDocument(pDoc);
	m_pOnlyDoc = pDoc;
}

void CSingleDocTemplate::RemoveDocument(CDocument* pDoc)
{
	CDocTemplate::RemoveDocument(pDoc);
	m_pOnlyDoc = nullptr;
}

POSITION CSingleDocTemplate::GetFirstDocPosition() const
{
	return m_pOnlyDoc != nullptr ? BEFORE_START_POSITION : nullptr;
}

CDocument* CSingleDocTemplate::GetNextDoc(POSITION& rPos) const
{
	CDocument* document = rPos == BEFORE_START_POSITION ? m_pOnlyDoc : nullptr;
	rPos = nullptr;
	return document;
}

void CSingleDocTemplate::SetDefaultTitle(CDocument* pDocument)
{
	CString documentName;
	if (!GetDocString(documentName, docName) || documentName.IsEmpty())
	{
		documentName.LoadString(AFX_IDS_UNTITLED);
	}
	pDocument->SetTitle(documentName);
}

CDocument* CSingleDocTemplate::OpenDocumentFile(LPCSTR lpszPathName, BOOL bMakeVisible)
{
	CDocument* document = m_pOnlyDoc;
	CFrameWnd* frame = nullptr;
	const BOOL created = document == nullptr;
	if (!created)
	{
		if (!document->SaveModified())
		{
			return nullptr;
		}
		frame = static_cast<CFrameWnd*>(AfxGetMainWnd());
	}
	else
	{
		document = CreateNewDocument();
	}
	if (document == nullptr)
	{
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		return nullptr;
	}

	if (frame == nullptr)
	{
		const BOOL autoDelete = document->m_bAutoDelete;
		document->m_bAutoDelete = FALSE;
		frame = CreateNewFrame(document, nullptr);
		document->m_bAutoDelete = autoDelete;
		if (frame == nullptr)
		{
			AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
			delete document;
			return nullptr;
		}
	}

	if (!reopenDocument(document, frame, created, lpszPathName, bMakeVisible))
	{
		return nullptr;
	}
	CWinThread* thread = AfxGetThread();
	if (created && thread->m_pMainWnd == nullptr)
	{
		thread->m_pMainWnd = frame;
	}
	InitialUpdateFrame(frame, document, bMakeVisible);
	return document;
}

BOOL CSingleDocTemplate::reopenDocument(CDocument* pDocument, CFrameWnd* pFrame, BOOL bCreated, LPCSTR lpszPathName, BOOL bMakeVisible)
{
	if (lpszPathName == nullptr)
	{
		SetDefaultTitle(pDocument);
		if (!bMakeVisible)
		{
			pDocument->m_bEmbedded = TRUE;
		}
		if (pDocument->OnNewDocument())
		{
			return TRUE;
		}
		if (bCreated)
		{
			pFrame->DestroyWindow();
		}
		return FALSE;
	}

	CWaitCursor wait;
	const BOOL wasModified = pDocument->IsModified();
	pDocument->SetModifiedFlag(FALSE);
	if (pDocument->OnOpenDocument(lpszPathName))
	{
		pDocument->SetPathName(lpszPathName);
		return TRUE;
	}
	if (bCreated)
	{
		pFrame->DestroyWindow();
	}
	else if (!pDocument->IsModified())
	{
		pDocument->SetModifiedFlag(wasModified);
	}
	else
	{
		SetDefaultTitle(pDocument);
		pDocument->OnNewDocument();
	}
	return FALSE;
}
