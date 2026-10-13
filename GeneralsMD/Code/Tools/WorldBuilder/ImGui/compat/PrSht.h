#pragma once

#include "WinUser.h"

#define PSP_DEFAULT 0x00000000
#define PSP_DLGINDIRECT 0x00000001
#define PSP_USEHICON 0x00000002
#define PSP_USEICONID 0x00000004
#define PSP_USETITLE 0x00000008
#define PSP_HASHELP 0x00000020
#define PSP_USEREFPARENT 0x00000040
#define PSP_USECALLBACK 0x00000080
#define PSP_PREMATURE 0x00000400

#define PSPCB_RELEASE 1
#define PSPCB_CREATE 2

#define PSH_DEFAULT 0x00000000
#define PSH_PROPTITLE 0x00000001
#define PSH_USEHICON 0x00000002
#define PSH_USEICONID 0x00000004
#define PSH_PROPSHEETPAGE 0x00000008
#define PSH_WIZARDHASFINISH 0x00000010
#define PSH_WIZARD 0x00000020
#define PSH_USEPSTARTPAGE 0x00000040
#define PSH_NOAPPLYNOW 0x00000080
#define PSH_USECALLBACK 0x00000100
#define PSH_HASHELP 0x00000200
#define PSH_MODELESS 0x00000400

#define PSCB_INITIALIZED 1
#define PSCB_PRECREATE 2

#define PSN_FIRST (0U - 200U)
#define PSN_SETACTIVE (PSN_FIRST - 0)
#define PSN_KILLACTIVE (PSN_FIRST - 1)
#define PSN_APPLY (PSN_FIRST - 2)
#define PSN_RESET (PSN_FIRST - 3)
#define PSN_HELP (PSN_FIRST - 5)
#define PSN_WIZBACK (PSN_FIRST - 6)
#define PSN_WIZNEXT (PSN_FIRST - 7)
#define PSN_WIZFINISH (PSN_FIRST - 8)
#define PSN_QUERYCANCEL (PSN_FIRST - 9)

#define PSNRET_NOERROR 0
#define PSNRET_INVALID 1
#define PSNRET_INVALID_NOCHANGEPAGE 2

#define PSM_SETCURSEL (WM_USER + 101)
#define PSM_REMOVEPAGE (WM_USER + 102)
#define PSM_ADDPAGE (WM_USER + 103)
#define PSM_CHANGED (WM_USER + 104)
#define PSM_CANCELTOCLOSE (WM_USER + 107)
#define PSM_QUERYSIBLINGS (WM_USER + 108)
#define PSM_UNCHANGED (WM_USER + 109)
#define PSM_APPLY (WM_USER + 110)
#define PSM_SETTITLEA (WM_USER + 111)
#define PSM_PRESSBUTTON (WM_USER + 113)
#define PSM_GETTABCONTROL (WM_USER + 116)
#define PSM_ISDIALOGMESSAGE (WM_USER + 117)
#define PSM_GETCURRENTPAGEHWND (WM_USER + 118)
#define PSM_SETTITLE PSM_SETTITLEA

#define PSBTN_BACK 0
#define PSBTN_NEXT 1
#define PSBTN_FINISH 2
#define PSBTN_OK 3
#define PSBTN_APPLYNOW 4
#define PSBTN_CANCEL 5
#define PSBTN_HELP 6

#define ID_APPLY_NOW 0x3021
#define ID_WIZBACK 0x3023
#define ID_WIZNEXT 0x3024
#define ID_WIZFINISH 0x3025

struct _PROPSHEETPAGEA;
typedef struct _PSP* HPROPSHEETPAGE;
typedef UINT (CALLBACK* LPFNPSPCALLBACKA)(HWND hwnd, UINT message, struct _PROPSHEETPAGEA* page);
typedef int (CALLBACK* PFNPROPSHEETCALLBACK)(HWND hwnd, UINT message, LPARAM lParam);

typedef struct _PROPSHEETPAGEA
{
	DWORD dwSize;
	DWORD dwFlags;
	HINSTANCE hInstance;
	LPCSTR pszTemplate;
	HICON hIcon;
	LPCSTR pszTitle;
	DLGPROC pfnDlgProc;
	LPARAM lParam;
	LPFNPSPCALLBACKA pfnCallback;
	UINT* pcRefParent;
} PROPSHEETPAGEA, PROPSHEETPAGE, *LPPROPSHEETPAGEA;
typedef const PROPSHEETPAGEA* LPCPROPSHEETPAGEA;

typedef struct _PROPSHEETHEADERA
{
	DWORD dwSize;
	DWORD dwFlags;
	HWND hwndParent;
	HINSTANCE hInstance;
	HICON hIcon;
	LPCSTR pszCaption;
	UINT nPages;
	UINT nStartPage;
	LPCPROPSHEETPAGEA ppsp;
	PFNPROPSHEETCALLBACK pfnCallback;
} PROPSHEETHEADERA, PROPSHEETHEADER, *LPPROPSHEETHEADERA;
typedef const PROPSHEETHEADERA* LPCPROPSHEETHEADERA;

typedef struct _PSHNOTIFY
{
	NMHDR hdr;
	LPARAM lParam;
} PSHNOTIFY, *LPPSHNOTIFY;

INT_PTR PropertySheetA(LPCPROPSHEETHEADERA header);
HPROPSHEETPAGE CreatePropertySheetPageA(LPCPROPSHEETPAGEA page);
BOOL DestroyPropertySheetPage(HPROPSHEETPAGE page);
#define PropertySheet PropertySheetA
#define CreatePropertySheetPage CreatePropertySheetPageA

#define PropSheet_GetCurrentPageHwnd(hDlg) reinterpret_cast<HWND>(::SendMessageA(hDlg, PSM_GETCURRENTPAGEHWND, 0, 0))
#define PropSheet_SetCurSel(hDlg, hpage, index) ::SendMessageA(hDlg, PSM_SETCURSEL, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(hpage))
#define PropSheet_Changed(hDlg, hwnd) ::SendMessageA(hDlg, PSM_CHANGED, reinterpret_cast<WPARAM>(hwnd), 0)
#define PropSheet_UnChanged(hDlg, hwnd) ::SendMessageA(hDlg, PSM_UNCHANGED, reinterpret_cast<WPARAM>(hwnd), 0)
#define PropSheet_CancelToClose(hDlg) ::SendMessageA(hDlg, PSM_CANCELTOCLOSE, 0, 0)
#define PropSheet_QuerySiblings(hDlg, wParam, lParam) ::SendMessageA(hDlg, PSM_QUERYSIBLINGS, wParam, lParam)
#define PropSheet_PressButton(hDlg, button) ::SendMessageA(hDlg, PSM_PRESSBUTTON, static_cast<WPARAM>(button), 0)
#define PropSheet_SetTitle(hDlg, style, title) ::SendMessageA(hDlg, PSM_SETTITLEA, static_cast<WPARAM>(style), reinterpret_cast<LPARAM>(title))
#define PropSheet_GetTabControl(hDlg) reinterpret_cast<HWND>(::SendMessageA(hDlg, PSM_GETTABCONTROL, 0, 0))
