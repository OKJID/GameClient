#include "Win32RichEdit.h"

#include "Win32ControlDrawing.h"
#include "Win32Gdi.h"

#include <algorithm>
#include <cstring>

namespace
{

const float kTextMargin = 2.0f;

float glyphWidth(char symbol)
{
	const char text[2] = { symbol, '\0' };
	return ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, 0.0f, text, text + 1).x;
}

}

int Win32RichEdit::charFromPoint(LPARAM point) const
{
	const ImVec2 position(static_cast<float>(m_clientOrigin.x + GET_X_LPARAM(point)), static_cast<float>(m_clientOrigin.y + GET_Y_LPARAM(point)));
	int best = textLength();
	for (size_t index = 0; index < m_glyphs.size(); ++index)
	{
		const Win32RichGlyph& glyph = m_glyphs[index];
		if (position.y < glyph.min.y || position.y >= glyph.max.y)
		{
			continue;
		}
		if (position.x < (glyph.min.x + glyph.max.x) * 0.5f)
		{
			return static_cast<int>(index);
		}
		best = static_cast<int>(index) + 1;
	}
	return best;
}

LRESULT Win32RichEdit::handleMouse(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (notifyMessageFilter(message, wParam, lParam) != 0)
	{
		return 0;
	}

	const int index = charFromPoint(lParam);
	const bool isOnLink = isLink(index);
	if (isOnLink && notifyLink(message, wParam, lParam, linkRangeAt(index)) != 0)
	{
		return 0;
	}

	switch (message)
	{
	case WM_LBUTTONDOWN:
		takeFocus();
		m_isSelecting = true;
		m_anchor = index;
		SetCapture(hwnd());
		setSelection(index, index);
		return 0;
	case WM_MOUSEMOVE:
		if (m_isSelecting)
		{
			setSelection(std::min(m_anchor, index), std::max(m_anchor, index));
		}
		return 0;
	case WM_LBUTTONUP:
		if (m_isSelecting)
		{
			m_isSelecting = false;
			ReleaseCapture();
		}
		return 0;
	case WM_SETCURSOR:
		SetCursor(LoadCursorA(nullptr, isOnLink ? IDC_HAND : IDC_IBEAM));
		return TRUE;
	default:
		m_isSelecting = false;
		return 0;
	}
}

LRESULT Win32RichEdit::handleKey(WPARAM key)
{
	if (notifyMessageFilter(WM_KEYDOWN, key, 0) != 0)
	{
		return 0;
	}

	const bool extends = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	const int caret = m_selectionEnd != m_anchor ? m_selectionEnd : m_selectionStart;
	int target = caret;
	switch (key)
	{
	case VK_LEFT:
		target = caret - 1;
		break;
	case VK_RIGHT:
		target = caret + 1;
		break;
	case VK_HOME:
		target = 0;
		break;
	case VK_END:
		target = textLength();
		break;
	case VK_DELETE:
		if (!isReadOnly())
		{
			setSelection(m_selectionStart, m_selectionEnd == m_selectionStart ? m_selectionEnd + 1 : m_selectionEnd);
			replaceSelection("");
		}
		return 0;
	default:
		return 0;
	}

	target = std::clamp(target, 0, textLength());
	m_anchor = extends ? m_anchor : target;
	setSelection(std::min(m_anchor, target), std::max(m_anchor, target));
	return 0;
}

LRESULT Win32RichEdit::handleCharacter(char symbol)
{
	if (notifyMessageFilter(WM_CHAR, static_cast<WPARAM>(static_cast<unsigned char>(symbol)), 0) != 0 || isReadOnly())
	{
		return 0;
	}
	if (symbol == 0x08)
	{
		if (m_selectionStart == m_selectionEnd && m_selectionStart > 0)
		{
			setSelection(m_selectionStart - 1, m_selectionEnd);
		}
		replaceSelection("");
		return 0;
	}
	const bool isLineBreak = symbol == '\r' && (style() & ES_MULTILINE) != 0;
	if (static_cast<unsigned char>(symbol) < 0x20 && !isLineBreak)
	{
		return 0;
	}
	replaceSelection(std::string(1, symbol));
	return 0;
}

LRESULT Win32RichEdit::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	bool handled = false;
	LRESULT result = handleTextMessage(message, wParam, lParam, handled);
	if (handled)
	{
		return result;
	}
	result = handleFormatMessage(message, wParam, lParam, handled);
	if (handled)
	{
		return result;
	}

	switch (message)
	{
	case WM_CREATE:
		m_formats.assign(m_window.text.size(), m_defaultFormat);
		return 0;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MOUSEMOVE:
	case WM_CAPTURECHANGED:
		return handleMouse(message, wParam, lParam);
	case WM_SETCURSOR:
	{
		POINT cursor;
		GetCursorPos(&cursor);
		ScreenToClient(hwnd(), &cursor);
		return handleMouse(message, wParam, MAKELPARAM(cursor.x, cursor.y));
	}
	case WM_KEYDOWN:
		return handleKey(wParam);
	case WM_CHAR:
		return handleCharacter(static_cast<char>(wParam));
	case WM_GETDLGCODE:
		return DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS | ((style() & ES_MULTILINE) != 0 ? DLGC_WANTALLKEYS : 0);
	case WM_SETFOCUS:
		notifyCommand(EN_SETFOCUS);
		return 0;
	case WM_KILLFOCUS:
		notifyCommand(EN_KILLFOCUS);
		return 0;
	default:
		return Win32Control::handleMessage(message, wParam, lParam);
	}
}

