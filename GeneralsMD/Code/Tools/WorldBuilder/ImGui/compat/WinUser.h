#pragma once

#include <cstdint>

#include "WinUserMessages.h"
#include "WinUserStyles.h"

#define OUT
#define IN

typedef uintptr_t UINT_PTR;
typedef intptr_t INT_PTR;
typedef intptr_t LONG_PTR;
typedef uintptr_t ULONG_PTR;
typedef uintptr_t DWORD_PTR;
typedef short SHORT;
typedef WORD ATOM;
typedef void* HACCEL;
typedef void* HDWP;
typedef char TCHAR;
typedef const char* LPCTSTR;
typedef char* LPTSTR;
typedef POINT* LPPOINT;
typedef SIZE* LPSIZE;
typedef int* LPINT;

#define _T(text) text
#define TEXT(text) text

#define MAKEINTRESOURCE(id) (reinterpret_cast<LPCTSTR>(static_cast<uintptr_t>(static_cast<WORD>(id))))
#define IS_INTRESOURCE(name) ((reinterpret_cast<uintptr_t>(name) >> 16) == 0)
#define LOWORD(value) (static_cast<WORD>(static_cast<DWORD_PTR>(value) & 0xffff))
#define HIWORD(value) (static_cast<WORD>((static_cast<DWORD_PTR>(value) >> 16) & 0xffff))
#define MAKELONG(low, high) (static_cast<LONG>((static_cast<WORD>(low)) | (static_cast<DWORD>(static_cast<WORD>(high))) << 16))
#define MAKEWPARAM(low, high) (static_cast<WPARAM>(static_cast<DWORD>(MAKELONG(low, high))))
#define MAKELPARAM(low, high) (static_cast<LPARAM>(static_cast<DWORD>(MAKELONG(low, high))))
#define MAKELRESULT(low, high) (static_cast<LRESULT>(static_cast<DWORD>(MAKELONG(low, high))))
#define GET_X_LPARAM(lp) (static_cast<int>(static_cast<short>(LOWORD(lp))))
#define GET_Y_LPARAM(lp) (static_cast<int>(static_cast<short>(HIWORD(lp))))
#define GET_WHEEL_DELTA_WPARAM(wParam) (static_cast<short>(HIWORD(wParam)))
#define WHEEL_DELTA 120

#define VK_CANCEL 0x03
#define VK_SELECT 0x29
#define VK_EXECUTE 0x2B
#define VK_NUMPAD0 0x60
#define VK_NUMPAD1 0x61
#define VK_NUMPAD2 0x62
#define VK_NUMPAD3 0x63
#define VK_NUMPAD4 0x64
#define VK_NUMPAD5 0x65
#define VK_NUMPAD6 0x66
#define VK_NUMPAD7 0x67
#define VK_NUMPAD8 0x68
#define VK_NUMPAD9 0x69
#define VK_MULTIPLY 0x6A
#define VK_ADD 0x6B
#define VK_SEPARATOR 0x6C
#define VK_SUBTRACT 0x6D
#define VK_DECIMAL 0x6E
#define VK_DIVIDE 0x6F
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#define VK_NUMLOCK 0x90
#define VK_SCROLL 0x91
#define VK_LSHIFT 0xA0
#define VK_RSHIFT 0xA1
#define VK_LCONTROL 0xA2
#define VK_RCONTROL 0xA3
#define VK_LMENU 0xA4
#define VK_RMENU 0xA5
#define VK_OEM_1 0xBA
#define VK_OEM_PLUS 0xBB
#define VK_OEM_COMMA 0xBC
#define VK_OEM_MINUS 0xBD
#define VK_OEM_PERIOD 0xBE
#define VK_OEM_2 0xBF
#define VK_OEM_3 0xC0
#define VK_OEM_4 0xDB
#define VK_OEM_5 0xDC
#define VK_OEM_6 0xDD
#define VK_OEM_7 0xDE

#define MK_LBUTTON 0x0001
#define MK_RBUTTON 0x0002
#define MK_SHIFT 0x0004
#define MK_CONTROL 0x0008
#define MK_MBUTTON 0x0010

