#include "AfxInternal.h"

#include <cstdlib>
#include <cstring>

IMPLEMENT_DYNAMIC(CResourceException, CException)
IMPLEMENT_DYNCREATE(CGdiObject, CObject)
IMPLEMENT_DYNAMIC(CPen, CGdiObject)
IMPLEMENT_DYNAMIC(CBrush, CGdiObject)
IMPLEMENT_DYNAMIC(CFont, CGdiObject)
IMPLEMENT_DYNAMIC(CBitmap, CGdiObject)
IMPLEMENT_DYNAMIC(CRgn, CGdiObject)
IMPLEMENT_DYNCREATE(CDC, CObject)
IMPLEMENT_DYNAMIC(CPaintDC, CDC)
IMPLEMENT_DYNAMIC(CClientDC, CDC)
IMPLEMENT_DYNAMIC(CWindowDC, CDC)

namespace
{

const int kLogicalPixelsPerInch = 96;
const int kDecipointsPerInch = 720;

}

void AFXAPI AfxThrowResourceException()
{
	throw new CResourceException;
}

CGdiObject::CGdiObject() :
	m_hObject(nullptr)
{
}

CGdiObject::~CGdiObject()
{
	DeleteObject();
}

HGDIOBJ CGdiObject::GetSafeHandle() const
{
	const CGdiObject* self = this;
	return self != nullptr ? m_hObject : nullptr;
}

CGdiObject* PASCAL CGdiObject::FromHandle(HGDIOBJ hObject)
{
	return AfxGdiObjectMap().fromHandle(hObject);
}

void PASCAL CGdiObject::DeleteTempMap()
{
	AfxGdiObjectMap().deleteTemporaries();
}

BOOL CGdiObject::Attach(HGDIOBJ hObject)
{
	if (hObject == nullptr)
	{
		return FALSE;
	}
	m_hObject = hObject;
	AfxGdiObjectMap().setPermanent(hObject, this);
	return TRUE;
}

HGDIOBJ CGdiObject::Detach()
{
	const HGDIOBJ hObject = m_hObject;
	if (hObject != nullptr)
	{
		AfxGdiObjectMap().removeHandle(hObject);
	}
	m_hObject = nullptr;
	return hObject;
}

BOOL CGdiObject::DeleteObject()
{
	if (m_hObject == nullptr)
	{
		return FALSE;
	}
	return ::DeleteObject(Detach());
}

int CGdiObject::GetObject(int nCount, LPVOID lpObject) const
{
	return ::GetObjectA(m_hObject, nCount, lpObject);
}

BOOL CGdiObject::CreateStockObject(int nIndex)
{
	return Attach(::GetStockObject(nIndex));
}

CPen::CPen(int nPenStyle, int nWidth, COLORREF crColor)
{
	if (!CreatePen(nPenStyle, nWidth, crColor))
	{
		AfxThrowResourceException();
	}
}

BOOL CPen::CreatePen(int nPenStyle, int nWidth, COLORREF crColor)
{
	return Attach(::CreatePen(nPenStyle, nWidth, crColor));
}

BOOL CPen::CreatePenIndirect(LPLOGPEN lpLogPen)
{
	return Attach(::CreatePenIndirect(lpLogPen));
}

CBrush::CBrush(COLORREF crColor)
{
	if (!CreateSolidBrush(crColor))
	{
		AfxThrowResourceException();
	}
}

BOOL CBrush::CreateSolidBrush(COLORREF crColor)
{
	return Attach(::CreateSolidBrush(crColor));
}

BOOL CBrush::CreateBrushIndirect(const LOGBRUSH* lpLogBrush)
{
	return Attach(::CreateBrushIndirect(lpLogBrush));
}

BOOL CBrush::CreateSysColorBrush(int nIndex)
{
	return Attach(::GetSysColorBrush(nIndex));
}

BOOL CFont::CreateFontIndirect(const LOGFONTA* lpLogFont)
{
	return Attach(::CreateFontIndirectA(lpLogFont));
}

