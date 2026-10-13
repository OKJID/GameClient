#pragma once

#include "AfxCore.h"

class CSize;
class CRect;

class CPoint : public POINT
{
public:
	CPoint() { x = 0; y = 0; }
	CPoint(int initX, int initY) { x = initX; y = initY; }
	CPoint(POINT initPt) { x = initPt.x; y = initPt.y; }
	CPoint(SIZE initSize) { x = initSize.cx; y = initSize.cy; }
	CPoint(LPARAM dwPoint) { x = GET_X_LPARAM(dwPoint); y = GET_Y_LPARAM(dwPoint); }

	void Offset(int xOffset, int yOffset) { x += xOffset; y += yOffset; }
	void Offset(POINT point) { Offset(point.x, point.y); }
	void Offset(SIZE size) { Offset(size.cx, size.cy); }

	BOOL operator==(POINT point) const { return x == point.x && y == point.y; }
	BOOL operator!=(POINT point) const { return x != point.x || y != point.y; }
	void operator+=(SIZE size) { Offset(size); }
	void operator-=(SIZE size) { Offset(-size.cx, -size.cy); }
	void operator+=(POINT point) { Offset(point); }
	void operator-=(POINT point) { Offset(-point.x, -point.y); }

	CPoint operator+(SIZE size) const { return CPoint(x + size.cx, y + size.cy); }
	CPoint operator-(SIZE size) const { return CPoint(x - size.cx, y - size.cy); }
	CPoint operator-() const { return CPoint(-x, -y); }
	CPoint operator+(POINT point) const { return CPoint(x + point.x, y + point.y); }
	CSize operator-(POINT point) const;
};

class CSize : public SIZE
{
public:
	CSize() { cx = 0; cy = 0; }
	CSize(int initCX, int initCY) { cx = initCX; cy = initCY; }
	CSize(SIZE initSize) { cx = initSize.cx; cy = initSize.cy; }
	CSize(POINT initPt) { cx = initPt.x; cy = initPt.y; }

	BOOL operator==(SIZE size) const { return cx == size.cx && cy == size.cy; }
	BOOL operator!=(SIZE size) const { return cx != size.cx || cy != size.cy; }
	void operator+=(SIZE size) { cx += size.cx; cy += size.cy; }
	void operator-=(SIZE size) { cx -= size.cx; cy -= size.cy; }

	CSize operator+(SIZE size) const { return CSize(cx + size.cx, cy + size.cy); }
	CSize operator-(SIZE size) const { return CSize(cx - size.cx, cy - size.cy); }
	CSize operator-() const { return CSize(-cx, -cy); }
	CPoint operator+(POINT point) const { return CPoint(cx + point.x, cy + point.y); }
	CPoint operator-(POINT point) const { return CPoint(cx - point.x, cy - point.y); }
};

inline CSize CPoint::operator-(POINT point) const
{
	return CSize(x - point.x, y - point.y);
}

class CRect : public RECT
{
public:
	CRect() { ::SetRectEmpty(this); }
	CRect(int l, int t, int r, int b) { ::SetRect(this, l, t, r, b); }
	CRect(const RECT& srcRect) { ::CopyRect(this, &srcRect); }
	CRect(const RECT* lpSrcRect) { ::CopyRect(this, lpSrcRect); }
	CRect(POINT point, SIZE size) { ::SetRect(this, point.x, point.y, point.x + size.cx, point.y + size.cy); }
	CRect(POINT topLeft, POINT bottomRight) { ::SetRect(this, topLeft.x, topLeft.y, bottomRight.x, bottomRight.y); }