#define HTERROR (-2)
#define HTTRANSPARENT (-1)
#define HTNOWHERE 0
#define HTCLIENT 1
#define HTCAPTION 2

#define MA_ACTIVATE 1
#define MA_ACTIVATEANDEAT 2
#define MA_NOACTIVATE 3
#define MA_NOACTIVATEANDEAT 4

#define SW_HIDE 0
#define SW_NORMAL 1
#define SW_SHOWMINIMIZED 2
#define SW_SHOWMAXIMIZED 3
#define SW_MAXIMIZE 3
#define SW_SHOWNOACTIVATE 4
#define SW_SHOW 5
#define SW_MINIMIZE 6
#define SW_SHOWMINNOACTIVE 7
#define SW_SHOWNA 8
#define SW_RESTORE 9
#define SW_SHOWDEFAULT 10

#define SW_PARENTCLOSING 1
#define SW_PARENTOPENING 3

#define MSGF_DIALOGBOX 0
#define MSGF_MESSAGEBOX 1
#define MSGF_MENU 2

#define SWP_NOSIZE 0x0001
#define SWP_NOMOVE 0x0002
#define SWP_NOZORDER 0x0004
#define SWP_NOREDRAW 0x0008
#define SWP_NOACTIVATE 0x0010
#define SWP_FRAMECHANGED 0x0020
#define SWP_SHOWWINDOW 0x0040
#define SWP_HIDEWINDOW 0x0080
#define SWP_NOCOPYBITS 0x0100
#define SWP_NOOWNERZORDER 0x0200
#define SWP_NOSENDCHANGING 0x0400
#define SWP_NOCLIENTSIZE 0x0800
#define SWP_NOCLIENTMOVE 0x1000

#define HWND_TOP (reinterpret_cast<HWND>(0))
#define HWND_BOTTOM (reinterpret_cast<HWND>(1))
#define HWND_TOPMOST (reinterpret_cast<HWND>(-1))
#define HWND_NOTOPMOST (reinterpret_cast<HWND>(-2))

#define GW_HWNDFIRST 0
#define GW_HWNDLAST 1
#define GW_HWNDNEXT 2
#define GW_HWNDPREV 3
#define GW_OWNER 4
#define GW_CHILD 5

#define GWL_STYLE (-16)
#define GWL_EXSTYLE (-20)
#define GWL_ID (-12)
#define GWLP_WNDPROC (-4)
#define GWLP_HINSTANCE (-6)
#define GWLP_HWNDPARENT (-8)
#define GWLP_USERDATA (-21)
#define GWLP_ID GWL_ID
#define GWL_WNDPROC GWLP_WNDPROC
#define GWL_USERDATA GWLP_USERDATA
#define DWLP_MSGRESULT 0
#define DWLP_DLGPROC (DWLP_MSGRESULT + static_cast<int>(sizeof(LRESULT)))
#define DWLP_USER (DWLP_DLGPROC + static_cast<int>(sizeof(void*)))

#define CW_USEDEFAULT (static_cast<int>(0x80000000))

#define CS_VREDRAW 0x0001
#define CS_HREDRAW 0x0002
#define CS_DBLCLKS 0x0008
#define CS_OWNDC 0x0020
#define CS_CLASSDC 0x0040
#define CS_PARENTDC 0x0080
#define CS_NOCLOSE 0x0200
#define CS_SAVEBITS 0x0800
#define CS_BYTEALIGNCLIENT 0x1000
#define CS_BYTEALIGNWINDOW 0x2000
#define CS_GLOBALCLASS 0x4000

#define SB_HORZ 0
#define SB_VERT 1
#define SB_CTL 2
#define SB_BOTH 3

#define SB_LINEUP 0
#define SB_LINELEFT 0
#define SB_LINEDOWN 1
#define SB_LINERIGHT 1
#define SB_PAGEUP 2
#define SB_PAGELEFT 2
#define SB_PAGEDOWN 3
#define SB_PAGERIGHT 3
#define SB_THUMBPOSITION 4
#define SB_THUMBTRACK 5
#define SB_TOP 6
#define SB_LEFT 6
#define SB_BOTTOM 7
#define SB_RIGHT 7
#define SB_ENDSCROLL 8

