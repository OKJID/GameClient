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

// ============================================================================
// Basic integer types
// All guarded with #ifndef to coexist with d3d8_compat.h
// ============================================================================

#ifndef DWORD_DEFINED
#define DWORD_DEFINED
typedef unsigned long DWORD;
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
typedef long LONG;
#endif

#ifndef ULONG_DEFINED
#define ULONG_DEFINED
typedef unsigned long ULONG;
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

// ============================================================================
// HRESULT and COM basics
// ============================================================================

#ifndef HRESULT_DEFINED
#define HRESULT_DEFINED
typedef long HRESULT;
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

#endif // __APPLE__
