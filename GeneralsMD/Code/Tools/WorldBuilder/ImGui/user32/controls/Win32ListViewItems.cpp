#include "Win32ListView.h"

#include "Win32ItemList.h"
#include "Win32Metrics.h"

#include <algorithm>
#include <cstring>
#include <strings.h>
#include <utility>

int Win32ListView::rowHeight() const
{
	return Win32Metrics::lineHeight() + 2;
}

int Win32ListView::headerHeight() const
{
	return (style() & LVS_NOCOLUMNHEADER) != 0 || (style() & LVS_TYPEMASK) != LVS_REPORT ? 0 : Win32Metrics::captionHeight();
}

LRESULT Win32ListView::notifyItemChange(UINT code, int index, UINT newState, UINT oldState)
{
	NMLISTVIEW notification = {};
	notification.iItem = index;
	notification.uNewState = newState;
	notification.uOldState = oldState;
	notification.uChanged = LVIF_STATE;
	notification.lParam = isValid(index) ? m_items[static_cast<size_t>(index)].lParam : 0;
	return notifyParent(notification.hdr, code);
}

LRESULT Win32ListView::notifyActivation(UINT code, int index, POINT point)
{
	NMITEMACTIVATE notification = {};
	notification.iItem = index;
	notification.ptAction = point;
	notification.lParam = isValid(index) ? m_items[static_cast<size_t>(index)].lParam : 0;
	return notifyParent(notification.hdr, code);
}

int Win32ListView::insertItem(const LVITEMA* item)
{
	if (item == nullptr || item->iSubItem != 0)
	{
		return -1;
	}

	Win32ListViewItem created;
	created.texts.assign(std::max<size_t>(1, m_columns.size()), std::string());
	created.texts[0] = (item->mask & LVIF_TEXT) != 0 && item->pszText != nullptr ? item->pszText : "";
	created.lParam = (item->mask & LVIF_PARAM) != 0 ? item->lParam : 0;
	created.image = (item->mask & LVIF_IMAGE) != 0 ? item->iImage : 0;
	created.state = (item->mask & LVIF_STATE) != 0 ? (item->state & item->stateMask) : 0;

	size_t position = std::min(static_cast<size_t>(std::max(0, item->iItem)), m_items.size());
	const bool sortsAscending = (style() & LVS_SORTASCENDING) != 0;
	const bool sortsDescending = (style() & LVS_SORTDESCENDING) != 0;
	if (sortsAscending || sortsDescending)
	{
		position = 0;
		while (position < m_items.size() && (Win32CompareNoCase(m_items[position].texts[0], created.texts[0]) <= 0) == sortsAscending)
		{
			++position;
		}
	}
	m_items.insert(m_items.begin() + static_cast<long>(position), created);
	notifyItemChange(LVN_INSERTITEM, static_cast<int>(position), 0, 0);
	InvalidateRect(hwnd(), nullptr, TRUE);
	return static_cast<int>(position);
}

BOOL Win32ListView::deleteItem(int index)
{
	if (!isValid(index))
	{
		return FALSE;
	}
	notifyItemChange(LVN_DELETEITEM, index, 0, 0);
	m_items.erase(m_items.begin() + index);
	m_focusedItem = m_focusedItem >= static_cast<int>(m_items.size()) ? static_cast<int>(m_items.size()) - 1 : m_focusedItem;
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}

BOOL Win32ListView::deleteAllItems()
{
	if (notifyItemChange(LVN_DELETEALLITEMS, -1, 0, 0) == 0)
	{
		for (int index = 0; index < static_cast<int>(m_items.size()); ++index)
		{
			notifyItemChange(LVN_DELETEITEM, index, 0, 0);
		}
	}
	m_items.clear();
	m_focusedItem = -1;
	m_anchorItem = -1;
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}

BOOL Win32ListView::getItem(LVITEMA* item) const
{
	if (item == nullptr || !isValid(item->iItem))
	{
		return FALSE;
	}

	const Win32ListViewItem& source = m_items[static_cast<size_t>(item->iItem)];
	if ((item->mask & LVIF_TEXT) != 0 && item->pszText != nullptr && item->cchTextMax > 0)
	{
		const size_t column = static_cast<size_t>(std::max(0, item->iSubItem));
		strlcpy(item->pszText, column < source.texts.size() ? source.texts[column].c_str() : "", static_cast<size_t>(item->cchTextMax));
	}
	item->lParam = (item->mask & LVIF_PARAM) != 0 ? source.lParam : item->lParam;
	item->iImage = (item->mask & LVIF_IMAGE) != 0 ? source.image : item->iImage;
	item->state = (item->mask & LVIF_STATE) != 0 ? (source.state & item->stateMask) : item->state;
	return TRUE;
}

BOOL Win32ListView::setItem(const LVITEMA* item)
{
	if (item == nullptr || !isValid(item->iItem))
	{
		return FALSE;
	}

	Win32ListViewItem& target = m_items[static_cast<size_t>(item->iItem)];
	if ((item->mask & LVIF_TEXT) != 0)
	{
		setItemText(item->iItem, item);
	}
	target.lParam = (item->mask & LVIF_PARAM) != 0 ? item->lParam : target.lParam;
	target.image = (item->mask & LVIF_IMAGE) != 0 ? item->iImage : target.image;
	if ((item->mask & LVIF_STATE) != 0)
	{
		setItemState(item->iItem, item->state, item->stateMask);
	}
	return TRUE;
}