#define SIF_RANGE 0x0001
#define SIF_PAGE 0x0002
#define SIF_POS 0x0004
#define SIF_DISABLENOSCROLL 0x0008
#define SIF_TRACKPOS 0x0010
#define SIF_ALL (SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS)

#define RDW_INVALIDATE 0x0001
#define RDW_INTERNALPAINT 0x0002
#define RDW_ERASE 0x0004
#define RDW_VALIDATE 0x0008
#define RDW_NOINTERNALPAINT 0x0010
#define RDW_NOERASE 0x0020
#define RDW_NOCHILDREN 0x0040
#define RDW_ALLCHILDREN 0x0080
#define RDW_UPDATENOW 0x0100
#define RDW_ERASENOW 0x0200
#define RDW_FRAME 0x0400
#define RDW_NOFRAME 0x0800

#define PM_NOREMOVE 0x0000
#define PM_REMOVE 0x0001
#define PM_NOYIELD 0x0002

#define MF_INSERT 0x00000000L
#define MF_CHANGE 0x00000080L
#define MF_APPEND 0x00000100L
#define MF_DELETE 0x00000200L
#define MF_REMOVE 0x00001000L
#define MF_BYCOMMAND 0x00000000L
#define MF_BYPOSITION 0x00000400L
#define MF_SEPARATOR 0x00000800L
#define MF_ENABLED 0x00000000L
#define MF_GRAYED 0x00000001L
#define MF_DISABLED 0x00000002L
#define MF_UNCHECKED 0x00000000L
#define MF_CHECKED 0x00000008L
#define MF_STRING 0x00000000L
#define MF_BITMAP 0x00000004L
#define MF_OWNERDRAW 0x00000100L
#define MF_POPUP 0x00000010L
#define MF_MENUBARBREAK 0x00000020L
#define MF_MENUBREAK 0x00000040L
#define MF_HILITE 0x00000080L
#define MF_DEFAULT 0x00001000L

#define TPM_LEFTBUTTON 0x0000L
#define TPM_RIGHTBUTTON 0x0002L
#define TPM_LEFTALIGN 0x0000L
#define TPM_CENTERALIGN 0x0004L
#define TPM_RIGHTALIGN 0x0008L
#define TPM_TOPALIGN 0x0000L
#define TPM_VCENTERALIGN 0x0010L
#define TPM_BOTTOMALIGN 0x0020L
#define TPM_NONOTIFY 0x0080L
#define TPM_RETURNCMD 0x0100L

#define FVIRTKEY TRUE
#define FNOINVERT 0x02
#define FSHIFT 0x04
#define FCONTROL 0x08
#define FALT 0x10

#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
#define SM_CXVSCROLL 2
#define SM_CYHSCROLL 3
#define SM_CYCAPTION 4
#define SM_CXBORDER 5
#define SM_CYBORDER 6
#define SM_CXDLGFRAME 7
#define SM_CYDLGFRAME 8
#define SM_CXICON 11
#define SM_CYICON 12
#define SM_CXCURSOR 13
#define SM_CYCURSOR 14
#define SM_CYMENU 15
#define SM_CXFULLSCREEN 16
#define SM_CYFULLSCREEN 17
#define SM_CXFRAME 32
#define SM_CYFRAME 33
#define SM_CXDOUBLECLK 36
#define SM_CYDOUBLECLK 37
#define SM_CXEDGE 45
#define SM_CYEDGE 46
#define SM_CXSMICON 49
#define SM_CYSMICON 50
#define SM_CYSMCAPTION 51

