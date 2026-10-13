#include "Win32ComboBox.h"

#include "Win32ControlDrawing.h"
#include "Win32Metrics.h"

#include <algorithm>
#include <cctype>

int Win32ComboBox::fieldHeight() const
{
	return Win32Metrics::captionHeight() + 2;
}

void Win32ComboBox::initialize(const CREATESTRUCTA* create)
{
	m_dropHeight = create != nullptr ? create->cy : 0;
	m_editorText = text();
	if (!isSimple())
	{
		SetWindowPos(hwnd(), nullptr, 0, 0, m_window.width(), fieldHeight(), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
}

LRESULT Win32ComboBox::setSelection(int index)
{
	m_selection = m_items.isValid(index) ? index : -1;
	m_window.text = m_selection >= 0 ? m_items.at(m_selection).text : "";
	m_editorText = m_window.text;
	m_needsReload = true;
	return m_selection >= 0 ? m_selection : CB_ERR;
}

LRESULT Win32ComboBox::setText(LPCSTR newText)
{
	const std::string value = newText != nullptr ? newText : "";
	if (!hasEdit())
	{
		const int found = m_items.find(-1, value, true);
		return found < 0 ? CB_ERR : (setSelection(found), TRUE);
	}
	m_window.text = value.substr(0, static_cast<size_t>(m_limit));
	m_editorText = m_window.text;
	m_needsReload = true;
	return TRUE;
}

LRESULT Win32ComboBox::getEditSelection(WPARAM wParam, LPARAM lParam) const
{
	DWORD* start = reinterpret_cast<DWORD*>(wParam);
	DWORD* end = reinterpret_cast<DWORD*>(lParam);
	if (start != nullptr)
	{
		*start = static_cast<DWORD>(m_editSelectionStart);
	}
	if (end != nullptr)
	{
		*end = static_cast<DWORD>(m_editSelectionEnd);
	}
	return MAKELRESULT(m_editSelectionStart, m_editSelectionEnd);
}

LRESULT Win32ComboBox::droppedRect(RECT* rect) const
{
	if (rect == nullptr)
	{
		return CB_ERR;
	}
	GetWindowRect(hwnd(), rect);
	rect->bottom = rect->top + std::max(m_dropHeight, fieldHeight());
	return TRUE;
}

LRESULT Win32ComboBox::deleteItem(int index)
{
	const int remaining = m_items.remove(index);
	if (remaining < 0)
	{
		return CB_ERR;
	}
	m_selection = index == m_selection ? -1 : (index < m_selection ? m_selection - 1 : m_selection);
	return remaining;
}

LRESULT Win32ComboBox::resetContent()
{
	m_items.clear();
	m_selection = -1;
	if (!hasEdit())
	{
		m_window.text.clear();
	}
	return CB_OKAY;
}

LRESULT Win32ComboBox::setItemData(int index, DWORD_PTR data)
{
	if (!m_items.isValid(index))
	{
		return CB_ERR;
	}
	m_items.at(index).data = data;
	return CB_OKAY;
}

LRESULT Win32ComboBox::handleItemMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = true;
	const char* itemText = reinterpret_cast<const char*>(lParam);
	const int index = static_cast<int>(static_cast<INT_PTR>(wParam));
	switch (message)
	{
	case CB_ADDSTRING:
		return m_items.add(itemText != nullptr ? itemText : "", (style() & CBS_SORT) != 0);
	case CB_INSERTSTRING:
		return m_items.insert(index, itemText != nullptr ? itemText : "");
	case CB_DELETESTRING:
		return deleteItem(index);
	case CB_RESETCONTENT:
		return resetContent();
	case CB_GETCOUNT:
		return m_items.count();
	case CB_GETLBTEXT:
	{
		const int length = m_items.copyText(index, reinterpret_cast<LPSTR>(lParam));
		return length < 0 ? CB_ERR : length;
	}
	case CB_GETLBTEXTLEN:
	{
		const int length = m_items.textLength(index);
		return length < 0 ? CB_ERR : length;
	}
	case CB_FINDSTRING:
	case CB_FINDSTRINGEXACT:
	{
		const int found = m_items.find(index, itemText != nullptr ? itemText : "", message == CB_FINDSTRINGEXACT);
		return found < 0 ? CB_ERR : found;
	}
	case CB_GETITEMDATA:
		return m_items.isValid(index) ? static_cast<LRESULT>(m_items.at(index).data) : CB_ERR;
	case CB_SETITEMDATA:
		return setItemData(index, static_cast<DWORD_PTR>(lParam));
	default:
		handled = false;
		return 0;
	}
}

LRESULT Win32ComboBox::handleStateMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = true;
	const int index = static_cast<int>(static_cast<INT_PTR>(wParam));
	switch (message)
	{
	case CB_GETCURSEL:
		return m_selection >= 0 ? m_selection : CB_ERR;
	case CB_SETCURSEL:
		return setSelection(index);
	case CB_SELECTSTRING:
	{
		const int found = m_items.find(index, reinterpret_cast<const char*>(lParam) != nullptr ? reinterpret_cast<const char*>(lParam) : "", false);
		return found < 0 ? CB_ERR : setSelection(found);
	}
	case CB_SHOWDROPDOWN:
		wParam != 0 ? openList() : closeList(false);
		return TRUE;
	case CB_GETDROPPEDSTATE:
		return m_isDropped ? TRUE : FALSE;
	case CB_GETDROPPEDCONTROLRECT:
		return droppedRect(reinterpret_cast<RECT*>(lParam));
	case CB_LIMITTEXT:
		m_limit = wParam != 0 ? static_cast<int>(wParam) : 0x7FFFFFFE;
		return TRUE;
	case CB_GETEDITSEL:
		return getEditSelection(wParam, lParam);
	case CB_SETEDITSEL:
		m_editSelectionStart = static_cast<short>(LOWORD(lParam));
		m_editSelectionEnd = static_cast<short>(HIWORD(lParam));
		return TRUE;
	case CB_GETITEMHEIGHT:
		return index == -1 ? fieldHeight() : Win32Metrics::lineHeight();
	case CB_GETDROPPEDWIDTH:
		return std::max(m_droppedWidth, m_window.width());
	case CB_SETDROPPEDWIDTH:
		m_droppedWidth = index;
		return m_droppedWidth;
	default:
		handled = false;
		return 0;
	}
}