BOOL CFont::CreateFont(int nHeight, int nWidth, int nEscapement, int nOrientation, int nWeight, BYTE bItalic, BYTE bUnderline,
	BYTE cStrikeOut, BYTE nCharSet, BYTE nOutPrecision, BYTE nClipPrecision, BYTE nQuality, BYTE nPitchAndFamily, LPCSTR lpszFacename)
{
	LOGFONTA logFont = {};
	logFont.lfHeight = nHeight;
	logFont.lfWidth = nWidth;
	logFont.lfEscapement = nEscapement;
	logFont.lfOrientation = nOrientation;
	logFont.lfWeight = nWeight;
	logFont.lfItalic = bItalic;
	logFont.lfUnderline = bUnderline;
	logFont.lfStrikeOut = cStrikeOut;
	logFont.lfCharSet = nCharSet;
	logFont.lfOutPrecision = nOutPrecision;
	logFont.lfClipPrecision = nClipPrecision;
	logFont.lfQuality = nQuality;
	logFont.lfPitchAndFamily = nPitchAndFamily;
	strlcpy(logFont.lfFaceName, lpszFacename != nullptr ? lpszFacename : "", sizeof(logFont.lfFaceName));
	return CreateFontIndirect(&logFont);
}

BOOL CFont::CreatePointFont(int nPointSize, LPCSTR lpszFaceName, CDC*)
{
	LOGFONTA logFont = {};
	logFont.lfCharSet = DEFAULT_CHARSET;
	logFont.lfHeight = -std::abs(nPointSize * kLogicalPixelsPerInch / kDecipointsPerInch);
	strlcpy(logFont.lfFaceName, lpszFaceName != nullptr ? lpszFaceName : "", sizeof(logFont.lfFaceName));
	return CreateFontIndirect(&logFont);
}

BOOL CBitmap::LoadBitmap(LPCSTR lpszResourceName)
{
	return Attach(::LoadImageA(AfxGetResourceHandle(), lpszResourceName, IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR));
}

BOOL CBitmap::LoadBitmap(UINT nIDResource)
{
	return LoadBitmap(MAKEINTRESOURCE(nIDResource));
}

BOOL CBitmap::CreateBitmap(int nWidth, int nHeight, UINT nPlanes, UINT nBitcount, const void* lpBits)
{
	return Attach(::CreateBitmap(nWidth, nHeight, nPlanes, nBitcount, lpBits));
}

BOOL CRgn::CreateRectRgn(int x1, int y1, int x2, int y2)
{
	return Attach(::CreateRectRgn(x1, y1, x2, y2));
}

BOOL CRgn::CreateRectRgnIndirect(LPCRECT lpRect)
{
	return Attach(::CreateRectRgnIndirect(lpRect));
}

int CRgn::CombineRgn(CRgn* pRgn1, CRgn* pRgn2, int nCombineMode)
{
	return ::CombineRgn(static_cast<HRGN>(m_hObject), static_cast<HRGN>(pRgn1->GetSafeHandle()), static_cast<HRGN>(pRgn2->GetSafeHandle()), nCombineMode);
}

int CRgn::CopyRgn(CRgn* pRgnSrc)
{
	return ::CombineRgn(static_cast<HRGN>(m_hObject), static_cast<HRGN>(pRgnSrc->GetSafeHandle()), nullptr, RGN_COPY);
}

CDC::CDC() :
	m_hDC(nullptr),
	m_hAttribDC(nullptr)
{
}

CDC::~CDC()
{
	Detach();
}

HDC CDC::GetSafeHdc() const
{
	const CDC* self = this;
	return self != nullptr ? m_hDC : nullptr;
}

CDC* PASCAL CDC::FromHandle(HDC hDC)
{
	return AfxDeviceContextMap().fromHandle(hDC);
}

void PASCAL CDC::DeleteTempMap()
{
	AfxDeviceContextMap().deleteTemporaries();
}

BOOL CDC::Attach(HDC hDC)
{
	if (hDC == nullptr)
	{
		return FALSE;
	}
	m_hDC = hDC;
	m_hAttribDC = hDC;
	AfxDeviceContextMap().setPermanent(hDC, this);
	return TRUE;
}

HDC CDC::Detach()
{
	const HDC hDC = m_hDC;
	if (hDC != nullptr)
	{
		AfxDeviceContextMap().removeHandle(hDC);
	}
	m_hDC = nullptr;
	m_hAttribDC = nullptr;
	return hDC;
}