#define COLOR_SCROLLBAR 0
#define COLOR_BACKGROUND 1
#define COLOR_ACTIVECAPTION 2
#define COLOR_INACTIVECAPTION 3
#define COLOR_MENU 4
#define COLOR_WINDOW 5
#define COLOR_WINDOWFRAME 6
#define COLOR_MENUTEXT 7
#define COLOR_WINDOWTEXT 8
#define COLOR_CAPTIONTEXT 9
#define COLOR_ACTIVEBORDER 10
#define COLOR_INACTIVEBORDER 11
#define COLOR_APPWORKSPACE 12
#define COLOR_HIGHLIGHT 13
#define COLOR_HIGHLIGHTTEXT 14
#define COLOR_BTNFACE 15
#define COLOR_BTNSHADOW 16
#define COLOR_GRAYTEXT 17
#define COLOR_BTNTEXT 18
#define COLOR_INACTIVECAPTIONTEXT 19
#define COLOR_BTNHIGHLIGHT 20
#define COLOR_3DDKSHADOW 21
#define COLOR_3DLIGHT 22
#define COLOR_INFOTEXT 23
#define COLOR_INFOBK 24
#define COLOR_HOTLIGHT 26
#define COLOR_GRADIENTACTIVECAPTION 27
#define COLOR_GRADIENTINACTIVECAPTION 28
#define COLOR_MENUHILIGHT 29
#define COLOR_MENUBAR 30
#define COLOR_3DFACE COLOR_BTNFACE
#define COLOR_3DSHADOW COLOR_BTNSHADOW
#define COLOR_3DHIGHLIGHT COLOR_BTNHIGHLIGHT
#define COLOR_3DHILIGHT COLOR_BTNHIGHLIGHT
#define COLOR_BTNHILIGHT COLOR_BTNHIGHLIGHT

#define IDC_ARROW MAKEINTRESOURCE(32512)
#define IDC_IBEAM MAKEINTRESOURCE(32513)
#define IDC_WAIT MAKEINTRESOURCE(32514)
#define IDC_CROSS MAKEINTRESOURCE(32515)
#define IDC_UPARROW MAKEINTRESOURCE(32516)
#define IDC_SIZENWSE MAKEINTRESOURCE(32642)
#define IDC_SIZENESW MAKEINTRESOURCE(32643)
#define IDC_SIZEWE MAKEINTRESOURCE(32644)
#define IDC_SIZENS MAKEINTRESOURCE(32645)
#define IDC_SIZEALL MAKEINTRESOURCE(32646)
#define IDC_NO MAKEINTRESOURCE(32648)
#define IDC_HAND MAKEINTRESOURCE(32649)
#define IDC_APPSTARTING MAKEINTRESOURCE(32650)

#define IDI_APPLICATION MAKEINTRESOURCE(32512)
#define IDI_HAND MAKEINTRESOURCE(32513)
#define IDI_QUESTION MAKEINTRESOURCE(32514)
#define IDI_EXCLAMATION MAKEINTRESOURCE(32515)
#define IDI_ASTERISK MAKEINTRESOURCE(32516)

#define IMAGE_BITMAP 0
#define IMAGE_ICON 1
#define IMAGE_CURSOR 2

#define LR_DEFAULTCOLOR 0x00000000
#define LR_MONOCHROME 0x00000001
#define LR_LOADFROMFILE 0x00000010
#define LR_LOADTRANSPARENT 0x00000020
#define LR_DEFAULTSIZE 0x00000040
#define LR_LOADMAP3DCOLORS 0x00001000
#define LR_CREATEDIBSECTION 0x00002000
#define LR_SHARED 0x00008000

#define DI_MASK 0x0001
#define DI_IMAGE 0x0002
#define DI_NORMAL 0x0003
#define DI_COMPAT 0x0004
#define DI_DEFAULTSIZE 0x0008

#define MB_ABORTRETRYIGNORE 0x00000002
#define MB_RETRYCANCEL 0x00000005
#define MB_ICONSTOP MB_ICONHAND
#define MB_ICONASTERISK MB_ICONINFORMATION
#define MB_DEFBUTTON1 0x00000000
#define MB_DEFBUTTON2 0x00000100
#define MB_DEFBUTTON3 0x00000200
#define MB_SYSTEMMODAL 0x00001000
#define MB_TASKMODAL 0x00002000
#define MB_TOPMOST 0x00040000
#define MB_SETFOREGROUND 0x00010000
#define MB_TYPEMASK 0x0000000F
#define MB_ICONMASK 0x000000F0

