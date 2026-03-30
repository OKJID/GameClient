/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifdef __APPLE__

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <unistd.h>

#ifndef __forceinline
#define __forceinline inline __attribute__((always_inline))
#endif

#ifndef __int64
#define __int64 long long
#endif

#ifndef _int64
#define _int64 long long
#endif

#define ERROR_SUCCESS 0L
#define REG_SZ 1
#define REG_OPTION_NON_VOLATILE 0
#define KEY_READ 0x20019
#define KEY_WRITE 0x20006

// ============================================================================
// Basic integer types
// All guarded with #ifndef to coexist with d3d8_compat.h
// ============================================================================

#ifndef DWORD_DEFINED
#define DWORD_DEFINED
typedef uint32_t DWORD;
#endif

#ifndef UINT_DEFINED
#define UINT_DEFINED
typedef unsigned int UINT;
#endif

#ifndef INT_DEFINED
#define INT_DEFINED
typedef int INT;
#endif

#ifndef WORD_DEFINED
#define WORD_DEFINED
typedef unsigned short WORD;
#endif

#ifndef BYTE_DEFINED
#define BYTE_DEFINED
typedef unsigned char BYTE;
#endif

#ifndef BOOL_DEFINED
#define BOOL_DEFINED
typedef int BOOL;
#endif

#ifndef LONG_DEFINED
#define LONG_DEFINED
typedef int32_t LONG;
#endif

#ifndef ULONG_DEFINED
#define ULONG_DEFINED
typedef uint32_t ULONG;
#endif

typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef void* LPVOID;
typedef const char* LPCSTR;
typedef char* LPSTR;
typedef const wchar_t* LPCWSTR;
typedef wchar_t* LPWSTR;
typedef const void* LPCVOID;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

// ============================================================================
// Handle types
// ============================================================================

#ifndef HANDLE_DEFINED
#define HANDLE_DEFINED
typedef void* HANDLE;
#endif

#ifndef HWND_DEFINED
#define HWND_DEFINED
typedef void* HWND;
#endif

#ifndef HINSTANCE_DEFINED
#define HINSTANCE_DEFINED
typedef void* HINSTANCE;
#endif

typedef void* HMODULE;
typedef void* HICON;
typedef void* HCURSOR;
typedef void* HBRUSH;
typedef void* HMENU;
typedef void* HDC;
typedef void* HGLOBAL;
typedef void* HMONITOR;
typedef void* HKEY;
typedef void* HBITMAP;
typedef void* HFONT;
typedef void* HRGN;
typedef void* HGDIOBJ;

// ============================================================================
// HRESULT and COM basics
// ============================================================================

#ifndef HRESULT_DEFINED
#define HRESULT_DEFINED
typedef int32_t HRESULT;
#endif

#ifndef S_OK
#define S_OK      ((HRESULT)0)
#define S_FALSE   ((HRESULT)1)
#define E_FAIL    ((HRESULT)0x80004005L)
#define E_NOINTERFACE ((HRESULT)0x80004002L)
#define E_OUTOFMEMORY ((HRESULT)0x8007000EL)
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
#define FAILED(hr)    ((HRESULT)(hr) < 0)
#endif

// ============================================================================
// Window message types
// ============================================================================

typedef UINT WPARAM;
typedef LONG LPARAM;
typedef LONG LRESULT;

#ifndef _RECT_DEFINED
#define _RECT_DEFINED
typedef struct tagRECT {
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
} RECT;
#endif

typedef struct tagPOINT {
  LONG x;
  LONG y;
} POINT;

typedef struct tagSIZE {
  LONG cx;
  LONG cy;
} SIZE;

typedef RECT* LPRECT;
typedef const RECT* LPCRECT;

// ============================================================================
// MessageBox stubs
// ============================================================================

#ifndef MessageBox
#define MessageBoxA(hwnd, text, caption, type) printf("[MessageBox] %s: %s\n", (caption), (text))
#define MessageBox MessageBoxA
#endif

#define MB_OK           0x00000000
#define MB_OKCANCEL     0x00000001
#define MB_YESNO        0x00000004
#define MB_ICONERROR    0x00000010
#define MB_ICONWARNING  0x00000030
#define MB_ICONQUESTION 0x00000020

#define IDOK     1
#define IDCANCEL 2
#define IDYES    6
#define IDNO     7

// ============================================================================
// Misc Win32 constants used in shared code
// ============================================================================

#define MAX_PATH 260
#define INFINITE 0xFFFFFFFF
#define INVALID_HANDLE_VALUE ((HANDLE)(long long)-1)

#define CALLBACK
#define WINAPI
#define APIENTRY

typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);

#include <strings.h>
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define _wcsicmp wcscasecmp
#define _wcsnicmp wcsncasecmp
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define _strdup strdup
#define _snprintf snprintf
#define _vsnprintf vsnprintf

inline LONG RegOpenKeyExA(HKEY, LPCSTR, DWORD, DWORD, HKEY*) { return 1; }
inline LONG RegCreateKeyExA(HKEY, LPCSTR, DWORD, LPSTR, DWORD, DWORD, void*, HKEY*, DWORD*) { return 1; }
inline LONG RegQueryValueExA(HKEY, LPCSTR, DWORD*, DWORD*, BYTE*, DWORD*) { return 1; }
inline LONG RegSetValueExA(HKEY, LPCSTR, DWORD, DWORD, const BYTE*, DWORD) { return 1; }
inline LONG RegCloseKey(HKEY) { return 0; }