int CDC::SaveDC() { return ::SaveDC(m_hDC); }
BOOL CDC::RestoreDC(int nSavedDC) { return ::RestoreDC(m_hDC, nSavedDC); }
CPen* CDC::SelectObject(CPen* pPen) { return static_cast<CPen*>(CGdiObject::FromHandle(::SelectObject(m_hDC, pPen->GetSafeHandle()))); }
CBrush* CDC::SelectObject(CBrush* pBrush) { return static_cast<CBrush*>(CGdiObject::FromHandle(::SelectObject(m_hDC, pBrush->GetSafeHandle()))); }
CFont* CDC::SelectObject(CFont* pFont) { return static_cast<CFont*>(CGdiObject::FromHandle(::SelectObject(m_hDC, pFont->GetSafeHandle()))); }
CBitmap* CDC::SelectObject(CBitmap* pBitmap) { return static_cast<CBitmap*>(CGdiObject::FromHandle(::SelectObject(m_hDC, pBitmap->GetSafeHandle()))); }
int CDC::SelectObject(CRgn* pRgn) { return ::SelectClipRgn(m_hDC, static_cast<HRGN>(pRgn->GetSafeHandle())); }
CGdiObject* CDC::SelectObject(CGdiObject* pObject) { return CGdiObject::FromHandle(::SelectObject(m_hDC, pObject->GetSafeHandle())); }
CGdiObject* CDC::SelectStockObject(int nIndex) { return CGdiObject::FromHandle(::SelectObject(m_hDC, ::GetStockObject(nIndex))); }

COLORREF CDC::SetBkColor(COLORREF crColor) { return ::SetBkColor(m_hDC, crColor); }
COLORREF CDC::SetTextColor(COLORREF crColor) { return ::SetTextColor(m_hDC, crColor); }
int CDC::SetBkMode(int nBkMode) { return ::SetBkMode(m_hDC, nBkMode); }
int CDC::SetROP2(int nDrawMode) { return ::SetROP2(m_hDC, nDrawMode); }
int CDC::SetStretchBltMode(int nStretchMode) { return ::SetStretchBltMode(m_hDC, nStretchMode); }

CPoint CDC::SetViewportOrg(int x, int y)
{
	CPoint previous;
	::SetViewportOrgEx(m_hDC, x, y, &previous);
	return previous;
}

CPoint CDC::GetViewportOrg() const
{
	CPoint origin;
	::GetViewportOrgEx(m_hDC, &origin);
	return origin;
}

CPoint CDC::OffsetViewportOrg(int nWidth, int nHeight)
{
	CPoint previous;
	::OffsetViewportOrgEx(m_hDC, nWidth, nHeight, &previous);
	return previous;
}

CPoint CDC::MoveTo(int x, int y)
{
	CPoint previous;
	::MoveToEx(m_hDC, x, y, &previous);
	return previous;
}

BOOL CDC::LineTo(int x, int y) { return ::LineTo(m_hDC, x, y); }
BOOL CDC::Polyline(const POINT* lpPoints, int nCount) { return ::Polyline(m_hDC, lpPoints, nCount); }
BOOL CDC::Polygon(const POINT* lpPoints, int nCount) { return ::Polygon(m_hDC, lpPoints, nCount); }
BOOL CDC::Rectangle(int x1, int y1, int x2, int y2) { return ::Rectangle(m_hDC, x1, y1, x2, y2); }
BOOL CDC::Ellipse(int x1, int y1, int x2, int y2) { return ::Ellipse(m_hDC, x1, y1, x2, y2); }
void CDC::FillRect(LPCRECT lpRect, CBrush* pBrush) { ::FillRect(m_hDC, lpRect, static_cast<HBRUSH>(pBrush->GetSafeHandle())); }
void CDC::FrameRect(LPCRECT lpRect, CBrush* pBrush) { ::FrameRect(m_hDC, lpRect, static_cast<HBRUSH>(pBrush->GetSafeHandle())); }

void CDC::FillSolidRect(LPCRECT lpRect, COLORREF clr)
{
	CRect rect(lpRect);
	rect.NormalizeRect();
	::SetBkColor(m_hDC, clr);
	::ExtTextOutA(m_hDC, 0, 0, ETO_OPAQUE, &rect, nullptr, 0, nullptr);
}

void CDC::FillSolidRect(int x, int y, int cx, int cy, COLORREF clr)
{
	const CRect rect(x, y, x + cx, y + cy);
	FillSolidRect(&rect, clr);
}

