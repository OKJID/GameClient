#include "Win32ListBox.h"

#include "Win32ControlDrawing.h"
#include "Win32Metrics.h"

#include <algorithm>
#include <cctype>

int Win32ListBox::itemHeight() const
{
	return m_customItemHeight > 0 ? m_customItemHeight : Win32Metrics::lineHeight();
}

void Win32ListBox::notifySelectionChange()
{
	if (notifies())
	{
		notifyCommand(LBN_SELCHANGE);
	}
}

LRESULT Win32ListBox::setCurrentSelection(int index)
{
	if (isMultiSelect())
	{
		return LB_ERR;
	}
	for (int item = 0; item < m_items.count(); ++item)
	{
		m_items.at(item).isSelected = item == index;
	}
	m_caret = m_items.isValid(index) ? index : -1;
	m_wantsScroll = m_caret >= 0;
	return m_caret >= 0 ? m_caret : LB_ERR;
}

LRESULT Win32ListBox::setSelected(bool isSelected, int index)
{
	if (!isMultiSelect())
	{
		return LB_ERR;
	}
	if (index == -1)
	{
		for (int item = 0; item < m_items.count(); ++item)
		{
			m_items.at(item).isSelected = isSelected;
		}
		return 0;
	}
	if (!m_items.isValid(index))
	{
		return LB_ERR;
	}
	m_items.at(index).isSelected = isSelected;
	m_caret = index;
	return 0;
}

LRESULT Win32ListBox::selectRange(bool isSelected, int first, int last)
{
	for (int index = std::max(0, first); index <= last && index < m_items.count(); ++index)
	{
		m_items.at(index).isSelected = isSelected;
	}
	return 0;
}

int Win32ListBox::selectedCount() const
{
	int count = 0;
	for (int index = 0; index < m_items.count(); ++index)
	{
		count += m_items.at(index).isSelected ? 1 : 0;
	}
	return count;
}

LRESULT Win32ListBox::copySelectedIndexes(int capacity, int* indexes) const
{
	int copied = 0;
	for (int index = 0; index < m_items.count() && copied < capacity; ++index)
	{
		if (m_items.at(index).isSelected)
		{
			indexes[copied++] = index;
		}
	}
	return copied;
}

LRESULT Win32ListBox::itemRect(int index, RECT* rect) const
{
	if (!m_items.isValid(index) || rect == nullptr)
	{
		return LB_ERR;
	}
	RECT client;
	GetClientRect(hwnd(), &client);
	const int top = (index - m_topIndex) * itemHeight();
	SetRect(rect, 0, top, client.right, top + itemHeight());
	return 0;
}

LRESULT Win32ListBox::itemFromPoint(LPARAM point) const
{
	const int y = GET_Y_LPARAM(point);
	const int index = m_topIndex + y / std::max(1, itemHeight());
	const bool isOutside = y < 0 || index >= m_items.count();
	const int clamped = std::clamp(index, 0, std::max(0, m_items.count() - 1));
	return MAKELONG(clamped, isOutside ? 1 : 0);
}

LRESULT Win32ListBox::deleteItem(int index)
{
	const int remaining = m_items.remove(index);
	if (remaining < 0)
	{
		return LB_ERR;
	}
	m_caret = std::min(m_caret, m_items.count() - 1);
	m_anchor = std::min(m_anchor, m_items.count() - 1);
	return remaining;
}

LRESULT Win32ListBox::resetContent()
{
	m_items.clear();
	m_caret = -1;
	m_anchor = -1;
	m_topIndex = 0;
	return 0;
}

LRESULT Win32ListBox::setItemData(int index, DWORD_PTR data)
{
	if (!m_items.isValid(index))
	{
		return LB_ERR;
	}
	m_items.at(index).data = data;
	return 0;
}

LRESULT Win32ListBox::handleItemMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = true;
	const char* text = reinterpret_cast<const char*>(lParam);
	const int index = static_cast<int>(static_cast<INT_PTR>(wParam));
	switch (message)
	{
	case LB_ADDSTRING:
		return m_items.add(text != nullptr ? text : "", (style() & LBS_SORT) != 0);
	case LB_INSERTSTRING:
		return m_items.insert(index, text != nullptr ? text : "");
	case LB_DELETESTRING:
		return deleteItem(index);
	case LB_RESETCONTENT:
		return resetContent();
	case LB_GETCOUNT:
		return m_items.count();
	case LB_GETTEXT:
	{
		const int length = m_items.copyText(index, reinterpret_cast<LPSTR>(lParam));
		return length < 0 ? LB_ERR : length;
	}
	case LB_GETTEXTLEN:
	{
		const int length = m_items.textLength(index);
		return length < 0 ? LB_ERR : length;
	}
	case LB_FINDSTRING:
	case LB_FINDSTRINGEXACT:
	{
		const int found = m_items.find(index, text != nullptr ? text : "", message == LB_FINDSTRINGEXACT);
		return found < 0 ? LB_ERR : found;
	}
	case LB_GETITEMDATA:
		return m_items.isValid(index) ? static_cast<LRESULT>(m_items.at(index).data) : LB_ERR;
	case LB_SETITEMDATA:
		return setItemData(index, static_cast<DWORD_PTR>(lParam));
	default:
		handled = false;
		return 0;
	}
}

