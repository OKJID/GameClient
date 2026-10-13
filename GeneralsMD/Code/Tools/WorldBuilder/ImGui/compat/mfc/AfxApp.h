#pragma once

#include "AfxDocument.h"

class CPrintDialog;
class CWinApp;

class CCommandLineInfo : public CObject
{
public:
	enum
	{
		FileNew,
		FileOpen,
		FilePrint,
		FilePrintTo,
		FileDDE,
		AppUnregister,
		FileNothing = -1,
	};

	CCommandLineInfo();

	virtual void ParseParam(LPCSTR lpszParam, BOOL bFlag, BOOL bLast);

	BOOL m_bShowSplash;
	BOOL m_bRunEmbedded;
	BOOL m_bRunAutomated;
	int m_nShellCommand;
	CString m_strFileName;
	CString m_strPrinterName;
	CString m_strDriverName;
	CString m_strPortName;

protected:
	void ParseParamFlag(LPCSTR pszParam);
	void ParseParamNotFlag(LPCSTR pszParam);
	void ParseLast(BOOL bLast);
};

class CRecentFileList
{
public:
	CRecentFileList(UINT nStart, LPCSTR lpszSection, LPCSTR lpszEntryFormat, int nSize, int nMaxDispLen = 30);
	virtual ~CRecentFileList() = default;

	int GetSize() const { return static_cast<int>(m_arrNames.size()); }
	CString& operator[](int nIndex) { return m_arrNames[static_cast<size_t>(nIndex)]; }
	virtual void Remove(int nIndex);
	virtual void Add(LPCSTR lpszPathName);
	virtual BOOL GetDisplayName(CString& strName, int nIndex, LPCSTR lpszCurDir, int nCurDir, BOOL bAtLeastName = TRUE) const;
	virtual void UpdateMenu(CCmdUI* pCmdUI);
	virtual void ReadList();
	virtual void WriteList();

protected:
	CString entryName(int nIndex) const;
	void insertMenuItems(CCmdUI* pCmdUI);

	std::vector<CString> m_arrNames;
	CString m_strSectionName;
	CString m_strEntryFormat;
	UINT m_nStart;
	int m_nMaxDisplayLength;
	CString m_strOriginal;
};

class CDocManager : public CObject
{
	DECLARE_DYNAMIC(CDocManager)

public:
	CDocManager() = default;
	~CDocManager() override;

	virtual void AddDocTemplate(CDocTemplate* pTemplate);
	virtual POSITION GetFirstDocTemplatePosition() const;
	virtual CDocTemplate* GetNextDocTemplate(POSITION& pos) const;
	virtual BOOL DoPromptFileName(CString& fileName, UINT nIDSTitle, DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate);
	virtual CDocument* OpenDocumentFile(LPCSTR lpszFileName);
	virtual BOOL SaveAllModified();
	virtual void CloseAllDocuments(BOOL bEndSession);
	virtual int GetOpenDocumentCount();
	virtual void OnFileNew();
	virtual void OnFileOpen();

	std::vector<CDocTemplate*> m_templateList;

protected:
	CDocument* activateOpenDocument(CDocument* pDocument);
};

class CWinThread : public CCmdTarget
{
	DECLARE_DYNAMIC(CWinThread)

public:
	CWinThread();
	~CWinThread() override;

	virtual BOOL InitInstance();
	virtual int Run();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL PumpMessage();
	virtual BOOL OnIdle(LONG lCount);
	virtual BOOL IsIdleMessage(MSG* pMsg);
	virtual int ExitInstance();
	virtual CWnd* GetMainWnd();

	CWnd* m_pMainWnd;
	CWnd* m_pActiveWnd;
	MSG m_msgCur;

protected:
	void updateFrameCommandUI(CWnd* pMainWnd);

	CPoint m_ptCursorLast;
	UINT m_nMsgLast;
};

class CWinApp : public CWinThread
{
	DECLARE_DYNAMIC(CWinApp)

public:
	explicit CWinApp(LPCSTR lpszAppName = nullptr);
	~CWinApp() override;

