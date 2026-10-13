#pragma once

#include "AfxWnd.h"

class CResourceException : public CException
{
	DECLARE_DYNAMIC(CResourceException)
};

void AFXAPI AfxThrowResourceException();

class CGdiObject : public CObject
{
	DECLARE_DYNCREATE(CGdiObject)

public:
	CGdiObject();
	~CGdiObject() override;

	HGDIOBJ m_hObject;

	operator HGDIOBJ() const { return m_hObject; }
	HGDIOBJ GetSafeHandle() const;

	static CGdiObject* PASCAL FromHandle(HGDIOBJ hObject);
	static void PASCAL DeleteTempMap();
	BOOL Attach(HGDIOBJ hObject);
	HGDIOBJ Detach();
	BOOL DeleteObject();
	int GetObject(int nCount, LPVOID lpObject) const;
	BOOL CreateStockObject(int nIndex);

	BOOL operator==(const CGdiObject& obj) const { return obj.m_hObject == m_hObject; }
	BOOL operator!=(const CGdiObject& obj) const { return obj.m_hObject != m_hObject; }
};

class CPen : public CGdiObject
{
	DECLARE_DYNAMIC(CPen)

public:
	CPen() = default;
	CPen(int nPenStyle, int nWidth, COLORREF crColor);

	static CPen* PASCAL FromHandle(HPEN hPen) { return static_cast<CPen*>(CGdiObject::FromHandle(hPen)); }
	operator HPEN() const { return static_cast<HPEN>(m_hObject); }

	BOOL CreatePen(int nPenStyle, int nWidth, COLORREF crColor);
	BOOL CreatePenIndirect(LPLOGPEN lpLogPen);
	int GetLogPen(LOGPEN* pLogPen) { return GetObject(sizeof(LOGPEN), pLogPen); }
};

class CBrush : public CGdiObject
{
	DECLARE_DYNAMIC(CBrush)

public:
	CBrush() = default;
	explicit CBrush(COLORREF crColor);

	static CBrush* PASCAL FromHandle(HBRUSH hBrush) { return static_cast<CBrush*>(CGdiObject::FromHandle(hBrush)); }
	operator HBRUSH() const { return static_cast<HBRUSH>(m_hObject); }

	BOOL CreateSolidBrush(COLORREF crColor);
	BOOL CreateBrushIndirect(const LOGBRUSH* lpLogBrush);
	BOOL CreateSysColorBrush(int nIndex);
	int GetLogBrush(LOGBRUSH* pLogBrush) { return GetObject(sizeof(LOGBRUSH), pLogBrush); }
};

class CFont : public CGdiObject
{
	DECLARE_DYNAMIC(CFont)

public:
	static CFont* PASCAL FromHandle(HFONT hFont) { return static_cast<CFont*>(CGdiObject::FromHandle(hFont)); }
	operator HFONT() const { return static_cast<HFONT>(m_hObject); }

	BOOL CreateFontIndirect(const LOGFONTA* lpLogFont);
	BOOL CreateFont(int nHeight, int nWidth, int nEscapement, int nOrientation, int nWeight, BYTE bItalic, BYTE bUnderline,
		BYTE cStrikeOut, BYTE nCharSet, BYTE nOutPrecision, BYTE nClipPrecision, BYTE nQuality, BYTE nPitchAndFamily, LPCSTR lpszFacename);
	BOOL CreatePointFont(int nPointSize, LPCSTR lpszFaceName, CDC* pDC = nullptr);
	int GetLogFont(LOGFONTA* pLogFont) { return GetObject(sizeof(LOGFONTA), pLogFont); }
};

class CBitmap : public CGdiObject
{
	DECLARE_DYNAMIC(CBitmap)

public:
	static CBitmap* PASCAL FromHandle(HBITMAP hBitmap) { return static_cast<CBitmap*>(CGdiObject::FromHandle(hBitmap)); }
	operator HBITMAP() const { return static_cast<HBITMAP>(m_hObject); }

	BOOL LoadBitmap(LPCSTR lpszResourceName);
	BOOL LoadBitmap(UINT nIDResource);
	BOOL CreateBitmap(int nWidth, int nHeight, UINT nPlanes, UINT nBitcount, const void* lpBits);
	int GetBitmap(BITMAP* pBitMap) { return GetObject(sizeof(BITMAP), pBitMap); }
};

class CRgn : public CGdiObject
{
	DECLARE_DYNAMIC(CRgn)

public:
	static CRgn* PASCAL FromHandle(HRGN hRgn) { return static_cast<CRgn*>(CGdiObject::FromHandle(hRgn)); }
	operator HRGN() const { return static_cast<HRGN>(m_hObject); }

	BOOL CreateRectRgn(int x1, int y1, int x2, int y2);
	BOOL CreateRectRgnIndirect(LPCRECT lpRect);
	void SetRectRgn(int x1, int y1, int x2, int y2) { ::SetRectRgn(static_cast<HRGN>(m_hObject), x1, y1, x2, y2); }
	void SetRectRgn(LPCRECT lpRect) { SetRectRgn(lpRect->left, lpRect->top, lpRect->right, lpRect->bottom); }
	int CombineRgn(CRgn* pRgn1, CRgn* pRgn2, int nCombineMode);
	int CopyRgn(CRgn* pRgnSrc);
	int OffsetRgn(int x, int y) { return ::OffsetRgn(static_cast<HRGN>(m_hObject), x, y); }
	int OffsetRgn(POINT point) { return OffsetRgn(point.x, point.y); }
	int GetRgnBox(LPRECT lpRect) const { return ::GetRgnBox(static_cast<HRGN>(m_hObject), lpRect); }
	BOOL PtInRegion(int x, int y) const { return ::PtInRegion(static_cast<HRGN>(m_hObject), x, y); }
	BOOL PtInRegion(POINT point) const { return PtInRegion(point.x, point.y); }
	BOOL RectInRegion(LPCRECT lpRect) const { return ::RectInRegion(static_cast<HRGN>(m_hObject), lpRect); }
};

