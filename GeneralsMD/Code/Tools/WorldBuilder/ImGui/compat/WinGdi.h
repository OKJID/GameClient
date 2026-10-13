#pragma once

typedef void* HPEN;

#define PS_SOLID 0
#define PS_DASH 1
#define PS_DOT 2
#define PS_DASHDOT 3
#define PS_DASHDOTDOT 4
#define PS_NULL 5
#define PS_INSIDEFRAME 6

#define BS_SOLID 0
#define BS_NULL 1
#define BS_HOLLOW BS_NULL

#define WHITE_BRUSH 0
#define LTGRAY_BRUSH 1
#define GRAY_BRUSH 2
#define DKGRAY_BRUSH 3
#define BLACK_BRUSH 4
#define NULL_BRUSH 5
#define HOLLOW_BRUSH NULL_BRUSH
#define WHITE_PEN 6
#define BLACK_PEN 7
#define NULL_PEN 8
#define OEM_FIXED_FONT 10
#define ANSI_FIXED_FONT 11
#define ANSI_VAR_FONT 12
#define SYSTEM_FONT 13
#define DEVICE_DEFAULT_FONT 14
#define SYSTEM_FIXED_FONT 16
#define DEFAULT_GUI_FONT 17

#define FW_DONTCARE 0
#define FW_THIN 100
#define FW_LIGHT 300
#define FW_REGULAR FW_NORMAL
#define FW_MEDIUM 500
#define FW_SEMIBOLD 600
#define FW_HEAVY 900
#define ANSI_CHARSET 0
#define DEFAULT_QUALITY 0
#define DEFAULT_PITCH 0
#define FIXED_PITCH 1
#define FF_DONTCARE 0
#define FF_SWISS (2 << 4)
#define LF_FACESIZE 32

#define DT_TOP 0x00000000
#define DT_LEFT 0x00000000
#define DT_CENTER 0x00000001
#define DT_RIGHT 0x00000002
#define DT_VCENTER 0x00000004
#define DT_BOTTOM 0x00000008
#define DT_WORDBREAK 0x00000010
#define DT_SINGLELINE 0x00000020
#define DT_EXPANDTABS 0x00000040
#define DT_NOCLIP 0x00000100
#define DT_CALCRECT 0x00000400
#define DT_NOPREFIX 0x00000800
#define DT_END_ELLIPSIS 0x00008000

#define SRCCOPY 0x00CC0020
#define SRCPAINT 0x00EE0086
#define SRCAND 0x008800C6
#define SRCINVERT 0x00660046
#define PATCOPY 0x00F00021
#define BLACKNESS 0x00000042
#define WHITENESS 0x00FF0062

#define R2_NOT 6
#define R2_XORPEN 7
#define R2_COPYPEN 13

#define ERROR 0
#define NULLREGION 1
#define SIMPLEREGION 2
#define COMPLEXREGION 3

#define RGN_AND 1
#define RGN_OR 2
#define RGN_XOR 3
#define RGN_DIFF 4
#define RGN_COPY 5

#define COLORONCOLOR 3
#define HALFTONE 4

#define CLR_INVALID 0xFFFFFFFF

#define ETO_CLIPPED 0x0004

typedef struct tagLOGFONTA
{
	LONG lfHeight;
	LONG lfWidth;
	LONG lfEscapement;
	LONG lfOrientation;
	LONG lfWeight;
	BYTE lfItalic;
	BYTE lfUnderline;
	BYTE lfStrikeOut;
	BYTE lfCharSet;
	BYTE lfOutPrecision;
	BYTE lfClipPrecision;
	BYTE lfQuality;
	BYTE lfPitchAndFamily;
	char lfFaceName[LF_FACESIZE];
} LOGFONTA, LOGFONT, *LPLOGFONT;

typedef struct tagLOGPEN
{
	UINT lopnStyle;
	POINT lopnWidth;
	COLORREF lopnColor;
} LOGPEN, *LPLOGPEN;

typedef struct tagLOGBRUSH
{
	UINT lbStyle;
	COLORREF lbColor;
	ULONG_PTR lbHatch;
} LOGBRUSH, *LPLOGBRUSH;

typedef struct tagBITMAP
{
	LONG bmType;
	LONG bmWidth;
	LONG bmHeight;
	LONG bmWidthBytes;
	WORD bmPlanes;
	WORD bmBitsPixel;
	void* bmBits;
} BITMAP, *LPBITMAP;