	HINSTANCE m_hInstance;
	LPSTR m_lpCmdLine;
	int m_nCmdShow;
	LPCSTR m_pszAppName;
	LPCSTR m_pszExeName;
	LPCSTR m_pszProfileName;
	CDocManager* m_pDocManager;
	CRecentFileList* m_pRecentFileList;
	CCommandLineInfo* m_pCmdInfo;
	int m_nNumPreviewPages;

	BOOL InitInstance() override;
	int Run() override;
	BOOL OnIdle(LONG lCount) override;
	int ExitInstance() override;
	virtual BOOL InitApplication();
	virtual int DoMessageBox(LPCSTR lpszPrompt, UINT nType, UINT nIDPrompt);
	virtual BOOL SaveAllModified();
	virtual void AddToRecentFileList(LPCSTR lpszPathName);
	virtual CDocument* OpenDocumentFile(LPCSTR lpszFileName);
	virtual void DoWaitCursor(int nCode);

	void SetCurrentHandles();
	void HideApplication();
	void AddDocTemplate(CDocTemplate* pTemplate);
	POSITION GetFirstDocTemplatePosition() const;
	CDocTemplate* GetNextDocTemplate(POSITION& pos) const;
	BOOL DoPromptFileName(CString& fileName, UINT nIDSTitle, DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate);
	int DoPrintDialog(CPrintDialog* pPD);
	BOOL GetPrinterDeviceDefaults(PRINTDLG* pPrintDlg);
	void CloseAllDocuments(BOOL bEndSession);
	BOOL ProcessShellCommand(CCommandLineInfo& rCmdInfo);
	void ParseCommandLine(CCommandLineInfo& rCmdInfo);
	void LoadStdProfileSettings(UINT nMaxMRU = 4);
	void SaveStdProfileSettings();
	void EnableModeless(BOOL bEnable);
	BOOL Enable3dControls();
	BOOL Enable3dControlsStatic();

	UINT GetProfileInt(LPCSTR lpszSection, LPCSTR lpszEntry, int nDefault);
	BOOL WriteProfileInt(LPCSTR lpszSection, LPCSTR lpszEntry, int nValue);
	CString GetProfileString(LPCSTR lpszSection, LPCSTR lpszEntry, LPCSTR lpszDefault = nullptr);
	BOOL WriteProfileString(LPCSTR lpszSection, LPCSTR lpszEntry, LPCSTR lpszValue);

	HCURSOR LoadCursor(LPCSTR lpszResourceName) const;
	HCURSOR LoadCursor(UINT nIDResource) const;
	HCURSOR LoadStandardCursor(LPCSTR lpszCursorName) const;
	HICON LoadIcon(LPCSTR lpszResourceName) const;
	HICON LoadIcon(UINT nIDResource) const;
	HICON LoadStandardIcon(LPCSTR lpszIconName) const;

	afx_msg void OnAppExit();
	afx_msg void OnFileNew();
	afx_msg void OnFileOpen();
	afx_msg void OnFilePrintSetup();
	afx_msg void OnUpdateRecentFileMenu(CCmdUI* pCmdUI);
	afx_msg BOOL OnOpenRecentFile(UINT nID);

protected:
	DECLARE_MESSAGE_MAP()

	int m_nWaitCursorCount;
	HCURSOR m_hcurWaitCursorRestore;
};

CWinApp* AFXAPI AfxGetApp();
CWinThread* AFXAPI AfxGetThread();
CWnd* AFXAPI AfxGetMainWnd();
HINSTANCE AFXAPI AfxGetInstanceHandle();
HINSTANCE AFXAPI AfxGetResourceHandle();
LPCSTR AFXAPI AfxGetAppName();
void AFXAPI AfxPostQuitMessage(int nExitCode);
int AFXAPI AfxMessageBox(LPCSTR lpszText, UINT nType = MB_OK, UINT nIDHelp = 0);
int AFXAPI AfxMessageBox(UINT nIDPrompt, UINT nType = MB_OK, UINT nIDHelp = static_cast<UINT>(-1));
BOOL AFXAPI AfxWinInit(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow);
int AFXAPI AfxWinMain(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow);

extern int __argc;
extern char** __argv;