#define RegOpenKeyEx RegOpenKeyExA
#define RegCreateKeyEx RegCreateKeyExA
#define RegQueryValueEx RegQueryValueExA
#define RegSetValueEx RegSetValueExA

#define HKEY_LOCAL_MACHINE ((HKEY)(uintptr_t)0x80000002)
#define HKEY_CURRENT_USER  ((HKEY)(uintptr_t)0x80000001)

#define lstrcat strcat
#define lstrcpy strcpy
inline char* lstrcpyn(char* dst, const char* src, int n) {
    strncpy(dst, src, n - 1);
    dst[n - 1] = '\0';
    return dst;
}
#define lstrlen strlen
#define lstrcmp strcmp
#define lstrcmpi strcasecmp
#define wsprintf sprintf

#define _isnan isnan
inline char* strupr(char* s) {
    for (char* p = s; *p; ++p) *p = toupper((unsigned char)*p);
    return s;
}
#ifndef _STRLWR_DEFINED
#define _STRLWR_DEFINED
inline char* _strlwr(char* s) {
    for (char* p = s; *p; ++p) *p = tolower((unsigned char)*p);
    return s;
}
#endif

#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#define FILE_ATTRIBUTE_DIRECTORY 0x10
inline DWORD GetFileAttributes(LPCSTR) { return INVALID_FILE_ATTRIBUTES; }
inline DWORD GetFileAttributesA(LPCSTR p) { return GetFileAttributes(p); }
inline DWORD GetCurrentDirectoryA(DWORD n, LPSTR buf) {
    if (getcwd(buf, n)) return (DWORD)strlen(buf);
    return 0;
}
#define GetCurrentDirectory GetCurrentDirectoryA
#define GetFileAttributesA GetFileAttributes

typedef void* LPDISPATCH;

#define GMEM_FIXED 0x0000
inline void* GlobalAlloc(UINT, size_t size) { return malloc(size); }
inline void GlobalFree(void* p) { free(p); }

#define ZeroMemory(p, n) memset((p), 0, (n))
#define CopyMemory(d, s, n) memcpy((d), (s), (n))
inline int MulDiv(int a, int b, int c) { return (int)((long long)a * b / c); }

#pragma pack(push, 2)
typedef struct tagBITMAPFILEHEADER {
    WORD  bfType;
    DWORD bfSize;
    WORD  bfReserved1;
    WORD  bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER;
#pragma pack(pop)

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG  biWidth;
    LONG  biHeight;
    WORD  biPlanes;
    WORD  biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG  biXPelsPerMeter;
    LONG  biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[1];
} BITMAPINFO;

#define BI_RGB 0L
#define DIB_RGB_COLORS 0

#define FW_NORMAL 400
#define FW_BOLD   700
#define DEFAULT_CHARSET 1
#define OUT_DEFAULT_PRECIS 0
#define CLIP_DEFAULT_PRECIS 0
#define ANTIALIASED_QUALITY 4
#define VARIABLE_PITCH 2
#define ETO_OPAQUE 0x0002

typedef void* PAVIFILE;
typedef void* PAVISTREAM;
typedef struct { DWORD fccType; DWORD fccHandler; DWORD dwFlags; DWORD dwCaps; WORD wPriority; WORD wLanguage; DWORD dwScale; DWORD dwRate; DWORD dwStart; DWORD dwLength; DWORD dwInitialFrames; DWORD dwSuggestedBufferSize; DWORD dwQuality; DWORD dwSampleSize; RECT rcFrame; DWORD dwEditCount; DWORD dwFormatChangeCount; char szName[64]; } AVISTREAMINFO;

inline HFONT CreateFont(int,int,int,int,int,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPCSTR) { return nullptr; }
inline HDC GetDC(HWND) { return nullptr; }
inline int ReleaseDC(HWND, HDC) { return 0; }
inline HGDIOBJ SelectObject(HDC, HGDIOBJ) { return nullptr; }
inline BOOL DeleteObject(HGDIOBJ) { return 0; }
inline BOOL ExtTextOutW(HDC,int,int,UINT,const RECT*,const wchar_t*,UINT,const int*) { return 0; }
inline BOOL GetTextExtentPoint32W(HDC,const wchar_t*,int,void*) { return 0; }
inline void* CreateDIBSection(HDC,const BITMAPINFO*,UINT,void**,HANDLE,DWORD) { return nullptr; }
inline HBITMAP CreateCompatibleBitmap(HDC,int,int) { return nullptr; }
inline HDC CreateCompatibleDC(HDC) { return nullptr; }
inline BOOL DeleteDC(HDC) { return 0; }
inline int SetBkColor(HDC, DWORD) { return 0; }
inline int SetTextColor(HDC, DWORD) { return 0; }
inline int SetBkMode(HDC, int) { return 0; }
#define OPAQUE 2
#define TRANSPARENT 1
#define RGB(r,g,b) ((DWORD)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

#endif // __APPLE__