void Win32RichEdit::layoutGlyphs(const Win32DrawContext& context)
{
	const float lineHeight = ImGui::GetTextLineHeight();
	const float left = static_cast<float>(context.clientOrigin.x) + kTextMargin;
	const float right = static_cast<float>(context.clientOrigin.x + context.clientSize.cx) - kTextMargin;
	const bool wraps = (style() & ES_MULTILINE) != 0 && (style() & ES_AUTOHSCROLL) == 0;
	const std::string& text = m_window.text;
	m_glyphs.assign(text.size(), Win32RichGlyph());

	ImVec2 pen(left, static_cast<float>(context.clientOrigin.y) + kTextMargin);
	size_t lineStart = 0;
	for (size_t index = 0; index < text.size(); ++index)
	{
		const char symbol = text[index];
		if (symbol == '\r' || symbol == '\n')
		{
			m_glyphs[index] = { pen, ImVec2(pen.x, pen.y + lineHeight) };
			pen = ImVec2(left, pen.y + (symbol == '\n' && index > 0 && text[index - 1] == '\r' ? 0.0f : lineHeight));
			lineStart = index + 1;
			continue;
		}
		const float width = glyphWidth(symbol);
		if (wraps && pen.x + width > right && index > lineStart)
		{
			const size_t space = text.rfind(' ', index);
			const size_t breakAt = space != std::string::npos && space >= lineStart ? space + 1 : index;
			pen = ImVec2(left, pen.y + lineHeight);
			for (size_t moved = breakAt; moved < index; ++moved)
			{
				const float movedWidth = m_glyphs[moved].max.x - m_glyphs[moved].min.x;
				m_glyphs[moved] = { pen, ImVec2(pen.x + movedWidth, pen.y + lineHeight) };
				pen.x += movedWidth;
			}
			lineStart = breakAt;
		}
		m_glyphs[index] = { pen, ImVec2(pen.x + width, pen.y + lineHeight) };
		pen.x += width;
	}
}

void Win32RichEdit::drawGlyphs(const Win32DrawContext& context)
{
	const std::string& text = m_window.text;
	const bool showsSelection = !m_hidesSelection && (context.hasFocus || (style() & ES_NOHIDESEL) != 0);
	for (size_t index = 0; index < text.size(); ++index)
	{
		const Win32RichGlyph& glyph = m_glyphs[index];
		const Win32RichCharFormat& format = index < m_formats.size() ? m_formats[index] : m_defaultFormat;
		const bool isSelected = showsSelection && static_cast<int>(index) >= m_selectionStart && static_cast<int>(index) < m_selectionEnd;
		if (isSelected)
		{
			context.drawList->AddRectFilled(glyph.min, glyph.max, Win32ControlDrawing::color(ImGuiCol_TextSelectedBg));
		}
		if (text[index] == '\r' || text[index] == '\n')
		{
			continue;
		}
		const ImU32 color = format.usesAutoColor ? Win32ControlDrawing::color(ImGuiCol_Text) : Win32Gdi::toImColor(format.color);
		const char symbol[2] = { text[index], '\0' };
		context.drawList->AddText(glyph.min, color, symbol);
		if ((format.effects & CFE_BOLD) != 0)
		{
			context.drawList->AddText(ImVec2(glyph.min.x + 1.0f, glyph.min.y), color, symbol);
		}
		if ((format.effects & (CFE_UNDERLINE | CFE_LINK)) != 0)
		{
			context.drawList->AddLine(ImVec2(glyph.min.x, glyph.max.y - 1.0f), ImVec2(glyph.max.x, glyph.max.y - 1.0f), color);
		}
	}
}

void Win32RichEdit::draw(const Win32DrawContext& context)
{
	m_clientOrigin = context.clientOrigin;
	const ImVec2 origin = Win32ControlDrawing::screenPoint(context, 0, 0);
	const ImVec2 size = Win32ControlDrawing::clientSize(context);
	const ImU32 background = m_backgroundColor != CLR_DEFAULT ? Win32Gdi::toImColor(m_backgroundColor) : Win32ControlDrawing::color(ImGuiCol_FrameBg);
	context.drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), background);

	layoutGlyphs(context);
	drawGlyphs(context);
	if (context.hasFocus && m_selectionStart == m_selectionEnd)
	{
		const bool isAtEnd = m_selectionStart >= static_cast<int>(m_glyphs.size());
		const ImVec2 caret = isAtEnd ? (m_glyphs.empty() ? ImVec2(origin.x + kTextMargin, origin.y + kTextMargin) : ImVec2(m_glyphs.back().max.x, m_glyphs.back().min.y))
			: m_glyphs[static_cast<size_t>(m_selectionStart)].min;
		context.drawList->AddLine(caret, ImVec2(caret.x, caret.y + ImGui::GetTextLineHeight()), Win32ControlDrawing::color(ImGuiCol_Text));
	}
}
