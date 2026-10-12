#pragma once

#define IN
#define afx_msg
#define DECLARE_MESSAGE_MAP()
#define DECLARE_DYNCREATE(theClass)
#define DECLARE_DYNAMIC(theClass)
#define IMPLEMENT_DYNCREATE(theClass, baseClass)
#define IMPLEMENT_DYNAMIC(theClass, baseClass)
#define BEGIN_MESSAGE_MAP(theClass, baseClass)
#define END_MESSAGE_MAP()
#define ON_BN_CLICKED(...)
#define ON_CBN_CLOSEUP(...)
#define ON_CBN_EDITCHANGE(...)
#define ON_CBN_KILLFOCUS(...)
#define ON_CBN_SELCHANGE(...)
#define ON_CBN_SELENDOK(...)
#define ON_COMMAND(...)
#define ON_COMMAND_RANGE(...)
#define ON_EN_CHANGE(...)
#define ON_EN_KILLFOCUS(...)
#define ON_EN_UPDATE(...)
#define ON_LBN_DBLCLK(...)
#define ON_LBN_SELCHANGE(...)
#define ON_NOTIFY(...)
#define ON_UPDATE_COMMAND_UI(...)
#define ON_WM_CANCELMODE()
#define ON_WM_CLOSE()
#define ON_WM_CREATE()
#define ON_WM_DESTROY()
#define ON_WM_ERASEBKGND()
#define ON_WM_HSCROLL()
#define ON_WM_KEYDOWN()
#define ON_WM_KEYUP()
#define ON_WM_LBUTTONDOWN()
#define ON_WM_LBUTTONUP()
#define ON_WM_MBUTTONDOWN()
#define ON_WM_MBUTTONUP()
#define ON_WM_MOUSEMOVE()
#define ON_WM_MOUSEWHEEL()
#define ON_WM_MOVE()
#define ON_WM_PAINT()
#define ON_WM_RBUTTONDOWN()
#define ON_WM_RBUTTONUP()
#define ON_WM_SETCURSOR()
#define ON_WM_SHOWWINDOW()
#define ON_WM_SIZE()
#define ON_WM_TIMER()
#define ON_WM_VSCROLL()

typedef void* HTREEITEM;
typedef void* POSITION;
typedef DWORD COLORREF;

struct NMHDR {};
struct CREATESTRUCT {};
typedef CREATESTRUCT* LPCREATESTRUCT;
struct CHARRANGE {};
struct AFX_CMDHANDLERINFO {};

class CArchive {};
class CDumpContext {};
class CDataExchange {};
class CCmdUI {};
class CPrintInfo {};
class CCreateContext {};
class CDocTemplate {};
class CSingleDocTemplate : public CDocTemplate {};
class CFile {};

class CDC {};
class CPaintDC : public CDC {};
class CClientDC : public CDC {};
class CGdiObject {};
class CBrush : public CGdiObject {};
class CPen : public CGdiObject {};
class CFont : public CGdiObject {};
class CBitmap : public CGdiObject {};
class CRgn : public CGdiObject {};
class CImageList {};
class CMenu {};

class CObject
{
public:
	virtual ~CObject() {}
	virtual void AssertValid() const {}
	virtual void Dump(CDumpContext& dc) const {}
	virtual void Serialize(CArchive& ar) {}
};

class CCmdTarget : public CObject
{
public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) { return FALSE; }
};

class CWnd : public CCmdTarget
{
public:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam) { return FALSE; }
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) { return FALSE; }
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) { return 0; }
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs) { return TRUE; }
	virtual void PostNcDestroy() {}
	virtual void DoDataExchange(CDataExchange* pDX) {}
	virtual BOOL DestroyWindow() { return TRUE; }
};

class CDialog : public CWnd
{
public:
	virtual BOOL OnInitDialog() { return TRUE; }
	virtual void OnOK() {}
	virtual void OnCancel() {}
};

class CPropertyPage : public CDialog
{
public:
	virtual BOOL OnSetActive() { return TRUE; }
	virtual BOOL OnKillActive() { return TRUE; }
};

class CPropertySheet : public CWnd {};
class CDialogBar : public CWnd {};
class CToolBar : public CWnd {};
class CStatusBar : public CWnd {};

class CFrameWnd : public CWnd
{
public:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) { return TRUE; }
};

class CMDIFrameWnd : public CFrameWnd {};

class CView : public CWnd
{
public:
	virtual void OnDraw(CDC* pDC) {}
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo) { return TRUE; }
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo) {}
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo) {}
	virtual void OnInitialUpdate() {}
	virtual void OnActivateFrame(UINT nState, CFrameWnd* pFrameWnd) {}
};

class CDocument : public CCmdTarget
{
public:
	virtual BOOL OnNewDocument() { return TRUE; }
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName) { return TRUE; }
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName) { return TRUE; }
	virtual BOOL DoSave(LPCTSTR lpszPathName, BOOL bReplace = TRUE) { return TRUE; }
	virtual BOOL DoFileSave() { return TRUE; }
	virtual void DeleteContents() {}
};

class CWinApp : public CCmdTarget
{
public:
	virtual BOOL InitInstance() { return TRUE; }
	virtual int ExitInstance() { return 0; }
	virtual BOOL OnIdle(LONG lCount) { return FALSE; }
};

class CButton : public CWnd {};
class CComboBox : public CWnd {};
class CEdit : public CWnd {};
class CListBox : public CWnd {};
class CTreeCtrl : public CWnd {};
class CListCtrl : public CWnd {};
class CSliderCtrl : public CWnd {};
class CScrollBar : public CWnd {};
class CRichEditCtrl : public CWnd {};
class CStatic : public CWnd {};
class CProgressCtrl : public CWnd {};
class CSpinButtonCtrl : public CWnd {};
class CTabCtrl : public CWnd {};
