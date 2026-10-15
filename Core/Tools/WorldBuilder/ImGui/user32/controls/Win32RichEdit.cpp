#include "Win32RichEdit.h"

#include <algorithm>
#include <cstring>
#include <utility>

void Win32RichEdit::setText(const std::string& text)
{
	m_window.text = text.substr(0, static_cast<size_t>(m_limit));
	m_formats.assign(m_window.text.size(), m_defaultFormat);
	m_selectionStart = 0;
	m_selectionEnd = 0;
	m_anchor = 0;
	InvalidateRect(hwnd(), nullptr, TRUE);
}

void Win32RichEdit::setSelection(int start, int end)
{
	const int length = textLength();
	if (start < 0)
	{
		start = m_selectionEnd;
		end = m_selectionEnd;
	}
	if (end < 0 || end > length)
	{
		end = length;
	}
	start = std::clamp(start, 0, length);
	const int newStart = std::min(start, end);
	const int newEnd = std::max(start, end);
	if (newStart == m_selectionStart && newEnd == m_selectionEnd)
	{
		return;
	}
	m_selectionStart = newStart;
	m_selectionEnd = newEnd;
	notifySelectionChange();
}

void Win32RichEdit::replaceSelection(const std::string& replacement)
{
	const int room = std::max(0, m_limit - (textLength() - (m_selectionEnd - m_selectionStart)));
	const std::string inserted = replacement.substr(0, static_cast<size_t>(room));
	const Win32RichCharFormat insertedFormat = m_selectionStart > 0 && m_selectionStart <= static_cast<int>(m_formats.size())
		? m_formats[static_cast<size_t>(m_selectionStart - 1)] : m_defaultFormat;

	m_window.text.replace(static_cast<size_t>(m_selectionStart), static_cast<size_t>(m_selectionEnd - m_selectionStart), inserted);
	m_formats.erase(m_formats.begin() + m_selectionStart, m_formats.begin() + m_selectionEnd);
	m_formats.insert(m_formats.begin() + m_selectionStart, inserted.size(), insertedFormat);
	const int caret = m_selectionStart + static_cast<int>(inserted.size());
	m_selectionStart = caret;
	m_selectionEnd = caret;
	m_anchor = caret;
	notifyChange();
	notifySelectionChange();
}

LRESULT Win32RichEdit::getSelection(DWORD* start, DWORD* end) const
{
	if (start != nullptr)
	{
		*start = static_cast<DWORD>(m_selectionStart);
	}
	if (end != nullptr)
	{
		*end = static_cast<DWORD>(m_selectionEnd);
	}
	return MAKELRESULT(m_selectionStart, m_selectionEnd);
}

LRESULT Win32RichEdit::getSelectionRange(CHARRANGE* range) const
{
	if (range == nullptr)
	{
		return 0;
	}
	range->cpMin = m_selectionStart;
	range->cpMax = m_selectionEnd;
	return 0;
}

LRESULT Win32RichEdit::copySelectedText(LPSTR buffer) const
{
	if (buffer == nullptr)
	{
		return 0;
	}
	const int length = m_selectionEnd - m_selectionStart;
	memcpy(buffer, m_window.text.c_str() + m_selectionStart, static_cast<size_t>(length));
	buffer[length] = '\0';
	return length;
}

LRESULT Win32RichEdit::copyTextRange(TEXTRANGEA* range) const
{
	if (range == nullptr || range->lpstrText == nullptr)
	{
		return 0;
	}
	const int start = std::clamp(static_cast<int>(range->chrg.cpMin), 0, textLength());
	const int end = range->chrg.cpMax < 0 ? textLength() : std::clamp(static_cast<int>(range->chrg.cpMax), start, textLength());
	memcpy(range->lpstrText, m_window.text.c_str() + start, static_cast<size_t>(end - start));
	range->lpstrText[end - start] = '\0';
	return end - start;
}

void Win32RichEdit::applyFormat(Win32RichCharFormat& format, const CHARFORMATA& source) const
{
	const DWORD effectMask = source.dwMask & (CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_STRIKEOUT | CFM_PROTECTED | CFM_LINK);
	format.effects = (format.effects & ~effectMask) | (source.dwEffects & effectMask);
	if ((source.dwMask & CFM_COLOR) != 0)
	{
		format.usesAutoColor = (source.dwEffects & CFE_AUTOCOLOR) != 0;
		format.color = source.crTextColor;
	}
}

LRESULT Win32RichEdit::setCharFormat(WPARAM scope, const CHARFORMATA* format)
{
	if (format == nullptr)
	{
		return FALSE;
	}
	if (scope == SCF_DEFAULT)
	{
		applyFormat(m_defaultFormat, *format);
		return TRUE;
	}

	const bool isAll = (scope & SCF_ALL) != 0;
	const int start = isAll ? 0 : m_selectionStart;
	const int end = isAll ? textLength() : m_selectionEnd;
	for (int index = start; index < end && index < static_cast<int>(m_formats.size()); ++index)
	{
		applyFormat(m_formats[static_cast<size_t>(index)], *format);
	}
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}

LRESULT Win32RichEdit::getCharFormat(WPARAM scope, CHARFORMATA* format) const
{
	if (format == nullptr)
	{
		return 0;
	}
	const bool usesSelection = scope != SCF_DEFAULT && m_selectionStart < static_cast<int>(m_formats.size());
	const Win32RichCharFormat& source = usesSelection ? m_formats[static_cast<size_t>(m_selectionStart)] : m_defaultFormat;
	format->dwMask = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_STRIKEOUT | CFM_PROTECTED | CFM_LINK | CFM_COLOR;
	format->dwEffects = source.effects | (source.usesAutoColor ? CFE_AUTOCOLOR : 0);
	format->crTextColor = source.color;
	return static_cast<LRESULT>(format->dwMask);
}

