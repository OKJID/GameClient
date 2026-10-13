#include "Win32Gdi.h"

#include <algorithm>

namespace
{

Win32Region* region(HRGN handle)
{
	Win32GdiObject* object = Win32Gdi::object(handle);
	return object != nullptr && object->type == Win32GdiObjectType::Region ? static_cast<Win32Region*>(object) : nullptr;
}

std::vector<RECT> subtractRect(const RECT& source, const RECT& cut)
{
	RECT overlap;
	if (!IntersectRect(&overlap, &source, &cut))
	{
		return { source };
	}

	std::vector<RECT> pieces;
	const RECT above = { source.left, source.top, source.right, overlap.top };
	const RECT below = { source.left, overlap.bottom, source.right, source.bottom };
	const RECT left = { source.left, overlap.top, overlap.left, overlap.bottom };
	const RECT right = { overlap.right, overlap.top, source.right, overlap.bottom };
	for (const RECT& piece : { above, below, left, right })
	{
		if (!IsRectEmpty(&piece))
		{
			pieces.push_back(piece);
		}
	}
	return pieces;
}

std::vector<RECT> subtractRegion(const std::vector<RECT>& source, const std::vector<RECT>& cut)
{
	std::vector<RECT> result = source;
	for (const RECT& cutRect : cut)
	{
		std::vector<RECT> next;
		for (const RECT& rect : result)
		{
			const std::vector<RECT> pieces = subtractRect(rect, cutRect);
			next.insert(next.end(), pieces.begin(), pieces.end());
		}
		result.swap(next);
	}
	return result;
}

std::vector<RECT> intersectRegion(const std::vector<RECT>& first, const std::vector<RECT>& second)
{
	std::vector<RECT> result;
	for (const RECT& a : first)
	{
		for (const RECT& b : second)
		{
			RECT overlap;
			if (IntersectRect(&overlap, &a, &b))
			{
				result.push_back(overlap);
			}
		}
	}
	return result;
}

std::vector<RECT> unionRegion(const std::vector<RECT>& first, const std::vector<RECT>& second)
{
	std::vector<RECT> result = first;
	const std::vector<RECT> extra = subtractRegion(second, first);
	result.insert(result.end(), extra.begin(), extra.end());
	return result;
}

std::vector<RECT> combine(const std::vector<RECT>& first, const std::vector<RECT>& second, int mode)
{
	switch (mode)
	{
	case RGN_AND:
		return intersectRegion(first, second);
	case RGN_OR:
		return unionRegion(first, second);
	case RGN_DIFF:
		return subtractRegion(first, second);
	case RGN_XOR:
		return unionRegion(subtractRegion(first, second), subtractRegion(second, first));
	default:
		return first;
	}
}

}

HRGN CreateRectRgn(int left, int top, int right, int bottom)
{
	auto created = std::make_unique<Win32Region>();
	const RECT rect = { left, top, right, bottom };
	if (!IsRectEmpty(&rect))
	{
		created->rects.push_back(rect);
	}
	return static_cast<HRGN>(Win32Gdi::registerObject(std::move(created)));
}

HRGN CreateRectRgnIndirect(const RECT* rect)
{
	return rect != nullptr ? CreateRectRgn(rect->left, rect->top, rect->right, rect->bottom) : nullptr;
}

BOOL SetRectRgn(HRGN handle, int left, int top, int right, int bottom)
{
	Win32Region* target = region(handle);
	if (target == nullptr)
	{
		return FALSE;
	}
	const RECT rect = { left, top, right, bottom };
	target->rects.clear();
	if (!IsRectEmpty(&rect))
	{
		target->rects.push_back(rect);
	}
	return TRUE;
}

int CombineRgn(HRGN destination, HRGN first, HRGN second, int mode)
{
	Win32Region* target = region(destination);
	Win32Region* source = region(first);
	Win32Region* other = mode == RGN_COPY ? source : region(second);
	if (target == nullptr || source == nullptr || other == nullptr)
	{
		return ERROR;
	}
	target->rects = combine(source->rects, other->rects, mode);
	return target->complexity();
}

int OffsetRgn(HRGN handle, int dx, int dy)
{
	Win32Region* target = region(handle);
	if (target == nullptr)
	{
		return ERROR;
	}
	for (RECT& rect : target->rects)
	{
		OffsetRect(&rect, dx, dy);
	}
	return target->complexity();
}

int GetRgnBox(HRGN handle, LPRECT rect)
{
	Win32Region* target = region(handle);
	if (target == nullptr || rect == nullptr)
	{
		return ERROR;
	}
	*rect = target->bounds();
	return target->complexity();
}

BOOL PtInRegion(HRGN handle, int x, int y)
{
	Win32Region* target = region(handle);
	if (target == nullptr)
	{
		return FALSE;
	}
	const POINT point = { x, y };
	return std::any_of(target->rects.begin(), target->rects.end(), [&](const RECT& rect) { return PtInRect(&rect, point) != FALSE; }) ? TRUE : FALSE;
}

BOOL RectInRegion(HRGN handle, const RECT* rect)
{
	Win32Region* target = region(handle);
	if (target == nullptr || rect == nullptr)
	{
		return FALSE;
	}
	RECT overlap;
	return std::any_of(target->rects.begin(), target->rects.end(), [&](const RECT& part) { return IntersectRect(&overlap, &part, rect) != FALSE; }) ? TRUE : FALSE;
}

int SelectClipRgn(HDC hdc, HRGN handle)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr)
	{
		return ERROR;
	}

	const RECT surface = Win32Gdi::surfaceBounds(dc);
	Win32Region* clipRegion = region(handle);
	if (clipRegion == nullptr)
	{
		dc->clip = surface;
		return SIMPLEREGION;
	}

	const RECT bounds = clipRegion->bounds();
	IntersectRect(&dc->clip, &surface, &bounds);
	return clipRegion->complexity();
}

int GetClipBox(HDC hdc, LPRECT rect)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr || rect == nullptr)
	{
		return ERROR;
	}
	*rect = dc->clip;
	OffsetRect(rect, -dc->viewportOrigin.x, -dc->viewportOrigin.y);
	return IsRectEmpty(rect) ? NULLREGION : SIMPLEREGION;
}

int IntersectClipRect(HDC hdc, int left, int top, int right, int bottom)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr)
	{
		return ERROR;
	}
	RECT rect;
	SetRect(&rect, left, top, right, bottom);
	OffsetRect(&rect, dc->viewportOrigin.x, dc->viewportOrigin.y);
	IntersectRect(&dc->clip, &dc->clip, &rect);
	return IsRectEmpty(&dc->clip) ? NULLREGION : SIMPLEREGION;
}

int GetUpdateRgn(HWND hwnd, HRGN handle, BOOL erase)
{
	Win32Region* target = region(handle);
	if (target == nullptr)
	{
		return ERROR;
	}

	RECT update;
	target->rects.clear();
	if (GetUpdateRect(hwnd, &update, erase))
	{
		target->rects.push_back(update);
	}
	return target->complexity();
}
