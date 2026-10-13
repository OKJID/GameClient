#pragma once

#include "AfxView.h"

#include <vector>

struct __POSITION
{
};
typedef __POSITION* POSITION;

#define BEFORE_START_POSITION (reinterpret_cast<POSITION>(static_cast<intptr_t>(-1)))

POSITION AFXAPI AfxPositionFromIndex(size_t index, size_t count);
size_t AFXAPI AfxIndexFromPosition(POSITION position);

class CDocument : public CCmdTarget
{
	DECLARE_DYNAMIC(CDocument)

public:
	CDocument();
	~CDocument() override;

	const CString& GetTitle() const { return m_strTitle; }
	virtual void SetTitle(LPCSTR lpszTitle);
	const CString& GetPathName() const { return m_strPathName; }
	virtual void SetPathName(LPCSTR lpszPathName, BOOL bAddToMRU = TRUE);
	CDocTemplate* GetDocTemplate() const { return m_pDocTemplate; }
	virtual BOOL IsModified() { return m_bModified; }
	virtual void SetModifiedFlag(BOOL bModified = TRUE) { m_bModified = bModified; }

	virtual POSITION GetFirstViewPosition() const;
	virtual CView* GetNextView(POSITION& rPosition) const;
	void UpdateAllViews(CView* pSender, LPARAM lHint = 0L, CObject* pHint = nullptr);
	void AddView(CView* pView);
	void RemoveView(CView* pView);
	virtual void OnChangedViewList();
	virtual void UpdateFrameCounts();

	virtual void DeleteContents();
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCSTR lpszPathName);
	virtual void OnCloseDocument();
	virtual void ReportSaveLoadException(LPCSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault);
	virtual CFile* GetFile(LPCSTR lpszFileName, UINT nOpenFlags, CFileException* pError);
	virtual void ReleaseFile(CFile* pFile, BOOL bAbort);
	virtual BOOL CanCloseFrame(CFrameWnd* pFrame);
	virtual BOOL SaveModified();
	virtual void PreCloseFrame(CFrameWnd* pFrame);
	virtual BOOL DoSave(LPCSTR lpszPathName, BOOL bReplace = TRUE);
	virtual BOOL DoFileSave();
	virtual HACCEL GetDefaultAccelerator();

	BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) override;

	afx_msg void OnFileClose();
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveAs();

	CString m_strTitle;
	CString m_strPathName;
	CDocTemplate* m_pDocTemplate;
	std::vector<CView*> m_viewList;
	BOOL m_bModified;
	BOOL m_bAutoDelete;
	BOOL m_bEmbedded;

protected:
	DECLARE_MESSAGE_MAP()
};

class CDocTemplate : public CCmdTarget
{
	DECLARE_DYNAMIC(CDocTemplate)

protected:
	CDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass);

public:
	enum DocStringIndex
	{
		windowTitle,
		docName,
		fileNewName,
		filterName,
		filterExt,
		regFileTypeId,
		regFileTypeName,
	};

	enum Confidence
	{
		noAttempt,
		maybeAttemptForeign,
		maybeAttemptNative,
		yesAttemptForeign,
		yesAttemptNative,
		yesAlreadyOpen,
	};

	~CDocTemplate() override;

	virtual void LoadTemplate();
	virtual BOOL GetDocString(CString& rString, DocStringIndex index) const;
	virtual POSITION GetFirstDocPosition() const = 0;
	virtual CDocument* GetNextDoc(POSITION& rPos) const = 0;
	virtual void AddDocument(CDocument* pDoc);
	virtual void RemoveDocument(CDocument* pDoc);
	virtual CDocument* CreateNewDocument();
	virtual CFrameWnd* CreateNewFrame(CDocument* pDoc, CFrameWnd* pOther);
	virtual void InitialUpdateFrame(CFrameWnd* pFrame, CDocument* pDoc, BOOL bMakeVisible = TRUE);
	virtual BOOL SaveAllModified();
	virtual void CloseAllDocuments(BOOL bEndSession);
	virtual CDocument* OpenDocumentFile(LPCSTR lpszPathName, BOOL bMakeVisible = TRUE) = 0;
	virtual void SetDefaultTitle(CDocument* pDocument) = 0;
	virtual Confidence MatchDocType(LPCSTR lpszPathName, CDocument*& rpDocMatch);

	UINT m_nIDResource;
	CRuntimeClass* m_pDocClass;
	CRuntimeClass* m_pFrameClass;
	CRuntimeClass* m_pViewClass;
	CString m_strDocStrings;
	BOOL m_bAutoDelete;
};

class CSingleDocTemplate : public CDocTemplate
{
	DECLARE_DYNAMIC(CSingleDocTemplate)

public:
	CSingleDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass);
	~CSingleDocTemplate() override;

	void AddDocument(CDocument* pDoc) override;
	void RemoveDocument(CDocument* pDoc) override;
	POSITION GetFirstDocPosition() const override;
	CDocument* GetNextDoc(POSITION& rPos) const override;
	CDocument* OpenDocumentFile(LPCSTR lpszPathName, BOOL bMakeVisible = TRUE) override;
	void SetDefaultTitle(CDocument* pDocument) override;

protected:
	BOOL reopenDocument(CDocument* pDocument, CFrameWnd* pFrame, BOOL bCreated, LPCSTR lpszPathName, BOOL bMakeVisible);

	CDocument* m_pOnlyDoc;
};

BOOL AFXAPI AfxFullPath(LPSTR lpszPathOut, LPCSTR lpszFileIn);
UINT AFXAPI AfxGetFileTitle(LPCSTR lpszPathName, LPSTR lpszTitle, UINT nMax);