#define IDABORT 3
#define IDRETRY 4
#define IDIGNORE 5
#define IDCLOSE 8
#define IDHELP 9

typedef INT_PTR (*DLGPROC)(HWND, UINT, WPARAM, LPARAM);
typedef void (*TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);
typedef void* HHOOK;
typedef LRESULT (*HOOKPROC)(int code, WPARAM wParam, LPARAM lParam);

#define WH_CBT 5
#define HCBT_CREATEWND 3
#define HCBT_DESTROYWND 4

typedef struct tagWNDCLASSA
{
	UINT style;
	WNDPROC lpfnWndProc;
	int cbClsExtra;
	int cbWndExtra;
	HINSTANCE hInstance;
	HICON hIcon;
	HCURSOR hCursor;
	HBRUSH hbrBackground;
	LPCSTR lpszMenuName;
	LPCSTR lpszClassName;
} WNDCLASSA, WNDCLASS, *LPWNDCLASS;

typedef struct tagMSG
{
	HWND hwnd;
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
	DWORD time;
	POINT pt;
} MSG, *LPMSG;

typedef struct tagCREATESTRUCTA
{
	void* lpCreateParams;
	HINSTANCE hInstance;
	HMENU hMenu;
	HWND hwndParent;
	int cy;
	int cx;
	int y;
	int x;
	LONG style;
	LPCSTR lpszName;
	LPCSTR lpszClass;
	DWORD dwExStyle;
} CREATESTRUCTA, CREATESTRUCT, *LPCREATESTRUCT;

typedef struct tagCBT_CREATEWNDA
{
	LPCREATESTRUCT lpcs;
	HWND hwndInsertAfter;
} CBT_CREATEWNDA, CBT_CREATEWND, *LPCBT_CREATEWND;

typedef struct tagWINDOWPOS
{
	HWND hwnd;
	HWND hwndInsertAfter;
	int x;
	int y;
	int cx;
	int cy;
	UINT flags;
} WINDOWPOS, *LPWINDOWPOS, *PWINDOWPOS;

typedef struct tagNCCALCSIZE_PARAMS
{
	RECT rgrc[3];
	PWINDOWPOS lppos;
} NCCALCSIZE_PARAMS, *LPNCCALCSIZE_PARAMS;

typedef struct tagMINMAXINFO
{
	POINT ptReserved;
	POINT ptMaxSize;
	POINT ptMaxPosition;
	POINT ptMinTrackSize;
	POINT ptMaxTrackSize;
} MINMAXINFO, *LPMINMAXINFO;

typedef struct tagNMHDR
{
	HWND hwndFrom;
	UINT_PTR idFrom;
	UINT code;
} NMHDR, *LPNMHDR;

typedef struct tagACCEL
{
	BYTE fVirt;
	WORD key;
	WORD cmd;
} ACCEL, *LPACCEL;

typedef struct tagSCROLLINFO
{
	UINT cbSize;
	UINT fMask;
	int nMin;
	int nMax;
	UINT nPage;
	int nPos;
	int nTrackPos;
} SCROLLINFO, *LPSCROLLINFO;
typedef const SCROLLINFO* LPCSCROLLINFO;

ATOM RegisterClassA(const WNDCLASSA* windowClass);
BOOL UnregisterClassA(LPCSTR className, HINSTANCE instance);
BOOL GetClassInfoA(HINSTANCE instance, LPCSTR className, WNDCLASSA* windowClass);
int GetClassNameA(HWND hwnd, LPSTR className, int maxCount);
#define RegisterClass RegisterClassA
#define UnregisterClass UnregisterClassA
#define GetClassInfo GetClassInfoA
#define GetClassName GetClassNameA

HWND CreateWindowExA(DWORD exStyle, LPCSTR className, LPCSTR windowName, DWORD style, int x, int y, int width, int height,
	HWND parent, HMENU menu, HINSTANCE instance, void* param);
#define CreateWindowEx CreateWindowExA
#define CreateWindowA(className, windowName, style, x, y, width, height, parent, menu, instance, param) \
	CreateWindowExA(0, className, windowName, style, x, y, width, height, parent, menu, instance, param)