bool Win32RichEdit::isLink(int index) const
{
	return index >= 0 && index < static_cast<int>(m_formats.size()) && (m_formats[static_cast<size_t>(index)].effects & CFE_LINK) != 0;
}

CHARRANGE Win32RichEdit::linkRangeAt(int index) const
{
	CHARRANGE range = { index, index };
	while (range.cpMin > 0 && isLink(range.cpMin - 1))
	{
		--range.cpMin;
	}
	while (isLink(range.cpMax))
	{
		++range.cpMax;
	}
	return range;
}

LRESULT Win32RichEdit::notifySelectionChange()
{
	InvalidateRect(hwnd(), nullptr, TRUE);
	if (!wants(ENM_SELCHANGE))
	{
		return 0;
	}
	SELCHANGE notification = {};
	notification.chrg.cpMin = m_selectionStart;
	notification.chrg.cpMax = m_selectionEnd;
	notification.seltyp = m_selectionStart == m_selectionEnd ? SEL_EMPTY : SEL_TEXT;
	return notifyParent(notification.nmhdr, EN_SELCHANGE);
}

LRESULT Win32RichEdit::notifyLink(UINT message, WPARAM wParam, LPARAM lParam, CHARRANGE range)
{
	if (!wants(ENM_LINK))
	{
		return 0;
	}
	ENLINK notification = {};
	notification.msg = message;
	notification.wParam = wParam;
	notification.lParam = lParam;
	notification.chrg = range;
	return notifyParent(notification.nmhdr, EN_LINK);
}

LRESULT Win32RichEdit::notifyMessageFilter(UINT message, WPARAM wParam, LPARAM lParam)
{
	const bool isKeyMessage = message >= WM_KEYFIRST && message <= WM_KEYLAST;
	const bool isMouseMessage = message >= WM_MOUSEFIRST && message <= WM_MOUSELAST;
	if ((isKeyMessage && !wants(ENM_KEYEVENTS)) || (isMouseMessage && !wants(ENM_MOUSEEVENTS)) || (!isKeyMessage && !isMouseMessage))
	{
		return 0;
	}
	MSGFILTER notification = {};
	notification.msg = message;
	notification.wParam = wParam;
	notification.lParam = lParam;
	return notifyParent(notification.nmhdr, EN_MSGFILTER);
}

void Win32RichEdit::notifyChange()
{
	InvalidateRect(hwnd(), nullptr, TRUE);
	if (wants(ENM_UPDATE))
	{
		notifyCommand(EN_UPDATE);
	}
	if (wants(ENM_CHANGE))
	{
		notifyCommand(EN_CHANGE);
	}
}

LRESULT Win32RichEdit::handleTextMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = true;
	switch (message)
	{
	case WM_SETTEXT:
		setText(reinterpret_cast<LPCSTR>(lParam) != nullptr ? reinterpret_cast<LPCSTR>(lParam) : "");
		return TRUE;
	case EM_SETSEL:
		setSelection(static_cast<int>(static_cast<INT_PTR>(wParam)), static_cast<int>(lParam));
		return 0;
	case EM_GETSEL:
		return getSelection(reinterpret_cast<DWORD*>(wParam), reinterpret_cast<DWORD*>(lParam));
	case EM_EXSETSEL:
	{
		const CHARRANGE* range = reinterpret_cast<const CHARRANGE*>(lParam);
		if (range != nullptr)
		{
			setSelection(range->cpMin, range->cpMax);
		}
		return m_selectionEnd;
	}
	case EM_EXGETSEL:
		return getSelectionRange(reinterpret_cast<CHARRANGE*>(lParam));
	case EM_REPLACESEL:
		replaceSelection(reinterpret_cast<LPCSTR>(lParam) != nullptr ? reinterpret_cast<LPCSTR>(lParam) : "");
		return 0;
	case EM_GETSELTEXT:
		return copySelectedText(reinterpret_cast<LPSTR>(lParam));
	case EM_SELECTIONTYPE:
		return m_selectionStart == m_selectionEnd ? SEL_EMPTY : SEL_TEXT;
	default:
		handled = false;
		return 0;
	}
}

LRESULT Win32RichEdit::handleFormatMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = true;
	switch (message)
	{
	case EM_SETCHARFORMAT:
		return setCharFormat(wParam, reinterpret_cast<const CHARFORMATA*>(lParam));
	case EM_GETCHARFORMAT:
		return getCharFormat(wParam, reinterpret_cast<CHARFORMATA*>(lParam));
	case EM_SETEVENTMASK:
		return static_cast<LRESULT>(std::exchange(m_eventMask, static_cast<DWORD>(lParam)));
	case EM_GETEVENTMASK:
		return static_cast<LRESULT>(m_eventMask);
	case EM_SETREADONLY:
		m_window.style = wParam != 0 ? (m_window.style | ES_READONLY) : (m_window.style & ~ES_READONLY);
		return TRUE;
	case EM_SETBKGNDCOLOR:
		return static_cast<LRESULT>(std::exchange(m_backgroundColor, wParam != 0 ? CLR_DEFAULT : static_cast<COLORREF>(lParam)));
	case EM_HIDESELECTION:
		m_hidesSelection = wParam != 0;
		return 0;
	case EM_LIMITTEXT:
	case EM_EXLIMITTEXT:
		m_limit = static_cast<int>(lParam != 0 ? lParam : (wParam != 0 ? wParam : 32767));
		return 0;
	case EM_GETLIMITTEXT:
		return m_limit;
	case EM_GETTEXTRANGE:
		return copyTextRange(reinterpret_cast<TEXTRANGEA*>(lParam));
	default:
		handled = false;
		return 0;
	}
}