int Win32ListView::getItemText(int index, LVITEMA* item) const
{
	if (item == nullptr || !isValid(index) || item->pszText == nullptr || item->cchTextMax <= 0)
	{
		return 0;
	}
	const Win32ListViewItem& source = m_items[static_cast<size_t>(index)];
	const size_t column = static_cast<size_t>(std::max(0, item->iSubItem));
	strlcpy(item->pszText, column < source.texts.size() ? source.texts[column].c_str() : "", static_cast<size_t>(item->cchTextMax));
	return static_cast<int>(strlen(item->pszText));
}

BOOL Win32ListView::setItemText(int index, const LVITEMA* item)
{
	if (item == nullptr || !isValid(index))
	{
		return FALSE;
	}
	std::vector<std::string>& texts = m_items[static_cast<size_t>(index)].texts;
	const size_t column = static_cast<size_t>(std::max(0, item->iSubItem));
	if (column >= texts.size())
	{
		texts.resize(column + 1);
	}
	texts[column] = item->pszText != nullptr ? item->pszText : "";
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}

int Win32ListView::insertColumn(int index, const LVCOLUMNA* column)
{
	if (column == nullptr)
	{
		return -1;
	}
	Win32ListViewColumn created;
	created.text = (column->mask & LVCF_TEXT) != 0 && column->pszText != nullptr ? column->pszText : "";
	created.width = (column->mask & LVCF_WIDTH) != 0 ? column->cx : 50;
	created.format = (column->mask & LVCF_FMT) != 0 ? column->fmt : LVCFMT_LEFT;
	const size_t position = std::min(static_cast<size_t>(std::max(0, index)), m_columns.size());
	m_columns.insert(m_columns.begin() + static_cast<long>(position), created);
	for (Win32ListViewItem& item : m_items)
	{
		item.texts.resize(std::max(item.texts.size(), m_columns.size()));
	}
	return static_cast<int>(position);
}

int Win32ListView::nextItem(int start, UINT flags) const
{
	for (int index = start + 1; index < static_cast<int>(m_items.size()); ++index)
	{
		const UINT state = m_items[static_cast<size_t>(index)].state;
		const bool matchesSelected = (flags & LVNI_SELECTED) == 0 || (state & LVIS_SELECTED) != 0;
		const bool matchesFocused = (flags & LVNI_FOCUSED) == 0 || (state & LVIS_FOCUSED) != 0;
		if (matchesSelected && matchesFocused)
		{
			return index;
		}
	}
	return -1;
}

BOOL Win32ListView::setItemState(int index, UINT state, UINT mask)
{
	if (index == -1)
	{
		for (int item = 0; item < static_cast<int>(m_items.size()); ++item)
		{
			setItemState(item, state, mask);
		}
		return TRUE;
	}
	if (!isValid(index))
	{
		return FALSE;
	}

	Win32ListViewItem& item = m_items[static_cast<size_t>(index)];
	const UINT oldState = item.state;
	const UINT newState = (oldState & ~mask) | (state & mask);
	if (newState == oldState || notifyItemChange(LVN_ITEMCHANGING, index, newState, oldState) != 0)
	{
		return TRUE;
	}
	const bool isSingleSelection = (style() & LVS_SINGLESEL) != 0 && (newState & LVIS_SELECTED) != 0 && (oldState & LVIS_SELECTED) == 0;
	if (isSingleSelection)
	{
		for (int other = 0; other < static_cast<int>(m_items.size()); ++other)
		{
			if (other != index && (m_items[static_cast<size_t>(other)].state & LVIS_SELECTED) != 0)
			{
				setItemState(other, 0, LVIS_SELECTED);
			}
		}
	}
	item.state = newState;
	m_focusedItem = (newState & LVIS_FOCUSED) != 0 ? index : m_focusedItem;
	notifyItemChange(LVN_ITEMCHANGED, index, newState, oldState);
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}

int Win32ListView::findItem(int start, const LVFINDINFOA* info) const
{
	if (info == nullptr)
	{
		return -1;
	}
	for (int index = start + 1; index < static_cast<int>(m_items.size()); ++index)
	{
		const Win32ListViewItem& item = m_items[static_cast<size_t>(index)];
		const bool matchesParam = (info->flags & LVFI_PARAM) != 0 && item.lParam == info->lParam;
		const bool isPartial = (info->flags & LVFI_PARTIAL) != 0;
		const bool matchesText = (info->flags & LVFI_STRING) != 0 && info->psz != nullptr
			&& (isPartial ? strncasecmp(item.texts[0].c_str(), info->psz, strlen(info->psz)) == 0 : strcasecmp(item.texts[0].c_str(), info->psz) == 0);
		if (matchesParam || matchesText)
		{
			return index;
		}
	}
	return -1;
}

BOOL Win32ListView::sortItems(PFNLVCOMPARE compare, LPARAM sortParam)
{
	if (compare == nullptr)
	{
		return FALSE;
	}
	std::stable_sort(m_items.begin(), m_items.end(), [&](const Win32ListViewItem& first, const Win32ListViewItem& second) {
		return compare(first.lParam, second.lParam, sortParam) < 0;
	});
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}

int Win32ListView::selectedCount() const
{
	return static_cast<int>(std::count_if(m_items.begin(), m_items.end(), [](const Win32ListViewItem& item) { return (item.state & LVIS_SELECTED) != 0; }));
}