#define CreateWindow CreateWindowA
BOOL DestroyWindow(HWND hwnd);
BOOL IsWindow(HWND hwnd);
BOOL IsChild(HWND parent, HWND hwnd);
HWND GetParent(HWND hwnd);
HWND SetParent(HWND hwnd, HWND newParent);
HWND GetWindow(HWND hwnd, UINT command);
HWND GetTopWindow(HWND hwnd);
HWND GetDesktopWindow();
HWND WindowFromPoint(POINT point);
HWND ChildWindowFromPoint(HWND parent, POINT point);

BOOL ShowWindow(HWND hwnd, int command);
BOOL ShowOwnedPopups(HWND owner, BOOL show);
BOOL IsWindowVisible(HWND hwnd);
BOOL EnableWindow(HWND hwnd, BOOL enable);
BOOL IsWindowEnabled(HWND hwnd);
BOOL IsIconic(HWND hwnd);
BOOL IsZoomed(HWND hwnd);
BOOL SetWindowPos(HWND hwnd, HWND insertAfter, int x, int y, int width, int height, UINT flags);
BOOL MoveWindow(HWND hwnd, int x, int y, int width, int height, BOOL repaint);
HDWP BeginDeferWindowPos(int numWindows);
HDWP DeferWindowPos(HDWP positions, HWND hwnd, HWND insertAfter, int x, int y, int width, int height, UINT flags);
BOOL EndDeferWindowPos(HDWP positions);
BOOL BringWindowToTop(HWND hwnd);
BOOL GetWindowRect(HWND hwnd, LPRECT rect);
BOOL GetClientRect(HWND hwnd, LPRECT rect);
BOOL AdjustWindowRect(LPRECT rect, DWORD style, BOOL menu);
BOOL AdjustWindowRectEx(LPRECT rect, DWORD style, BOOL menu, DWORD exStyle);
BOOL ClientToScreen(HWND hwnd, LPPOINT point);
BOOL ScreenToClient(HWND hwnd, LPPOINT point);
int MapWindowPoints(HWND from, HWND to, LPPOINT points, UINT count);

BOOL SetWindowTextA(HWND hwnd, LPCSTR text);
int GetWindowTextA(HWND hwnd, LPSTR text, int maxCount);
int GetWindowTextLengthA(HWND hwnd);
#define SetWindowText SetWindowTextA
#define GetWindowText GetWindowTextA
#define GetWindowTextLength GetWindowTextLengthA

LONG GetWindowLongA(HWND hwnd, int index);
LONG SetWindowLongA(HWND hwnd, int index, LONG value);
LONG_PTR GetWindowLongPtrA(HWND hwnd, int index);
LONG_PTR SetWindowLongPtrA(HWND hwnd, int index, LONG_PTR value);
#define GetWindowLong GetWindowLongA
#define SetWindowLong SetWindowLongA
#define GetWindowLongPtr GetWindowLongPtrA
#define SetWindowLongPtr SetWindowLongPtrA

HWND SetFocus(HWND hwnd);
HWND GetFocus();
HWND SetActiveWindow(HWND hwnd);
HWND GetActiveWindow();
HWND GetLastActivePopup(HWND hwnd);
BOOL SetForegroundWindow(HWND hwnd);
HWND GetForegroundWindow();
HWND SetCapture(HWND hwnd);
BOOL ReleaseCapture();
HWND GetCapture();

BOOL InvalidateRect(HWND hwnd, const RECT* rect, BOOL erase);
BOOL ValidateRect(HWND hwnd, const RECT* rect);
BOOL InvalidateRgn(HWND hwnd, HRGN region, BOOL erase);
BOOL UpdateWindow(HWND hwnd);
BOOL RedrawWindow(HWND hwnd, const RECT* updateRect, HRGN updateRegion, UINT flags);
BOOL GetUpdateRect(HWND hwnd, LPRECT rect, BOOL erase);

UINT_PTR SetTimer(HWND hwnd, UINT_PTR id, UINT elapse, TIMERPROC timerProc);
BOOL KillTimer(HWND hwnd, UINT_PTR id);