void CDC::Draw3dRect(LPCRECT lpRect, COLORREF clrTopLeft, COLORREF clrBottomRight)
{
	Draw3dRect(lpRect->left, lpRect->top, lpRect->right - lpRect->left, lpRect->bottom - lpRect->top, clrTopLeft, clrBottomRight);
}

void CDC::Draw3dRect(int x, int y, int cx, int cy, COLORREF clrTopLeft, COLORREF clrBottomRight)
{
	FillSolidRect(x, y, cx - 1, 1, clrTopLeft);
	FillSolidRect(x, y, 1, cy - 1, clrTopLeft);
	FillSolidRect(x + cx, y, -1, cy, clrBottomRight);
	FillSolidRect(x, y + cy, cx, -1, clrBottomRight);
}

COLORREF CDC::SetPixel(int x, int y, COLORREF crColor) { return ::SetPixel(m_hDC, x, y, crColor); }
BOOL CDC::TextOut(int x, int y, LPCSTR lpszString, int nCount) { return ::TextOutA(m_hDC, x, y, lpszString, nCount); }

BOOL CDC::ExtTextOut(int x, int y, UINT nOptions, LPCRECT lpRect, LPCSTR lpszString, UINT nCount, LPINT lpDxWidths)
{
	return ::ExtTextOutA(m_hDC, x, y, nOptions, lpRect, lpszString, nCount, lpDxWidths);
}

int CDC::DrawText(LPCSTR lpszString, int nCount, LPRECT lpRect, UINT nFormat) { return ::DrawTextA(m_hDC, lpszString, nCount, lpRect, nFormat); }

CSize CDC::GetTextExtent(LPCSTR lpszString, int nCount) const
{
	SIZE size = {};
	::GetTextExtentPoint32A(m_hAttribDC, lpszString, nCount, &size);
	return CSize(size);
}

BOOL CDC::GetTextMetrics(LPTEXTMETRICA lpMetrics) const { return ::GetTextMetricsA(m_hAttribDC, lpMetrics); }
int CDC::SelectClipRgn(CRgn* pRgn) { return ::SelectClipRgn(m_hDC, pRgn != nullptr ? static_cast<HRGN>(pRgn->GetSafeHandle()) : nullptr); }
int CDC::GetClipBox(LPRECT lpRect) const { return ::GetClipBox(m_hDC, lpRect); }
int CDC::IntersectClipRect(int x1, int y1, int x2, int y2) { return ::IntersectClipRect(m_hDC, x1, y1, x2, y2); }
int CDC::IntersectClipRect(LPCRECT lpRect) { return ::IntersectClipRect(m_hDC, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom); }

int CDC::StretchDIBits(int x, int y, int nWidth, int nHeight, int xSrc, int ySrc, int nSrcWidth, int nSrcHeight, const void* lpBits,
	const BITMAPINFO* lpBitsInfo, UINT nUsage, DWORD dwRop)
{
	return ::StretchDIBits(m_hDC, x, y, nWidth, nHeight, xSrc, ySrc, nSrcWidth, nSrcHeight, lpBits, lpBitsInfo, nUsage, dwRop);
}

CPaintDC::CPaintDC(CWnd* pWnd) :
	m_ps(),
	m_hWnd(pWnd->m_hWnd)
{
	if (!Attach(::BeginPaint(m_hWnd, &m_ps)))
	{
		AfxThrowResourceException();
	}
}

CPaintDC::~CPaintDC()
{
	::EndPaint(m_hWnd, &m_ps);
	Detach();
}

CClientDC::CClientDC(CWnd* pWnd) :
	m_hWnd(pWnd->GetSafeHwnd())
{
	if (!Attach(::GetDC(m_hWnd)))
	{
		AfxThrowResourceException();
	}
}

CClientDC::~CClientDC()
{
	::ReleaseDC(m_hWnd, Detach());
}

CWindowDC::CWindowDC(CWnd* pWnd) :
	m_hWnd(pWnd->GetSafeHwnd())
{
	if (!Attach(::GetWindowDC(m_hWnd)))
	{
		AfxThrowResourceException();
	}
}

CWindowDC::~CWindowDC()
{
	::ReleaseDC(m_hWnd, Detach());
}
