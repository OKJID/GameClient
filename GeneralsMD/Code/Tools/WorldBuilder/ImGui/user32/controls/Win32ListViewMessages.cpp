#include "Win32ListView.h"

#include "Win32ControlDrawing.h"

#include <algorithm>
#include <utility>

int Win32ListView::hitTest(LVHITTESTINFO* info, bool withSubItem) const
{
	if (info == nullptr)
	{
		return -1;
	}
	info->flags = LVHT_NOWHERE;
	info->iItem = -1;
	info->iSubItem = 0;
	const int y = info->pt.y - headerHeight() + static_cast<int>(m_scrollY);
	const int index = y >= 0 ? y / std::max(1, rowHeight()) : -1;
	if (info->pt.y < headerHeight() || !isValid(index))
	{
		return -1;
	}

	info->iItem = index;
	info->flags = LVHT_ONITEMLABEL;
	if (withSubItem)
	{
		int left = -static_cast<int>(m_scrollX);
		for (size_t column = 0; column < m_columns.size(); ++column)
		{
			if (info->pt.x >= left && info->pt.x < left + m_columns[column].width)
			{
				info->iSubItem = static_cast<int>(column);
			}
			left += m_columns[column].width;
		}
	}
	return index;
}

BOOL Win32ListView::itemRect(int index, RECT* rect) const
{
	if (!isValid(index) || rect == nullptr)
	{
		return FALSE;
	}
	const int top = headerHeight() + index * rowHeight() - static_cast<int>(m_scrollY);
	SetRect(rect, 0, top, m_window.width(), top + rowHeight());
	return TRUE;
}