HHOOK SetWindowsHookExA(int hookType, HOOKPROC hookProc, HINSTANCE instance, DWORD threadId);
BOOL UnhookWindowsHookEx(HHOOK hook);
LRESULT CallNextHookEx(HHOOK hook, int code, WPARAM wParam, LPARAM lParam);
#define SetWindowsHookEx SetWindowsHookExA

LRESULT SendMessageA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL PostMessageA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void PostQuitMessage(int exitCode);
BOOL PeekMessageA(LPMSG message, HWND hwnd, UINT filterMin, UINT filterMax, UINT removeFlags);
BOOL GetMessageA(LPMSG message, HWND hwnd, UINT filterMin, UINT filterMax);
BOOL TranslateMessage(const MSG* message);
LRESULT DispatchMessageA(const MSG* message);
BOOL WaitMessage();
DWORD GetMessageTime();
LRESULT CallWindowProcA(WNDPROC windowProc, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT DefWindowProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
#define SendMessage SendMessageA
#define PostMessage PostMessageA
#define PeekMessage PeekMessageA
#define GetMessage GetMessageA
#define DispatchMessage DispatchMessageA
#define CallWindowProc CallWindowProcA
#define DefWindowProc DefWindowProcA

HWND CreateDialogParamA(HINSTANCE instance, LPCSTR templateName, HWND parent, DLGPROC dialogProc, LPARAM initParam);
INT_PTR DialogBoxParamA(HINSTANCE instance, LPCSTR templateName, HWND parent, DLGPROC dialogProc, LPARAM initParam);
BOOL EndDialog(HWND dialog, INT_PTR result);
LRESULT DefDlgProcA(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam);
BOOL IsDialogMessageA(HWND dialog, LPMSG message);
BOOL MapDialogRect(HWND dialog, LPRECT rect);
LONG GetDialogBaseUnits();
HWND GetDlgItem(HWND dialog, int id);
int GetDlgCtrlID(HWND hwnd);
UINT GetDlgItemTextA(HWND dialog, int id, LPSTR text, int maxCount);
BOOL SetDlgItemTextA(HWND dialog, int id, LPCSTR text);
UINT GetDlgItemInt(HWND dialog, int id, BOOL* translated, BOOL isSigned);
BOOL SetDlgItemInt(HWND dialog, int id, UINT value, BOOL isSigned);
BOOL CheckDlgButton(HWND dialog, int id, UINT check);
UINT IsDlgButtonChecked(HWND dialog, int id);
BOOL CheckRadioButton(HWND dialog, int firstId, int lastId, int checkId);
LRESULT SendDlgItemMessageA(HWND dialog, int id, UINT message, WPARAM wParam, LPARAM lParam);
HWND GetNextDlgTabItem(HWND dialog, HWND control, BOOL previous);
HWND GetNextDlgGroupItem(HWND dialog, HWND control, BOOL previous);
#define CreateDialogParam CreateDialogParamA
#define DialogBoxParam DialogBoxParamA
#define DefDlgProc DefDlgProcA
#define IsDialogMessage IsDialogMessageA
#define GetDlgItemText GetDlgItemTextA
#define SetDlgItemText SetDlgItemTextA
#define SendDlgItemMessage SendDlgItemMessageA

HMENU CreateMenu();
HMENU CreatePopupMenu();
BOOL DestroyMenu(HMENU menu);
HMENU LoadMenuA(HINSTANCE instance, LPCSTR menuName);
HMENU GetMenu(HWND hwnd);
BOOL SetMenu(HWND hwnd, HMENU menu);
BOOL DrawMenuBar(HWND hwnd);
HMENU GetSubMenu(HMENU menu, int position);
int GetMenuItemCount(HMENU menu);
UINT GetMenuItemID(HMENU menu, int position);
UINT GetMenuState(HMENU menu, UINT id, UINT flags);
int GetMenuStringA(HMENU menu, UINT id, LPSTR text, int maxCount, UINT flags);
BOOL AppendMenuA(HMENU menu, UINT flags, UINT_PTR idOrPopup, LPCSTR text);
BOOL InsertMenuA(HMENU menu, UINT position, UINT flags, UINT_PTR idOrPopup, LPCSTR text);
BOOL ModifyMenuA(HMENU menu, UINT position, UINT flags, UINT_PTR idOrPopup, LPCSTR text);
BOOL DeleteMenu(HMENU menu, UINT position, UINT flags);
BOOL RemoveMenu(HMENU menu, UINT position, UINT flags);
DWORD CheckMenuItem(HMENU menu, UINT id, UINT check);
BOOL EnableMenuItem(HMENU menu, UINT id, UINT enable);
BOOL CheckMenuRadioItem(HMENU menu, UINT first, UINT last, UINT check, UINT flags);
BOOL TrackPopupMenu(HMENU menu, UINT flags, int x, int y, int reserved, HWND owner, const RECT* rect);
#define LoadMenu LoadMenuA
#define GetMenuString GetMenuStringA

int LoadStringA(HINSTANCE instance, UINT id, LPSTR buffer, int bufferMax);
#define LoadString LoadStringA
#define AppendMenu AppendMenuA
#define InsertMenu InsertMenuA
#define ModifyMenu ModifyMenuA

HACCEL LoadAcceleratorsA(HINSTANCE instance, LPCSTR tableName);
int TranslateAcceleratorA(HWND hwnd, HACCEL table, LPMSG message);
BOOL DestroyAcceleratorTable(HACCEL table);
#define LoadAccelerators LoadAcceleratorsA
#define TranslateAccelerator TranslateAcceleratorA

SHORT GetAsyncKeyState(int virtualKey);
SHORT GetKeyState(int virtualKey);
BOOL GetCursorPos(LPPOINT point);
HCURSOR LoadCursorA(HINSTANCE instance, LPCSTR cursorName);
HCURSOR SetCursor(HCURSOR cursor);
HCURSOR GetCursor();
BOOL DestroyCursor(HCURSOR cursor);
int ShowCursor(BOOL show);
HICON LoadIconA(HINSTANCE instance, LPCSTR iconName);
BOOL DestroyIcon(HICON icon);
HANDLE LoadImageA(HINSTANCE instance, LPCSTR name, UINT type, int width, int height, UINT flags);
BOOL DrawIconEx(HDC hdc, int x, int y, HICON icon, int width, int height, UINT step, HBRUSH flickerFreeBrush, UINT flags);
#define LoadCursor LoadCursorA
#define LoadIcon LoadIconA
#define LoadImage LoadImageA

int GetSystemMetrics(int index);
DWORD GetSysColor(int index);
HBRUSH GetSysColorBrush(int index);
BOOL MessageBeep(UINT type);
BOOL Beep(DWORD frequency, DWORD duration);

int SetScrollPos(HWND hwnd, int bar, int position, BOOL redraw);
int GetScrollPos(HWND hwnd, int bar);
BOOL SetScrollRange(HWND hwnd, int bar, int minimum, int maximum, BOOL redraw);
BOOL GetScrollRange(HWND hwnd, int bar, int* minimum, int* maximum);
int SetScrollInfo(HWND hwnd, int bar, LPCSCROLLINFO info, BOOL redraw);
BOOL GetScrollInfo(HWND hwnd, int bar, LPSCROLLINFO info);
BOOL ScrollWindow(HWND hwnd, int dx, int dy, const RECT* scrollRect, const RECT* clipRect);

BOOL SetRect(LPRECT rect, int left, int top, int right, int bottom);
BOOL SetRectEmpty(LPRECT rect);
BOOL CopyRect(LPRECT destination, const RECT* source);
BOOL InflateRect(LPRECT rect, int dx, int dy);
BOOL OffsetRect(LPRECT rect, int dx, int dy);
BOOL IntersectRect(LPRECT destination, const RECT* first, const RECT* second);
BOOL UnionRect(LPRECT destination, const RECT* first, const RECT* second);
BOOL EqualRect(const RECT* first, const RECT* second);
BOOL IsRectEmpty(const RECT* rect);
BOOL PtInRect(const RECT* rect, POINT point);