LRESULT Win32ListBox::handleSelectionMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = true;
	const int index = static_cast<int>(static_cast<INT_PTR>(wParam));
	switch (message)
	{
	case LB_SETCURSEL:
		return setCurrentSelection(index);
	case LB_GETCURSEL:
		return m_caret >= 0 ? m_caret : LB_ERR;
	case LB_SETSEL:
		return setSelected(wParam != 0, static_cast<int>(lParam));
	case LB_GETSEL:
		return m_items.isValid(index) ? (m_items.at(index).isSelected ? 1 : 0) : LB_ERR;
	case LB_GETSELCOUNT:
		return isMultiSelect() ? selectedCount() : LB_ERR;
	case LB_GETSELITEMS:
		return isMultiSelect() ? copySelectedIndexes(index, reinterpret_cast<int*>(lParam)) : LB_ERR;
	case LB_SELITEMRANGE:
		return selectRange(wParam != 0, LOWORD(lParam), HIWORD(lParam));
	case LB_SELECTSTRING:
	{
		const int found = m_items.find(index, reinterpret_cast<const char*>(lParam) != nullptr ? reinterpret_cast<const char*>(lParam) : "", false);
		return found < 0 ? LB_ERR : setCurrentSelection(found);
	}
	case LB_GETCARETINDEX:
		return m_caret;
	case LB_SETCARETINDEX:
		m_caret = m_items.isValid(index) ? index : m_caret;
		return m_items.isValid(index) ? 0 : LB_ERR;
	case LB_GETTOPINDEX:
		return m_topIndex;
	case LB_SETTOPINDEX:
		m_topIndex = std::clamp(index, 0, std::max(0, m_items.count() - 1));
		m_wantsScroll = true;
		return 0;
	case LB_GETITEMRECT:
		return itemRect(index, reinterpret_cast<RECT*>(lParam));
	case LB_GETITEMHEIGHT:
		return itemHeight();
	case LB_SETITEMHEIGHT:
		m_customItemHeight = static_cast<int>(LOWORD(lParam));
		return 0;
	case LB_ITEMFROMPOINT:
		return itemFromPoint(lParam);
	case LB_SETHORIZONTALEXTENT:
		m_horizontalExtent = std::max(0, static_cast<int>(wParam));
		return 0;
	case LB_GETHORIZONTALEXTENT:
		return m_horizontalExtent;
	default:
		handled = false;
		return 0;
	}
}

void Win32ListBox::selectByUser(int index, bool toggle, bool extend)
{
	if ((style() & LBS_NOSEL) != 0 || !m_items.isValid(index))
	{
		return;
	}

	takeFocus();
	if (!isMultiSelect())
	{
		const bool changes = m_caret != index;
		setCurrentSelection(index);
		if (changes)
		{
			notifySelectionChange();
		}
		return;
	}

	const bool usesExtended = (style() & LBS_EXTENDEDSEL) != 0;
	if (usesExtended && extend && m_anchor >= 0)
	{
		setSelected(false, -1);
		selectRange(true, std::min(m_anchor, index), std::max(m_anchor, index));
	}
	else if (usesExtended && !toggle)
	{
		setSelected(false, -1);
		m_items.at(index).isSelected = true;
		m_anchor = index;
	}
	else
	{
		m_items.at(index).isSelected = !m_items.at(index).isSelected;
		m_anchor = index;
	}
	m_caret = index;
	notifySelectionChange();
}

void Win32ListBox::moveCaret(int delta)
{
	if (m_items.count() == 0)
	{
		return;
	}
	const int target = std::clamp(m_caret < 0 ? 0 : m_caret + delta, 0, m_items.count() - 1);
	selectByUser(target, false, false);
	m_wantsScroll = true;
}

void Win32ListBox::typeAhead(char symbol)
{
	const int found = m_items.find(m_caret, std::string(1, symbol), false);
	if (found >= 0)
	{
		selectByUser(found, false, false);
		m_wantsScroll = true;
	}
}

LRESULT Win32ListBox::handleKey(WPARAM key)
{
	const int page = std::max(1, m_window.height() / std::max(1, itemHeight()) - 1);
	switch (key)
	{
	case VK_UP:
		moveCaret(-1);
		return 0;
	case VK_DOWN:
		moveCaret(1);
		return 0;
	case VK_PRIOR:
		moveCaret(-page);
		return 0;
	case VK_NEXT:
		moveCaret(page);
		return 0;
	case VK_HOME:
		moveCaret(-m_items.count());
		return 0;
	case VK_END:
		moveCaret(m_items.count());
		return 0;
	default:
		return 0;
	}
}