void Win32ListView::selectByUser(int index, WPARAM keys)
{
	if (!isValid(index))
	{
		return;
	}
	const bool toggles = (keys & MK_CONTROL) != 0 && (style() & LVS_SINGLESEL) == 0;
	const bool extends = (keys & MK_SHIFT) != 0 && (style() & LVS_SINGLESEL) == 0 && m_anchorItem >= 0;
	if (toggles)
	{
		const UINT state = m_items[static_cast<size_t>(index)].state;
		setItemState(index, (state & LVIS_SELECTED) != 0 ? LVIS_FOCUSED : LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		m_anchorItem = index;
		return;
	}

	for (int other = 0; other < static_cast<int>(m_items.size()); ++other)
	{
		const bool inRange = extends && other >= std::min(m_anchorItem, index) && other <= std::max(m_anchorItem, index);
		if (other != index && !inRange)
		{
			setItemState(other, 0, LVIS_SELECTED | LVIS_FOCUSED);
		}
		else if (inRange)
		{
			setItemState(other, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
	setItemState(index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_anchorItem = extends ? m_anchorItem : index;
}

LRESULT Win32ListView::handleMouse(UINT message, WPARAM wParam, LPARAM lParam)
{
	LVHITTESTINFO info = {};
	info.pt.x = GET_X_LPARAM(lParam);
	info.pt.y = GET_Y_LPARAM(lParam);
	const int index = hitTest(&info, true);
	switch (message)
	{
	case WM_LBUTTONDOWN:
	{
		takeFocus();
		if (info.pt.y < headerHeight())
		{
			NMLISTVIEW notification = {};
			hitTest(&info, true);
			notification.iSubItem = info.iSubItem;
			notifyParent(notification.hdr, LVN_COLUMNCLICK);
			return 0;
		}
		selectByUser(index, wParam);
		m_pressedItem = index;
		return 0;
	}
	case WM_LBUTTONUP:
		notifyActivation(NM_CLICK, m_pressedItem, info.pt);
		m_pressedItem = -1;
		return 0;
	case WM_LBUTTONDBLCLK:
		selectByUser(index, wParam);
		notifyActivation(NM_DBLCLK, index, info.pt);
		return 0;
	case WM_RBUTTONDOWN:
		takeFocus();
		if (isValid(index) && (m_items[static_cast<size_t>(index)].state & LVIS_SELECTED) == 0)
		{
			selectByUser(index, 0);
		}
		return 0;
	case WM_RBUTTONUP:
		notifyActivation(NM_RCLICK, index, info.pt);
		return 0;
	default:
		return 0;
	}
}

LRESULT Win32ListView::handleKey(WPARAM key)
{
	NMLVKEYDOWN keyDown = {};
	keyDown.wVKey = static_cast<WORD>(key);
	notifyParent(keyDown.hdr, LVN_KEYDOWN);

	const int count = static_cast<int>(m_items.size());
	const int page = std::max(1, (m_window.height() - headerHeight()) / std::max(1, rowHeight()) - 1);
	int target = m_focusedItem;
	switch (key)
	{
	case VK_UP: target = m_focusedItem - 1; break;
	case VK_DOWN: target = m_focusedItem + 1; break;
	case VK_PRIOR: target = m_focusedItem - page; break;
	case VK_NEXT: target = m_focusedItem + page; break;
	case VK_HOME: target = 0; break;
	case VK_END: target = count - 1; break;
	default: return 0;
	}
	target = std::clamp(target, 0, std::max(0, count - 1));
	selectByUser(target, 0);
	m_scrollTarget = target;
	return 0;
}

LRESULT Win32ListView::handleItemMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = true;
	const int index = static_cast<int>(static_cast<INT_PTR>(wParam));
	switch (message)
	{
	case LVM_INSERTITEMA:
		return insertItem(reinterpret_cast<const LVITEMA*>(lParam));
	case LVM_DELETEITEM:
		return deleteItem(index);
	case LVM_DELETEALLITEMS:
		return deleteAllItems();
	case LVM_GETITEMA:
		return getItem(reinterpret_cast<LVITEMA*>(lParam));
	case LVM_SETITEMA:
		return setItem(reinterpret_cast<const LVITEMA*>(lParam));
	case LVM_GETITEMTEXTA:
		return getItemText(index, reinterpret_cast<LVITEMA*>(lParam));
	case LVM_SETITEMTEXTA:
		return setItemText(index, reinterpret_cast<const LVITEMA*>(lParam));
	case LVM_GETITEMCOUNT:
		return static_cast<LRESULT>(m_items.size());
	case LVM_GETNEXTITEM:
		return nextItem(index, static_cast<UINT>(lParam));
	case LVM_SETITEMSTATE:
	{
		const LVITEMA* item = reinterpret_cast<const LVITEMA*>(lParam);
		return item != nullptr ? setItemState(index, item->state, item->stateMask) : FALSE;
	}
	case LVM_GETITEMSTATE:
		return isValid(index) ? (m_items[static_cast<size_t>(index)].state & static_cast<UINT>(lParam)) : 0;
	case LVM_GETSELECTEDCOUNT:
		return selectedCount();
	case LVM_FINDITEMA:
		return findItem(index, reinterpret_cast<const LVFINDINFOA*>(lParam));
	case LVM_SORTITEMS:
		return sortItems(reinterpret_cast<PFNLVCOMPARE>(lParam), static_cast<LPARAM>(wParam));
	default:
		handled = false;
		return 0;
	}
}

LRESULT Win32ListView::handleViewMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = true;
	const int index = static_cast<int>(static_cast<INT_PTR>(wParam));
	switch (message)
	{
	case LVM_INSERTCOLUMNA:
		return insertColumn(index, reinterpret_cast<const LVCOLUMNA*>(lParam));
	case LVM_DELETECOLUMN:
		if (index < 0 || index >= static_cast<int>(m_columns.size()))
		{
			return FALSE;
		}
		m_columns.erase(m_columns.begin() + index);
		return TRUE;
	case LVM_GETCOLUMNWIDTH:
		return index >= 0 && index < static_cast<int>(m_columns.size()) ? m_columns[static_cast<size_t>(index)].width : 0;
	case LVM_SETCOLUMNWIDTH:
		if (index < 0 || index >= static_cast<int>(m_columns.size()))
		{
			return FALSE;
		}
		m_columns[static_cast<size_t>(index)].width = static_cast<int>(static_cast<short>(LOWORD(lParam)));
		return TRUE;
	case LVM_ENSUREVISIBLE:
		m_scrollTarget = index;
		return isValid(index) ? TRUE : FALSE;
	case LVM_HITTEST:
	case LVM_SUBITEMHITTEST:
		return hitTest(reinterpret_cast<LVHITTESTINFO*>(lParam), message == LVM_SUBITEMHITTEST);
	case LVM_GETITEMRECT:
		return itemRect(index, reinterpret_cast<RECT*>(lParam));
	case LVM_REDRAWITEMS:
		InvalidateRect(hwnd(), nullptr, TRUE);
		return TRUE;
	case LVM_SETEXTENDEDLISTVIEWSTYLE:
	{
		const DWORD mask = wParam != 0 ? static_cast<DWORD>(wParam) : 0xFFFFFFFFu;
		return static_cast<LRESULT>(std::exchange(m_extendedStyle, (m_extendedStyle & ~mask) | (static_cast<DWORD>(lParam) & mask)));
	}
	case LVM_GETEXTENDEDLISTVIEWSTYLE:
		return static_cast<LRESULT>(m_extendedStyle);
	case LVM_SETIMAGELIST:
		return index >= 0 && index < 3 ? reinterpret_cast<LRESULT>(std::exchange(m_imageLists[index], reinterpret_cast<HIMAGELIST>(lParam))) : 0;
	case LVM_GETIMAGELIST:
		return index >= 0 && index < 3 ? reinterpret_cast<LRESULT>(m_imageLists[index]) : 0;
	default:
		handled = false;
		return 0;
	}
}

LRESULT Win32ListView::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	bool handled = false;
	LRESULT result = handleItemMessage(message, wParam, lParam, handled);
	if (handled)
	{
		return result;
	}
	result = handleViewMessage(message, wParam, lParam, handled);
	if (handled)
	{
		return result;
	}

	switch (message)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		return handleMouse(message, wParam, lParam);
	case WM_MOUSEWHEEL:
		return 0;
	case WM_KEYDOWN:
		return handleKey(wParam);
	case WM_GETDLGCODE:
		return DLGC_WANTARROWS | DLGC_WANTCHARS;
	case WM_SETFOCUS:
	case WM_KILLFOCUS:
	{
		NMHDR header = {};
		notifyParent(header, message == WM_SETFOCUS ? NM_SETFOCUS : NM_KILLFOCUS);
		return 0;
	}
	default:
		return Win32Control::handleMessage(message, wParam, lParam);
	}
}

void Win32ListView::drawHeader(ImDrawList* drawList, ImVec2 origin)
{
	const float height = static_cast<float>(headerHeight());
	if (height <= 0.0f)
	{
		return;
	}
	float left = origin.x - m_scrollX;
	for (const Win32ListViewColumn& column : m_columns)
	{
		const ImVec2 min(left, origin.y);
		const ImVec2 max(left + static_cast<float>(column.width), origin.y + height);
		drawList->AddRectFilled(min, max, Win32ControlDrawing::color(ImGuiCol_TableHeaderBg));
		drawList->AddRect(min, max, Win32ControlDrawing::color(ImGuiCol_TableBorderStrong));
		drawList->PushClipRect(min, max, true);
		drawList->AddText(ImVec2(min.x + 4.0f, min.y + (height - ImGui::GetFontSize()) * 0.5f), Win32ControlDrawing::color(ImGuiCol_Text), column.text.c_str());
		drawList->PopClipRect();
		left = max.x;
	}
}

void Win32ListView::drawRows(const Win32DrawContext& context, ImDrawList* drawList, ImVec2 origin)
{
	const float height = static_cast<float>(rowHeight());
	const int first = std::max(0, static_cast<int>(m_scrollY / height));
	const int last = std::min(static_cast<int>(m_items.size()), first + static_cast<int>(ImGui::GetWindowHeight() / height) + 2);
	const float fullWidth = std::max(ImGui::GetContentRegionAvail().x, 1.0f);
	for (int index = first; index < last; ++index)
	{
		const Win32ListViewItem& item = m_items[static_cast<size_t>(index)];
		const ImVec2 rowMin(origin.x, origin.y + index * height);
		const bool showsSelection = (item.state & LVIS_SELECTED) != 0 && (context.hasFocus || (style() & LVS_SHOWSELALWAYS) != 0);
		if (showsSelection)
		{
			drawList->AddRectFilled(rowMin, ImVec2(rowMin.x + fullWidth, rowMin.y + height), Win32ControlDrawing::color(ImGuiCol_Header));
		}
		float left = rowMin.x;
		for (size_t column = 0; column < std::max<size_t>(1, m_columns.size()); ++column)
		{
			const float width = m_columns.empty() ? fullWidth : static_cast<float>(m_columns[column].width);
			const ImVec2 cellMin(left, rowMin.y);
			const ImVec2 cellMax(left + width, rowMin.y + height);
			drawList->PushClipRect(cellMin, cellMax, true);
			const char* text = column < item.texts.size() ? item.texts[column].c_str() : "";
			drawList->AddText(ImVec2(cellMin.x + 4.0f, cellMin.y + (height - ImGui::GetFontSize()) * 0.5f), Win32ControlDrawing::color(ImGuiCol_Text), text);
			drawList->PopClipRect();
			left += width;
		}
	}
}

void Win32ListView::draw(const Win32DrawContext& context)
{
	const ImVec2 origin = Win32ControlDrawing::screenPoint(context, 0, 0);
	drawHeader(context.drawList, origin);

	const float header = static_cast<float>(headerHeight());
	ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + header));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, Win32ControlDrawing::color(ImGuiCol_FrameBg));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::BeginChild("##listview", ImVec2(static_cast<float>(context.clientSize.cx), std::max(1.0f, context.clientSize.cy - header)), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
	if (isValid(m_scrollTarget))
	{
		const float top = static_cast<float>(m_scrollTarget * rowHeight());
		if (top < ImGui::GetScrollY() || top > ImGui::GetScrollY() + ImGui::GetWindowHeight() - rowHeight())
		{
			ImGui::SetScrollY(top);
		}
	}
	m_scrollTarget = -1;
	m_scrollY = ImGui::GetScrollY();
	m_scrollX = ImGui::GetScrollX();

	const ImVec2 rowsOrigin = ImGui::GetCursorScreenPos();
	drawRows(context, ImGui::GetWindowDrawList(), rowsOrigin);
	float contentWidth = 0.0f;
	for (const Win32ListViewColumn& column : m_columns)
	{
		contentWidth += static_cast<float>(column.width);
	}
	ImGui::Dummy(ImVec2(contentWidth, static_cast<float>(rowHeight() * static_cast<int>(m_items.size()))));
	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
}
