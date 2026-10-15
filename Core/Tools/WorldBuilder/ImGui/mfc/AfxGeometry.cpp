#include "afx.h"

#include <algorithm>

void CRect::InflateRect(LPCRECT lpRect)
{
	InflateRect(lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
}

void CRect::InflateRect(int l, int t, int r, int b)
{
	left -= l;
	top -= t;
	right += r;
	bottom += b;
}

void CRect::DeflateRect(LPCRECT lpRect)
{
	DeflateRect(lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
}

void CRect::DeflateRect(int l, int t, int r, int b)
{
	left += l;
	top += t;
	right -= r;
	bottom -= b;
}

void CRect::NormalizeRect()
{
	if (left > right)
	{
		std::swap(left, right);
	}
	if (top > bottom)
	{
		std::swap(top, bottom);
	}
}
