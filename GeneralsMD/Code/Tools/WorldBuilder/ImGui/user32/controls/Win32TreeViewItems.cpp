#include "Win32TreeView.h"

#include "Win32ItemList.h"
#include "Win32Metrics.h"

#include <algorithm>
#include <cstring>

Win32TreeView::Win32TreeView(Win32Window& window) : Win32Control(window)
{
	m_root.handle = TVI_ROOT;
	m_root.state = TVIS_EXPANDED;
}

Win32TreeNode* Win32TreeView::find(HTREEITEM handle)
{
	const auto it = m_nodes.find(handle);
	return it == m_nodes.end() ? nullptr : it->second.get();
}

Win32TreeNode* Win32TreeView::parentFor(HTREEITEM handle)
{
	return handle == nullptr || handle == TVI_ROOT ? &m_root : find(handle);
}

int Win32TreeView::itemHeight() const
{
	return m_customItemHeight > 0 ? m_customItemHeight : std::max(Win32Metrics::lineHeight(), 18);
}

std::vector<Win32TreeRow> Win32TreeView::visibleRows()
{
	std::vector<Win32TreeRow> rows;
	std::vector<Win32TreeRow> pending;
	for (auto it = m_root.children.rbegin(); it != m_root.children.rend(); ++it)
	{
		pending.push_back({ *it, 0 });
	}
	while (!pending.empty())
	{
		const Win32TreeRow row = pending.back();
		pending.pop_back();
		rows.push_back(row);
		if (!row.node->isExpanded())
		{
			continue;
		}
		for (auto it = row.node->children.rbegin(); it != row.node->children.rend(); ++it)
		{
			pending.push_back({ *it, row.depth + 1 });
		}
	}
	return rows;
}

int Win32TreeView::rowIndexOf(const Win32TreeNode* node)
{
	const std::vector<Win32TreeRow> rows = visibleRows();
	for (size_t index = 0; index < rows.size(); ++index)
	{
		if (rows[index].node == node)
		{
			return static_cast<int>(index);
		}
	}
	return -1;
}

void Win32TreeView::placeNode(Win32TreeNode* parent, Win32TreeNode* node, HTREEITEM insertAfter)
{
	std::vector<Win32TreeNode*>& siblings = parent->children;
	if (insertAfter == TVI_FIRST)
	{
		siblings.insert(siblings.begin(), node);
		return;
	}
	if (insertAfter == TVI_SORT)
	{
		auto position = siblings.begin();
		while (position != siblings.end() && Win32CompareNoCase((*position)->text, node->text) <= 0)
		{
			++position;
		}
		siblings.insert(position, node);
		return;
	}

	const auto after = std::find_if(siblings.begin(), siblings.end(), [&](Win32TreeNode* sibling) { return sibling->handle == insertAfter; });
	siblings.insert(after == siblings.end() ? siblings.end() : after + 1, node);
}

HTREEITEM Win32TreeView::insertItem(const TVINSERTSTRUCTA* insert)
{
	Win32TreeNode* parent = insert != nullptr ? parentFor(insert->hParent) : nullptr;
	if (parent == nullptr)
	{
		return nullptr;
	}

	auto node = std::make_unique<Win32TreeNode>();
	node->handle = reinterpret_cast<HTREEITEM>(m_nextHandle);
	m_nextHandle += 0x10;
	node->parent = parent;
	const TVITEMA& item = insert->item;
	const bool hasText = (item.mask & TVIF_TEXT) != 0 && item.pszText != nullptr;
	node->text = hasText ? item.pszText : "";
	node->image = (item.mask & TVIF_IMAGE) != 0 ? item.iImage : 0;
	node->selectedImage = (item.mask & TVIF_SELECTEDIMAGE) != 0 ? item.iSelectedImage : 0;
	node->lParam = (item.mask & TVIF_PARAM) != 0 ? item.lParam : 0;
	node->state = (item.mask & TVIF_STATE) != 0 ? (item.state & item.stateMask) : 0;
	node->childrenHint = (item.mask & TVIF_CHILDREN) != 0 ? item.cChildren : 0;

	const HTREEITEM handle = node->handle;
	placeNode(parent, node.get(), insert->hInsertAfter);
	m_nodes[handle] = std::move(node);
	InvalidateRect(hwnd(), nullptr, TRUE);
	return handle;
}