class CDC : public CObject
{
	DECLARE_DYNCREATE(CDC)

public:
	CDC();
	~CDC() override;

	HDC m_hDC;
	HDC m_hAttribDC;

	operator HDC() const { return m_hDC; }
	HDC GetSafeHdc() const;

	static CDC* PASCAL FromHandle(HDC hDC);
	static void PASCAL DeleteTempMap();
	BOOL Attach(HDC hDC);
	HDC Detach();

	virtual int SaveDC();
	virtual BOOL RestoreDC(int nSavedDC);

	CPen* SelectObject(CPen* pPen);
	CBrush* SelectObject(CBrush* pBrush);
	virtual CFont* SelectObject(CFont* pFont);
	CBitmap* SelectObject(CBitmap* pBitmap);
	int SelectObject(CRgn* pRgn);
	CGdiObject* SelectObject(CGdiObject* pObject);
	virtual CGdiObject* SelectStockObject(int nIndex);

	virtual COLORREF SetBkColor(COLORREF crColor);
	virtual COLORREF SetTextColor(COLORREF crColor);
	int SetBkMode(int nBkMode);
	int SetROP2(int nDrawMode);
	int SetStretchBltMode(int nStretchMode);

	virtual CPoint SetViewportOrg(int x, int y);
	CPoint SetViewportOrg(POINT point) { return SetViewportOrg(point.x, point.y); }
	CPoint GetViewportOrg() const;
	virtual CPoint OffsetViewportOrg(int nWidth, int nHeight);

	CPoint MoveTo(int x, int y);
	CPoint MoveTo(POINT point) { return MoveTo(point.x, point.y); }
	BOOL LineTo(int x, int y);
	BOOL LineTo(POINT point) { return LineTo(point.x, point.y); }
	BOOL Polyline(const POINT* lpPoints, int nCount);
	BOOL Polygon(const POINT* lpPoints, int nCount);
	BOOL Rectangle(int x1, int y1, int x2, int y2);
	BOOL Rectangle(LPCRECT lpRect) { return Rectangle(lpRect->left, lpRect->top, lpRect->right, lpRect->bottom); }
	BOOL Ellipse(int x1, int y1, int x2, int y2);
	BOOL Ellipse(LPCRECT lpRect) { return Ellipse(lpRect->left, lpRect->top, lpRect->right, lpRect->bottom); }
	void FillRect(LPCRECT lpRect, CBrush* pBrush);
	void FrameRect(LPCRECT lpRect, CBrush* pBrush);
	void FillSolidRect(LPCRECT lpRect, COLORREF clr);
	void FillSolidRect(int x, int y, int cx, int cy, COLORREF clr);
	void Draw3dRect(LPCRECT lpRect, COLORREF clrTopLeft, COLORREF clrBottomRight);
	void Draw3dRect(int x, int y, int cx, int cy, COLORREF clrTopLeft, COLORREF clrBottomRight);
	COLORREF SetPixel(int x, int y, COLORREF crColor);
	COLORREF SetPixel(POINT point, COLORREF crColor) { return SetPixel(point.x, point.y, crColor); }

	virtual BOOL TextOut(int x, int y, LPCSTR lpszString, int nCount);
	BOOL TextOut(int x, int y, const CString& str) { return TextOut(x, y, str, str.GetLength()); }
	virtual BOOL ExtTextOut(int x, int y, UINT nOptions, LPCRECT lpRect, LPCSTR lpszString, UINT nCount, LPINT lpDxWidths);
	virtual int DrawText(LPCSTR lpszString, int nCount, LPRECT lpRect, UINT nFormat);
	int DrawText(const CString& str, LPRECT lpRect, UINT nFormat) { return DrawText(str, str.GetLength(), lpRect, nFormat); }
	CSize GetTextExtent(LPCSTR lpszString, int nCount) const;
	CSize GetTextExtent(const CString& str) const { return GetTextExtent(str, str.GetLength()); }
	BOOL GetTextMetrics(LPTEXTMETRICA lpMetrics) const;

	virtual int SelectClipRgn(CRgn* pRgn);
	virtual int GetClipBox(LPRECT lpRect) const;
	int IntersectClipRect(int x1, int y1, int x2, int y2);
	int IntersectClipRect(LPCRECT lpRect);
	int StretchDIBits(int x, int y, int nWidth, int nHeight, int xSrc, int ySrc, int nSrcWidth, int nSrcHeight, const void* lpBits,
		const BITMAPINFO* lpBitsInfo, UINT nUsage, DWORD dwRop);
};

class CPaintDC : public CDC
{
	DECLARE_DYNAMIC(CPaintDC)

public:
	explicit CPaintDC(CWnd* pWnd);
	~CPaintDC() override;

	PAINTSTRUCT m_ps;

protected:
	HWND m_hWnd;
};

class CClientDC : public CDC
{
	DECLARE_DYNAMIC(CClientDC)

public:
	explicit CClientDC(CWnd* pWnd);
	~CClientDC() override;

protected:
	HWND m_hWnd;
};

class CWindowDC : public CDC
{
	DECLARE_DYNAMIC(CWindowDC)

public:
	explicit CWindowDC(CWnd* pWnd);
	~CWindowDC() override;

protected:
	HWND m_hWnd;
};