void Win32ComboBox::openList()
{
	if (m_isDropped || isSimple())
	{
		return;
	}
	notifyCommand(CBN_DROPDOWN);
	m_isDropped = true;
	m_wantsPopupOpen = true;
}

void Win32ComboBox::closeList(bool accept)
{
	if (!m_isDropped)
	{
		return;
	}
	m_isDropped = false;
	notifyCommand(accept ? CBN_SELENDOK : CBN_SELENDCANCEL);
	notifyCommand(CBN_CLOSEUP);
}

void Win32ComboBox::chooseByUser(int index)
{
	takeFocus();
	const bool changes = index != m_selection;
	setSelection(index);
	if (changes)
	{
		notifyCommand(CBN_SELCHANGE);
	}
	closeList(true);
}

void Win32ComboBox::moveSelection(int delta)
{
	if (m_items.count() == 0)
	{
		return;
	}
	const int target = std::clamp(m_selection < 0 ? 0 : m_selection + delta, 0, m_items.count() - 1);
	if (target == m_selection)
	{
		return;
	}
	setSelection(target);
	notifyCommand(CBN_SELCHANGE);
}

void Win32ComboBox::applyUserEdit(const std::string& editText)
{
	if (editText == m_window.text)
	{
		return;
	}
	m_window.text = editText;
	m_selection = -1;
	notifyCommand(CBN_EDITUPDATE);
	notifyCommand(CBN_EDITCHANGE);
}

LRESULT Win32ComboBox::handleKey(WPARAM key)
{
	switch (key)
	{
	case VK_UP:
		moveSelection(-1);
		return 0;
	case VK_DOWN:
		moveSelection(1);
		return 0;
	case VK_F4:
		m_isDropped ? closeList(true) : openList();
		return 0;
	case VK_RETURN:
		closeList(true);
		return 0;
	case VK_ESCAPE:
		closeList(false);
		return 0;
	default:
		return 0;
	}
}

LRESULT Win32ComboBox::handleCharacter(char symbol)
{
	if (hasEdit())
	{
		if (m_isEditing || static_cast<unsigned char>(symbol) < 0x20)
		{
			return 0;
		}
		applyUserEdit((m_window.text + symbol).substr(0, static_cast<size_t>(m_limit)));
		m_editorText = m_window.text;
		m_needsReload = true;
		return 0;
	}

	const int found = m_items.find(m_selection, std::string(1, symbol), false);
	if (found >= 0 && found != m_selection)
	{
		setSelection(found);
		notifyCommand(CBN_SELCHANGE);
	}
	return 0;
}

LRESULT Win32ComboBox::handleFieldClick(int x)
{
	takeFocus();
	const bool isOnArrow = x >= m_window.width() - fieldHeight();
	if (isSimple() || (hasEdit() && !isOnArrow))
	{
		return 0;
	}
	m_isDropped ? closeList(false) : openList();
	return 0;
}

LRESULT Win32ComboBox::dialogCode() const
{
	const LRESULT code = DLGC_WANTARROWS | DLGC_WANTCHARS | (hasEdit() ? DLGC_HASSETSEL : 0);
	return m_isDropped ? code | DLGC_WANTALLKEYS : code;
}

LRESULT Win32ComboBox::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	bool handled = false;
	LRESULT result = handleItemMessage(message, wParam, lParam, handled);
	if (handled)
	{
		return result;
	}
	result = handleStateMessage(message, wParam, lParam, handled);
	if (handled)
	{
		return result;
	}

	switch (message)
	{
	case WM_CREATE:
		initialize(reinterpret_cast<const CREATESTRUCTA*>(lParam));
		return 0;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		return handleFieldClick(GET_X_LPARAM(lParam));
	case WM_SETTEXT:
		return setText(reinterpret_cast<LPCSTR>(lParam));
	case WM_GETDLGCODE:
		return dialogCode();
	case WM_KEYDOWN:
		return handleKey(wParam);
	case WM_CHAR:
		return handleCharacter(static_cast<char>(wParam));
	case WM_SETFOCUS:
		m_wantsEditorFocus = hasEdit() && !m_isEditing;
		notifyCommand(CBN_SETFOCUS);
		return 0;
	case WM_KILLFOCUS:
		closeList(false);
		notifyCommand(CBN_KILLFOCUS);
		return 0;
	default:
		return Win32Control::handleMessage(message, wParam, lParam);
	}
}