void Win32TreeView::deleteSubtree(Win32TreeNode* node)
{
	const std::vector<Win32TreeNode*> children = node->children;
	for (Win32TreeNode* child : children)
	{
		deleteSubtree(child);
	}

	notifyTree(TVN_DELETEITEM, 0, node, nullptr, POINT());
	std::vector<Win32TreeNode*>& siblings = node->parent->children;
	siblings.erase(std::remove(siblings.begin(), siblings.end(), node), siblings.end());
	if (m_selected == node)
	{
		m_selected = nullptr;
	}
	if (m_dropHighlight == node)
	{
		m_dropHighlight = nullptr;
	}
	if (m_scrollTarget == node)
	{
		m_scrollTarget = nullptr;
	}
	if (m_pressedNode == node)
	{
		m_pressedNode = nullptr;
	}
	m_nodes.erase(node->handle);
}

void Win32TreeView::moveSelectionAwayFrom(Win32TreeNode* node)
{
	bool selectionInside = false;
	for (Win32TreeNode* current = m_selected; current != nullptr; current = current->parent)
	{
		selectionInside = selectionInside || current == node;
	}
	if (!selectionInside)
	{
		return;
	}

	const std::vector<Win32TreeNode*>& siblings = node->parent->children;
	const auto position = std::find(siblings.begin(), siblings.end(), node);
	Win32TreeNode* next = position + 1 < siblings.end() ? *(position + 1) : (position > siblings.begin() ? *(position - 1) : nullptr);
	if (next == nullptr && node->parent != &m_root)
	{
		next = node->parent;
	}
	selectNode(next, TVC_UNKNOWN);
}

BOOL Win32TreeView::deleteItem(HTREEITEM handle)
{
	if (m_labelEdit != nullptr)
	{
		endLabelEdit(false);
	}
	if (handle == nullptr || handle == TVI_ROOT)
	{
		const std::vector<Win32TreeNode*> roots = m_root.children;
		for (Win32TreeNode* node : roots)
		{
			deleteSubtree(node);
		}
		m_selected = nullptr;
		InvalidateRect(hwnd(), nullptr, TRUE);
		return TRUE;
	}

	Win32TreeNode* node = find(handle);
	if (node == nullptr)
	{
		return FALSE;
	}
	moveSelectionAwayFrom(node);
	deleteSubtree(node);
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}

BOOL Win32TreeView::getItem(TVITEMA* item)
{
	Win32TreeNode* node = item != nullptr ? find(item->hItem) : nullptr;
	if (node == nullptr)
	{
		return FALSE;
	}

	if ((item->mask & TVIF_TEXT) != 0 && item->pszText != nullptr && item->cchTextMax > 0)
	{
		strlcpy(item->pszText, node->text.c_str(), static_cast<size_t>(item->cchTextMax));
	}
	item->iImage = (item->mask & TVIF_IMAGE) != 0 ? node->image : item->iImage;
	item->iSelectedImage = (item->mask & TVIF_SELECTEDIMAGE) != 0 ? node->selectedImage : item->iSelectedImage;
	item->lParam = (item->mask & TVIF_PARAM) != 0 ? node->lParam : item->lParam;
	item->state = (item->mask & TVIF_STATE) != 0 ? (node->state & item->stateMask) : item->state;
	item->cChildren = (item->mask & TVIF_CHILDREN) != 0 ? (node->hasChildren() ? 1 : 0) : item->cChildren;
	return TRUE;
}

