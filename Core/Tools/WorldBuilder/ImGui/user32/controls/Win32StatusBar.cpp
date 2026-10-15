#include "Win32StatusBar.h"

#include "Win32ControlDrawing.h"

#include <algorithm>
#include <cstring>

namespace
{

const int kHorizontalBorder = 0;
const int kVerticalBorder = 2;
const int kPartSpacing = 2;
const int kTextPadding = 3;

}

Win32StatusBar::Part* Win32StatusBar::partAt(WPARAM index)
{
	if ((index & 0xFF) == SB_SIMPLEID)
	{
		return &m_simplePart;
	}
	return (index & 0xFF) < m_parts.size() ? &m_parts[index & 0xFF] : nullptr;
}

const Win32StatusBar::Part* Win32StatusBar::partAt(WPARAM index) const
{
	return const_cast<Win32StatusBar*>(this)->partAt(index);
}

LRESULT Win32StatusBar::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case SB_SETPARTS:
		return setParts(static_cast<int>(wParam), reinterpret_cast<const int*>(lParam));
	case SB_GETPARTS:
		return getParts(static_cast<int>(wParam), reinterpret_cast<int*>(lParam));
	case SB_GETRECT:
		return getRect(wParam, reinterpret_cast<RECT*>(lParam));
	case SB_GETBORDERS:
	{
		int* borders = reinterpret_cast<int*>(lParam);
		if (borders == nullptr)
		{
			return FALSE;
		}
		borders[0] = kHorizontalBorder;
		borders[1] = kVerticalBorder;
		borders[2] = kPartSpacing;
		return TRUE;
	}
	case SB_SETMINHEIGHT:
		m_minHeight = static_cast<int>(wParam);
		return 0;
	case SB_SIMPLE:
		m_isSimple = wParam != 0;
		InvalidateRect(hwnd(), nullptr, TRUE);
		return 0;
	case SB_ISSIMPLE:
		return m_isSimple ? TRUE : FALSE;
	case WM_ERASEBKGND:
		return Win32Control::eraseWithSystemColor(reinterpret_cast<HDC>(wParam), COLOR_BTNFACE);
	default:
		return handleTextMessage(message, wParam, lParam);
	}
}

LRESULT Win32StatusBar::handleTextMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case SB_SETTEXT:
		return setText(wParam, reinterpret_cast<LPCSTR>(lParam));
	case SB_GETTEXT:
		return getText(wParam, reinterpret_cast<LPSTR>(lParam));
	case SB_GETTEXTLENGTH:
		return textLength(wParam);
	case WM_SETTEXT:
		return setText(0, reinterpret_cast<LPCSTR>(lParam));
	case WM_GETTEXT:
		return Win32CopyWindowText(m_parts.front().text, reinterpret_cast<LPSTR>(lParam), static_cast<int>(wParam));
	case WM_GETTEXTLENGTH:
		return static_cast<LRESULT>(m_parts.front().text.size());
	default:
		return Win32Control::handleMessage(message, wParam, lParam);
	}
}

LRESULT Win32StatusBar::setParts(int count, const int* rightEdges)
{
	if (count <= 0 || count > 256 || rightEdges == nullptr)
	{
		return FALSE;
	}
	m_parts.resize(static_cast<size_t>(count));
	for (int index = 0; index < count; ++index)
	{
		m_parts[static_cast<size_t>(index)].rightEdge = rightEdges[index];
	}
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}

LRESULT Win32StatusBar::getParts(int count, int* rightEdges) const
{
	if (rightEdges != nullptr)
	{
		const int copied = std::min(count, static_cast<int>(m_parts.size()));
		for (int index = 0; index < copied; ++index)
		{
			rightEdges[index] = m_parts[static_cast<size_t>(index)].rightEdge;
		}
	}
	return static_cast<LRESULT>(m_parts.size());
}

LRESULT Win32StatusBar::setText(WPARAM indexAndType, LPCSTR text)
{
	Part* part = partAt(indexAndType);
	if (part == nullptr)
	{
		return FALSE;
	}
	part->text = text != nullptr ? text : "";
	part->type = static_cast<UINT>(indexAndType & 0xFF00);
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}

LRESULT Win32StatusBar::getText(WPARAM index, LPSTR buffer) const
{
	const Part* part = partAt(index);
	if (part == nullptr)
	{
		return 0;
	}
	if (buffer != nullptr)
	{
		memcpy(buffer, part->text.c_str(), part->text.size() + 1);
	}
	return MAKELONG(part->text.size(), part->type);
}

LRESULT Win32StatusBar::textLength(WPARAM index) const
{
	const Part* part = partAt(index);
	return part != nullptr ? MAKELONG(part->text.size(), part->type) : 0;
}

RECT Win32StatusBar::partRect(size_t index) const
{
	const int width = m_window.width();
	const int left = index == 0 ? kHorizontalBorder : m_parts[index - 1].rightEdge + kPartSpacing;
	const int right = m_parts[index].rightEdge < 0 ? width - kHorizontalBorder : m_parts[index].rightEdge;
	RECT rect;
	SetRect(&rect, std::min(left, width), kVerticalBorder, std::min(right, width), m_window.height() - kVerticalBorder);
	return rect;
}

LRESULT Win32StatusBar::getRect(WPARAM index, RECT* rect) const
{
	if (rect == nullptr || index >= m_parts.size())
	{
		return FALSE;
	}
	*rect = partRect(index);
	return TRUE;
}

void Win32StatusBar::draw(const Win32DrawContext& context)
{
	if (m_isSimple)
	{
		RECT rect;
		SetRect(&rect, kHorizontalBorder, kVerticalBorder, m_window.width() - kHorizontalBorder, m_window.height() - kVerticalBorder);
		drawPart(context, m_simplePart, rect);
		return;
	}
	for (size_t index = 0; index < m_parts.size(); ++index)
	{
		drawPart(context, m_parts[index], partRect(index));
	}
}

void Win32StatusBar::drawPart(const Win32DrawContext& context, const Part& part, const RECT& rect)
{
	if (IsRectEmpty(&rect))
	{
		return;
	}
	const ImVec2 min = Win32ControlDrawing::screenPoint(context, rect.left, rect.top);
	const ImVec2 max = Win32ControlDrawing::screenPoint(context, rect.right, rect.bottom);
	if ((part.type & SBT_NOBORDERS) == 0)
	{
		context.drawList->AddRect(min, max, Win32ControlDrawing::color(ImGuiCol_Border));
	}
	const float textHeight = ImGui::GetTextLineHeight();
	const ImVec2 position(min.x + kTextPadding, min.y + (max.y - min.y - textHeight) * 0.5f);
	const ImGuiCol textColor = context.isEnabled ? ImGuiCol_Text : ImGuiCol_TextDisabled;
	context.drawList->PushClipRect(min, max, true);
	context.drawList->AddText(position, Win32ControlDrawing::color(textColor), part.text.c_str());
	context.drawList->PopClipRect();
}