LRESULT Win32ListBox::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	bool handled = false;
	LRESULT result = handleItemMessage(message, wParam, lParam, handled);
	if (handled)
	{
		return result;
	}
	result = handleSelectionMessage(message, wParam, lParam, handled);
	if (handled)
	{
		return result;
	}

	switch (message)
	{
	case WM_LBUTTONDOWN:
		selectByUser(indexAtClientY(GET_Y_LPARAM(lParam)), (wParam & MK_CONTROL) != 0, (wParam & MK_SHIFT) != 0);
		return 0;
	case WM_LBUTTONDBLCLK:
		selectByUser(indexAtClientY(GET_Y_LPARAM(lParam)), false, false);
		if (notifies())
		{
			notifyCommand(LBN_DBLCLK);
		}
		return 0;
	case WM_GETDLGCODE:
		return DLGC_WANTARROWS | DLGC_WANTCHARS;
	case WM_KEYDOWN:
		return handleKey(wParam);
	case WM_CHAR:
		if (std::isprint(static_cast<unsigned char>(wParam)))
		{
			typeAhead(static_cast<char>(wParam));
		}
		return 0;
	case WM_SETFOCUS:
		notifyCommand(LBN_SETFOCUS);
		return 0;
	case WM_KILLFOCUS:
		notifyCommand(LBN_KILLFOCUS);
		return 0;
	default:
		return Win32Control::handleMessage(message, wParam, lParam);
	}
}

int Win32ListBox::indexAtClientY(int y) const
{
	return static_cast<int>((static_cast<float>(y) - m_contentTop + m_scrollY) / static_cast<float>(std::max(1, itemHeight())));
}

void Win32ListBox::drawRows(const Win32DrawContext& context, float rowHeight)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const bool scrollsHorizontally = (style() & WS_HSCROLL) != 0;
	const float width = std::max(ImGui::GetContentRegionAvail().x, scrollsHorizontally ? static_cast<float>(m_horizontalExtent) : 0.0f);
	const int first = std::max(0, static_cast<int>(m_scrollY / rowHeight));
	const int last = std::min(m_items.count(), first + static_cast<int>(ImGui::GetWindowHeight() / rowHeight) + 2);
	const ImU32 textColor = Win32ControlDrawing::color(context.isEnabled ? ImGuiCol_Text : ImGuiCol_TextDisabled);
	for (int index = first; index < last; ++index)
	{
		const Win32ListItem& item = m_items.at(index);
		const ImVec2 rowMin(origin.x, origin.y + index * rowHeight);
		const ImVec2 rowMax(rowMin.x + width, rowMin.y + rowHeight);
		if (item.isSelected)
		{
			drawList->AddRectFilled(rowMin, rowMax, Win32ControlDrawing::color(ImGuiCol_Header));
		}
		if (index == m_caret && context.hasFocus)
		{
			drawList->AddRect(rowMin, rowMax, Win32ControlDrawing::color(ImGuiCol_NavCursor));
		}
		drawList->AddText(ImVec2(rowMin.x + 2.0f, rowMin.y + (rowHeight - ImGui::GetFontSize()) * 0.5f), textColor, item.text.c_str());
	}
	ImGui::Dummy(ImVec2(width, rowHeight * static_cast<float>(m_items.count())));
}

void Win32ListBox::draw(const Win32DrawContext& context)
{
	Win32ControlDrawing::placeCursor(context);
	const bool hasBorder = (style() & WS_BORDER) != 0 || (m_window.exStyle & WS_EX_CLIENTEDGE) != 0;
	const float rowHeight = static_cast<float>(itemHeight());
	ImGui::PushStyleColor(ImGuiCol_ChildBg, Win32ControlDrawing::color(ImGuiCol_FrameBg));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1.0f, 1.0f));
	const ImGuiWindowFlags scrollFlags = (style() & WS_HSCROLL) != 0 ? ImGuiWindowFlags_HorizontalScrollbar : ImGuiWindowFlags_None;
	ImGui::BeginChild("##listbox", Win32ControlDrawing::clientSize(context), hasBorder ? ImGuiChildFlags_Borders : ImGuiChildFlags_None, scrollFlags);
	if (m_wantsScroll)
	{
		const int visibleRows = std::max(1, static_cast<int>(ImGui::GetWindowHeight() / rowHeight));
		m_topIndex = m_caret >= 0 && (m_caret < m_topIndex || m_caret >= m_topIndex + visibleRows) ? m_caret : m_topIndex;
		ImGui::SetScrollY(m_topIndex * rowHeight);
		m_wantsScroll = false;
	}

	m_scrollY = ImGui::GetScrollY();
	m_contentTop = ImGui::GetCursorScreenPos().y + m_scrollY - static_cast<float>(context.clientOrigin.y);
	drawRows(context, rowHeight);
	m_topIndex = static_cast<int>(m_scrollY / rowHeight);
	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
}