BOOL Win32TreeView::setItem(const TVITEMA* item)
{
	Win32TreeNode* node = item != nullptr ? find(item->hItem) : nullptr;
	if (node == nullptr)
	{
		return FALSE;
	}

	if ((item->mask & TVIF_TEXT) != 0 && item->pszText != nullptr)
	{
		node->text = item->pszText;
	}
	node->image = (item->mask & TVIF_IMAGE) != 0 ? item->iImage : node->image;
	node->selectedImage = (item->mask & TVIF_SELECTEDIMAGE) != 0 ? item->iSelectedImage : node->selectedImage;
	node->lParam = (item->mask & TVIF_PARAM) != 0 ? item->lParam : node->lParam;
	node->state = (item->mask & TVIF_STATE) != 0 ? ((node->state & ~item->stateMask) | (item->state & item->stateMask)) : node->state;
	node->childrenHint = (item->mask & TVIF_CHILDREN) != 0 ? item->cChildren : node->childrenHint;
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}

HTREEITEM Win32TreeView::visibleNeighbour(HTREEITEM handle, int delta)
{
	const std::vector<Win32TreeRow> rows = visibleRows();
	for (size_t index = 0; index < rows.size(); ++index)
	{
		if (rows[index].node->handle != handle)
		{
			continue;
		}
		const long target = static_cast<long>(index) + delta;
		return target >= 0 && target < static_cast<long>(rows.size()) ? rows[static_cast<size_t>(target)].node->handle : nullptr;
	}
	return nullptr;
}

HTREEITEM Win32TreeView::nextItem(UINT relation, HTREEITEM handle)
{
	Win32TreeNode* node = find(handle);
	const std::vector<Win32TreeNode*>* siblings = node != nullptr ? &node->parent->children : nullptr;
	const auto position = siblings != nullptr ? std::find(siblings->begin(), siblings->end(), node) : std::vector<Win32TreeNode*>::const_iterator();
	switch (relation)
	{
	case TVGN_ROOT:
		return m_root.children.empty() ? nullptr : m_root.children.front()->handle;
	case TVGN_NEXT:
		return siblings != nullptr && position + 1 < siblings->end() ? (*(position + 1))->handle : nullptr;
	case TVGN_PREVIOUS:
		return siblings != nullptr && position > siblings->begin() ? (*(position - 1))->handle : nullptr;
	case TVGN_PARENT:
		return node != nullptr && node->parent != &m_root ? node->parent->handle : nullptr;
	case TVGN_CHILD:
	{
		Win32TreeNode* parent = parentFor(handle);
		return parent != nullptr && !parent->children.empty() ? parent->children.front()->handle : nullptr;
	}
	case TVGN_CARET:
		return m_selected != nullptr ? m_selected->handle : nullptr;
	case TVGN_DROPHILITE:
		return m_dropHighlight != nullptr ? m_dropHighlight->handle : nullptr;
	default:
		return visibleRelation(relation, handle);
	}
}

HTREEITEM Win32TreeView::visibleRelation(UINT relation, HTREEITEM handle)
{
	if (relation == TVGN_NEXTVISIBLE || relation == TVGN_PREVIOUSVISIBLE)
	{
		return visibleNeighbour(handle, relation == TVGN_NEXTVISIBLE ? 1 : -1);
	}

	const std::vector<Win32TreeRow> rows = visibleRows();
	if (rows.empty())
	{
		return nullptr;
	}
	if (relation == TVGN_LASTVISIBLE)
	{
		return rows.back().node->handle;
	}
	if (relation != TVGN_FIRSTVISIBLE)
	{
		return nullptr;
	}
	const size_t first = static_cast<size_t>(std::max(0.0f, m_scrollY) / static_cast<float>(std::max(1, itemHeight())));
	return first < rows.size() ? rows[first].node->handle : rows.back().node->handle;
}

BOOL Win32TreeView::sortChildren(HTREEITEM handle, bool recurse, PFNTVCOMPARE compare, LPARAM sortParam)
{
	Win32TreeNode* parent = parentFor(handle);
	if (parent == nullptr)
	{
		return FALSE;
	}

	std::stable_sort(parent->children.begin(), parent->children.end(), [&](const Win32TreeNode* first, const Win32TreeNode* second) {
		return compare != nullptr ? compare(first->lParam, second->lParam, sortParam) < 0 : Win32CompareNoCase(first->text, second->text) < 0;
	});
	if (recurse)
	{
		for (Win32TreeNode* child : parent->children)
		{
			sortChildren(child->handle, true, compare, sortParam);
		}
	}
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}