	int Width() const { return right - left; }
	int Height() const { return bottom - top; }
	CSize Size() const { return CSize(Width(), Height()); }
	CPoint& TopLeft() { return *reinterpret_cast<CPoint*>(this); }
	CPoint& BottomRight() { return *(reinterpret_cast<CPoint*>(this) + 1); }
	const CPoint& TopLeft() const { return *reinterpret_cast<const CPoint*>(this); }
	const CPoint& BottomRight() const { return *(reinterpret_cast<const CPoint*>(this) + 1); }
	CPoint CenterPoint() const { return CPoint((left + right) / 2, (top + bottom) / 2); }

	operator LPRECT() { return this; }
	operator LPCRECT() const { return this; }

	BOOL IsRectEmpty() const { return ::IsRectEmpty(this); }
	BOOL IsRectNull() const { return left == 0 && right == 0 && top == 0 && bottom == 0; }
	BOOL PtInRect(POINT point) const { return ::PtInRect(this, point); }

	void SetRect(int x1, int y1, int x2, int y2) { ::SetRect(this, x1, y1, x2, y2); }
	void SetRect(POINT topLeft, POINT bottomRight) { ::SetRect(this, topLeft.x, topLeft.y, bottomRight.x, bottomRight.y); }
	void SetRectEmpty() { ::SetRectEmpty(this); }
	void CopyRect(LPCRECT lpSrcRect) { ::CopyRect(this, lpSrcRect); }
	BOOL EqualRect(LPCRECT lpRect) const { return ::EqualRect(this, lpRect); }

	void InflateRect(int x, int y) { ::InflateRect(this, x, y); }
	void InflateRect(SIZE size) { ::InflateRect(this, size.cx, size.cy); }
	void InflateRect(LPCRECT lpRect);
	void InflateRect(int l, int t, int r, int b);
	void DeflateRect(int x, int y) { ::InflateRect(this, -x, -y); }
	void DeflateRect(SIZE size) { ::InflateRect(this, -size.cx, -size.cy); }
	void DeflateRect(LPCRECT lpRect);
	void DeflateRect(int l, int t, int r, int b);

	void OffsetRect(int x, int y) { ::OffsetRect(this, x, y); }
	void OffsetRect(SIZE size) { ::OffsetRect(this, size.cx, size.cy); }
	void OffsetRect(POINT point) { ::OffsetRect(this, point.x, point.y); }
	void NormalizeRect();

	BOOL IntersectRect(LPCRECT lpRect1, LPCRECT lpRect2) { return ::IntersectRect(this, lpRect1, lpRect2); }
	BOOL UnionRect(LPCRECT lpRect1, LPCRECT lpRect2) { return ::UnionRect(this, lpRect1, lpRect2); }

	void operator=(const RECT& srcRect) { ::CopyRect(this, &srcRect); }
	BOOL operator==(const RECT& rect) const { return ::EqualRect(this, &rect); }
	BOOL operator!=(const RECT& rect) const { return !::EqualRect(this, &rect); }
	void operator+=(POINT point) { OffsetRect(point); }
	void operator+=(SIZE size) { OffsetRect(size); }
	void operator-=(POINT point) { OffsetRect(-point.x, -point.y); }
	void operator-=(SIZE size) { OffsetRect(-size.cx, -size.cy); }
	void operator&=(const RECT& rect) { ::IntersectRect(this, this, &rect); }
	void operator|=(const RECT& rect) { ::UnionRect(this, this, &rect); }

	CRect operator+(POINT point) const { CRect rect(*this); rect.OffsetRect(point); return rect; }
	CRect operator-(POINT point) const { CRect rect(*this); rect.OffsetRect(-point.x, -point.y); return rect; }
	CRect operator+(SIZE size) const { CRect rect(*this); rect.OffsetRect(size); return rect; }
	CRect operator-(SIZE size) const { CRect rect(*this); rect.OffsetRect(-size.cx, -size.cy); return rect; }
	CRect operator&(const RECT& rect2) const { CRect rect; ::IntersectRect(&rect, this, &rect2); return rect; }
	CRect operator|(const RECT& rect2) const { CRect rect; ::UnionRect(&rect, this, &rect2); return rect; }
};