typedef struct tagPAINTSTRUCT
{
	HDC hdc;
	BOOL fErase;
	RECT rcPaint;
	BOOL fRestore;
	BOOL fIncUpdate;
	BYTE rgbReserved[32];
} PAINTSTRUCT, *LPPAINTSTRUCT;

typedef BITMAPINFO* LPBITMAPINFO;
typedef BITMAPINFOHEADER* LPBITMAPINFOHEADER;

HDC BeginPaint(HWND hwnd, LPPAINTSTRUCT paint);
BOOL EndPaint(HWND hwnd, const PAINTSTRUCT* paint);
HDC GetWindowDC(HWND hwnd);

HPEN CreatePen(int style, int width, COLORREF color);
HPEN CreatePenIndirect(const LOGPEN* pen);
HBRUSH CreateSolidBrush(COLORREF color);
HBRUSH CreateBrushIndirect(const LOGBRUSH* brush);
HFONT CreateFontIndirectA(const LOGFONTA* font);
HGDIOBJ GetStockObject(int index);
int GetObjectA(HGDIOBJ object, int size, void* buffer);
HBITMAP CreateBitmap(int width, int height, UINT planes, UINT bitsPerPixel, const void* bits);
#define CreateFontIndirect CreateFontIndirectA
#define GetObject GetObjectA

int SaveDC(HDC hdc);
BOOL RestoreDC(HDC hdc, int savedDC);
int SetROP2(HDC hdc, int mode);
int SetStretchBltMode(HDC hdc, int mode);
BOOL SetViewportOrgEx(HDC hdc, int x, int y, LPPOINT previous);
BOOL GetViewportOrgEx(HDC hdc, LPPOINT origin);
BOOL OffsetViewportOrgEx(HDC hdc, int dx, int dy, LPPOINT previous);

BOOL MoveToEx(HDC hdc, int x, int y, LPPOINT previous);
BOOL LineTo(HDC hdc, int x, int y);
BOOL Polyline(HDC hdc, const POINT* points, int count);
BOOL Polygon(HDC hdc, const POINT* points, int count);
BOOL Rectangle(HDC hdc, int left, int top, int right, int bottom);
BOOL Ellipse(HDC hdc, int left, int top, int right, int bottom);
int FrameRect(HDC hdc, const RECT* rect, HBRUSH brush);
int FillRect(HDC hdc, const RECT* rect, HBRUSH brush);
COLORREF SetPixel(HDC hdc, int x, int y, COLORREF color);

BOOL TextOutA(HDC hdc, int x, int y, LPCSTR text, int length);
int DrawTextA(HDC hdc, LPCSTR text, int length, LPRECT rect, UINT format);
BOOL ExtTextOutA(HDC hdc, int x, int y, UINT options, const RECT* rect, LPCSTR text, UINT length, const int* dx);
BOOL GetTextExtentPoint32A(HDC hdc, LPCSTR text, int length, LPSIZE size);
BOOL GetTextMetricsA(HDC hdc, LPTEXTMETRICA metrics);
#define TextOut TextOutA
#define DrawText DrawTextA
#define ExtTextOut ExtTextOutA
#define GetTextExtentPoint32 GetTextExtentPoint32A
#define GetTextMetrics GetTextMetricsA

int StretchDIBits(HDC hdc, int destX, int destY, int destWidth, int destHeight, int sourceX, int sourceY, int sourceWidth, int sourceHeight,
	const void* bits, const BITMAPINFO* info, UINT usage, DWORD rop);

HRGN CreateRectRgn(int left, int top, int right, int bottom);
HRGN CreateRectRgnIndirect(const RECT* rect);
int CombineRgn(HRGN destination, HRGN first, HRGN second, int mode);
int OffsetRgn(HRGN region, int dx, int dy);
int GetRgnBox(HRGN region, LPRECT rect);
BOOL PtInRegion(HRGN region, int x, int y);
BOOL RectInRegion(HRGN region, const RECT* rect);
BOOL SetRectRgn(HRGN region, int left, int top, int right, int bottom);
int SelectClipRgn(HDC hdc, HRGN region);
int GetClipBox(HDC hdc, LPRECT rect);
int IntersectClipRect(HDC hdc, int left, int top, int right, int bottom);
int GetUpdateRgn(HWND hwnd, HRGN region, BOOL erase);
