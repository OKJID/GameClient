#pragma once

#include <cstdarg>
#include <cstdio>
#include <string>

class CPoint : public POINT
{
public:
	CPoint() { x = 0; y = 0; }
	CPoint(int initX, int initY) { x = initX; y = initY; }
	CPoint(POINT point) { x = point.x; y = point.y; }

	bool operator==(const POINT& other) const { return x == other.x && y == other.y; }
	bool operator!=(const POINT& other) const { return !(*this == other); }
	CPoint operator+(const POINT& other) const { return CPoint(x + other.x, y + other.y); }
	CPoint operator-(const POINT& other) const { return CPoint(x - other.x, y - other.y); }
	void Offset(int dx, int dy) { x += dx; y += dy; }
};

class CSize : public SIZE
{
public:
	CSize() { cx = 0; cy = 0; }
	CSize(int initCX, int initCY) { cx = initCX; cy = initCY; }
};

class CRect : public RECT
{
public:
	CRect() { left = 0; top = 0; right = 0; bottom = 0; }
	CRect(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
	CRect(const RECT& rect) { left = rect.left; top = rect.top; right = rect.right; bottom = rect.bottom; }

	int Width() const { return right - left; }
	int Height() const { return bottom - top; }
	CPoint TopLeft() const { return CPoint(left, top); }
	CPoint BottomRight() const { return CPoint(right, bottom); }
	CPoint CenterPoint() const { return CPoint((left + right) / 2, (top + bottom) / 2); }
	bool IsRectEmpty() const { return right <= left || bottom <= top; }
	bool PtInRect(POINT point) const { return point.x >= left && point.x < right && point.y >= top && point.y < bottom; }
	void OffsetRect(int dx, int dy) { left += dx; right += dx; top += dy; bottom += dy; }
	void InflateRect(int dx, int dy) { left -= dx; right += dx; top -= dy; bottom += dy; }

	void NormalizeRect()
	{
		if (left > right) { int swap = left; left = right; right = swap; }
		if (top > bottom) { int swap = top; top = bottom; bottom = swap; }
	}

	operator RECT*() { return this; }
	operator const RECT*() const { return this; }
};

class CString
{
public:
	CString() {}
	CString(const char* text) : m_text(text ? text : "") {}
	CString(const CString& other) = default;

	CString& operator=(const char* text) { m_text = text ? text : ""; return *this; }
	CString& operator=(const CString& other) = default;
	CString& operator+=(const char* text) { m_text += text ? text : ""; return *this; }
	CString& operator+=(char symbol) { m_text += symbol; return *this; }
	CString operator+(const char* text) const { CString result(*this); result += text; return result; }

	bool operator==(const char* text) const { return m_text == (text ? text : ""); }
	bool operator!=(const char* text) const { return !(*this == text); }
	operator const char*() const { return m_text.c_str(); }

	int GetLength() const { return (int)m_text.size(); }
	bool IsEmpty() const { return m_text.empty(); }
	void Empty() { m_text.clear(); }
	char GetAt(int index) const { return m_text[index]; }

	void Format(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		va_list argsCopy;
		va_copy(argsCopy, args);
		const int length = vsnprintf(nullptr, 0, format, argsCopy);
		va_end(argsCopy);
		m_text.resize(length > 0 ? length : 0);
		vsnprintf(&m_text[0], m_text.size() + 1, format, args);
		va_end(args);
	}

private:
	std::string m_text;
};
