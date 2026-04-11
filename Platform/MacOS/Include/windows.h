/*
**  macOS Shadow Header: windows.h
**  Provides Win32 type definitions for macOS compilation.
**  Shared code includes <windows.h> — on macOS this file is found via include path.
*/

#pragma once

#ifdef __APPLE__

#define __AIFF__

#include <Utility/CppMacros.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <cwctype>
#include <cmath>
#include <ctime>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <mach-o/dyld.h>

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
#define REG_DWORD 4
#define REG_OPTION_NON_VOLATILE 0
#define KEY_READ 0x20019
#define KEY_WRITE 0x20006

#define VK_LBUTTON 0x01
#define VK_RBUTTON 0x02
#define VK_MBUTTON 0x04
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_CLEAR 0x0C
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_PAUSE 0x13
#define VK_CAPITAL 0x14
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_PRINT 0x2A
#define VK_SNAPSHOT 0x2C
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_HELP 0x2F
#define VK_LWIN 0x5B
#define VK_RWIN 0x5C

// ============================================================================
// Basic integer types
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
#ifdef __OBJC__
#include <objc/objc.h>
#else
typedef int BOOL;
#endif
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
// Misc Win32 constants
// ============================================================================

#define MAX_PATH 260
#define _MAX_PATH MAX_PATH
#define INFINITE 0xFFFFFFFF
#define INVALID_HANDLE_VALUE ((HANDLE)(long long)-1)

#define CSIDL_PERSONAL 0x0005

#define _stat stat
#define _S_IFDIR S_IFDIR

typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME;

inline DWORD GetDoubleClickTime() { return 500; }

inline BOOL SHGetSpecialFolderPath(HWND, char* pszPath, int, BOOL) {
    const char* home = getenv("HOME");
    if (home && pszPath) {
        strncpy(pszPath, home, MAX_PATH - 1);
        pszPath[MAX_PATH - 1] = '\0';
        return TRUE;
    }
    return FALSE;
}

inline BOOL CreateDirectory(LPCSTR lpPathName, void*) {
    return mkdir(lpPathName, 0755) == 0 || errno == EEXIST;
}

inline DWORD GetModuleFileName(HMODULE, char* lpFilename, DWORD nSize) {
    uint32_t bufsize = nSize;
    if (_NSGetExecutablePath(lpFilename, &bufsize) == 0)
        return (DWORD)strlen(lpFilename);
    return 0;
}

extern char MacOSCommandLineString[4096];
inline LPCSTR GetCommandLineA() {
    return MacOSCommandLineString;
}

#ifndef MAKE_HRESULT
#define SEVERITY_ERROR 1
#define FACILITY_ITF 4
#define MAKE_HRESULT(sev,fac,code) \
    ((HRESULT)(((unsigned long)(sev)<<31)|((unsigned long)(fac)<<16)|((unsigned long)(code))))
#endif

#define CALLBACK
#define WINAPI
#define APIENTRY
#ifndef CONST
#define CONST const
#endif

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
#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}

inline char* itoa(int value, char* str, int base) {
    if (base == 16) {
        snprintf(str, 33, "%x", value);
    } else if (base == 8) {
        snprintf(str, 33, "%o", value);
    } else {
        snprintf(str, 33, "%d", value);
    }
    return str;
}

#define _P_NOWAIT 0
inline int _spawnl(int mode, const char* cmdname, const char* arg0, ...) { return -1; }

#define GetLastError() 0
#define FORMAT_MESSAGE_FROM_SYSTEM 0x00001000
inline DWORD FormatMessage(DWORD, const void*, DWORD, DWORD, char* lpBuffer, DWORD nSize, va_list*) {
    if (lpBuffer && nSize > 0) lpBuffer[0] = '\0';
    return 0;
}
inline DWORD FormatMessageW(DWORD, const void*, DWORD, DWORD, wchar_t* lpBuffer, DWORD nSize, va_list*) {
    if (lpBuffer && nSize > 0) lpBuffer[0] = L'\0';
    return 0;
}

typedef void* LPITEMIDLIST;
#define CSIDL_DESKTOPDIRECTORY 0x0010
inline BOOL SHGetSpecialFolderLocation(void*, int, LPITEMIDLIST*) { return FALSE; }
inline BOOL SHGetPathFromIDList(LPITEMIDLIST, char*) { return FALSE; }
#include <unistd.h>
inline BOOL DeleteFile(const char* lpFileName) { return unlink(lpFileName) == 0; }

typedef struct _OSVERSIONINFOA {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    char  szCSDVersion[128];
} OSVERSIONINFOA, *POSVERSIONINFOA, *LPOSVERSIONINFOA;
typedef OSVERSIONINFOA OSVERSIONINFO;
#define VER_PLATFORM_WIN32_WINDOWS 1
inline int GetVersionEx(OSVERSIONINFO* os) { 
	if(os) { 
		os->dwMajorVersion = 5; 
		os->dwMinorVersion = 1; 
		os->dwPlatformId = VER_PLATFORM_WIN32_WINDOWS;
	}
	return 1; 
}

#define SW_SHOWNORMAL 1

#define LOCALE_SYSTEM_DEFAULT 0x0800
typedef void* HINSTANCE;
inline HINSTANCE ShellExecuteA(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, INT) { return (HINSTANCE)33; }
typedef struct _WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    char cFileName[260];
} WIN32_FIND_DATAA, *PWIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;
typedef WIN32_FIND_DATAA WIN32_FIND_DATA;
inline HANDLE FindFirstFile(const char* lpFileName, WIN32_FIND_DATA* lpFindFileData) { return INVALID_HANDLE_VALUE; }
inline BOOL FindNextFile(HANDLE hFindFile, WIN32_FIND_DATA* lpFindFileData) { return FALSE; }
inline BOOL FindClose(HANDLE hFindFile) { return TRUE; }

typedef int (*FARPROC)();
inline void* LoadLibrary(const char* lpFileName) { return NULL; }
inline FARPROC GetProcAddress(void* hModule, const char* lpProcName) { return NULL; }
inline BOOL FreeLibrary(void* hModule) { return TRUE; }

inline int GetDateFormat(DWORD, DWORD, const void*, const char* format, char* dateStr, int cchDate) {
    if (dateStr && cchDate > 0) {
        if (strcmp(format, "yyyy") == 0) strncpy(dateStr, "2025", cchDate);
        else if (strcmp(format, "MM") == 0) strncpy(dateStr, "01", cchDate);
        else if (strcmp(format, "dd") == 0) strncpy(dateStr, "01", cchDate);
        else strncpy(dateStr, "01", cchDate);
        dateStr[cchDate-1] = '\0';
    }
    return 1;
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

typedef struct tagTEXTMETRICA {
  long tmHeight;
  long tmAscent;
  long tmDescent;
  long tmInternalLeading;
  long tmExternalLeading;
  long tmAveCharWidth;
  long tmMaxCharWidth;
  long tmWeight;
  long tmOverhang;
  long tmDigitizedAspectX;
  long tmDigitizedAspectY;
  char tmFirstChar;
  char tmLastChar;
  char tmDefaultChar;
  char tmBreakChar;
  unsigned char tmItalic;
  unsigned char tmUnderlined;
  unsigned char tmStruckOut;
  unsigned char tmPitchAndFamily;
  unsigned char tmCharSet;
} TEXTMETRICA, *PTEXTMETRICA, *NPTEXTMETRICA, *LPTEXTMETRICA;
typedef TEXTMETRICA TEXTMETRIC;
typedef LPTEXTMETRICA LPTEXTMETRIC;
inline BOOL GetTextMetrics(HDC hdc, LPTEXTMETRIC lptm) {
    if (lptm) memset(lptm, 0, sizeof(TEXTMETRICA));
    return 1;
}

// ============================================================================
// MSVC intrinsics / CRT
// ============================================================================

#ifndef __max
#define __max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef __min
#define __min(a,b) (((a) < (b)) ? (a) : (b))
#endif

inline void __debugbreak() {}

// ============================================================================
// Time
// ============================================================================

inline void GetLocalTime(SYSTEMTIME* st) {
    if (!st) return;
    time_t t = time(nullptr);
    struct tm tm_local;
    localtime_r(&t, &tm_local);
    st->wYear = (WORD)(tm_local.tm_year + 1900);
    st->wMonth = (WORD)(tm_local.tm_mon + 1);
    st->wDayOfWeek = (WORD)tm_local.tm_wday;
    st->wDay = (WORD)tm_local.tm_mday;
    st->wHour = (WORD)tm_local.tm_hour;
    st->wMinute = (WORD)tm_local.tm_min;
    st->wSecond = (WORD)tm_local.tm_sec;
    st->wMilliseconds = 0;
}

inline void GetSystemTime(SYSTEMTIME* st) { GetLocalTime(st); }

// ============================================================================
// File operations
// ============================================================================

inline BOOL CopyFile(LPCSTR src, LPCSTR dst, BOOL failIfExists) {
    char pSrc[MAX_PATH];
    char pDst[MAX_PATH];
    strncpy(pSrc, src, MAX_PATH - 1); pSrc[MAX_PATH - 1] = '\0';
    strncpy(pDst, dst, MAX_PATH - 1); pDst[MAX_PATH - 1] = '\0';
    for(char* p = pSrc; *p; ++p) if(*p == '\\') *p = '/';
    for(char* p = pDst; *p; ++p) if(*p == '\\') *p = '/';

    if (failIfExists) {
        struct stat st;
        if (::stat(pDst, &st) == 0) return FALSE;
    }
    FILE* in = fopen(pSrc, "rb");
    if (!in) return FALSE;
    FILE* out = fopen(pDst, "wb");
    if (!out) { fclose(in); return FALSE; }
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) fwrite(buf, 1, n, out);
    fclose(in);
    fclose(out);
    return TRUE;
}

// ============================================================================
// Threading
// ============================================================================

#include <pthread.h>

typedef pthread_mutex_t CRITICAL_SECTION;

inline void InitializeCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(cs, &attr);
    pthread_mutexattr_destroy(&attr);
}
inline void DeleteCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutex_destroy(cs);
}
inline void EnterCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutex_lock(cs);
}
inline void LeaveCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutex_unlock(cs);
}

#endif // __APPLE__
