#include "Win32TreeView.h"

#include <algorithm>
#include <cstring>
#include <utility>

void Win32TreeView::fillNotifyItem(TVITEMA& item, const Win32TreeNode* node) const
{
	item = TVITEMA();
	if (node == nullptr)
	{
		return;
	}
	item.mask = TVIF_HANDLE | TVIF_STATE | TVIF_PARAM;
	item.hItem = node->handle;
	item.state = node->state;
	item.stateMask = 0xFFFF;
	item.lParam = node->lParam;
}

LRESULT Win32TreeView::notifyTree(UINT code, UINT action, const Win32TreeNode* oldNode, const Win32TreeNode* newNode, POINT dragPoint)
{
	NMTREEVIEWA notification = {};
	notification.action = action;
	fillNotifyItem(notification.itemOld, oldNode);
	fillNotifyItem(notification.itemNew, newNode);
	notification.ptDrag = dragPoint;
	return notifyParent(notification.hdr, code);
}

BOOL Win32TreeView::expandNode(Win32TreeNode* node, UINT code, bool isUserAction)
{
	if (node == nullptr || !node->hasChildren())
	{
		return FALSE;
	}

	const UINT baseCode = code & 0x3;
	const bool expands = baseCode == TVE_EXPAND || (baseCode == TVE_TOGGLE && !node->isExpanded());
	if (expands == node->isExpanded() && (code & TVE_COLLAPSERESET) == 0)
	{
		return TRUE;
	}

	const UINT action = expands ? TVE_EXPAND : TVE_COLLAPSE;
	const bool notifies = isUserAction || (node->state & TVIS_EXPANDEDONCE) == 0;
	if (notifies && notifyTree(TVN_ITEMEXPANDING, action, nullptr, node, POINT()) != 0)
	{
		return FALSE;
	}

	node->state = expands ? (node->state | TVIS_EXPANDED | TVIS_EXPANDEDONCE) : (node->state & ~TVIS_EXPANDED);
	if (!expands && (code & TVE_COLLAPSERESET) != 0)
	{
		const std::vector<Win32TreeNode*> children = node->children;
		for (Win32TreeNode* child : children)
		{
			deleteSubtree(child);
		}
		node->state &= ~TVIS_EXPANDEDONCE;
	}
	if (!expands)
	{
		for (Win32TreeNode* current = m_selected; current != nullptr; current = current->parent)
		{
			if (current->parent == node)
			{
				selectNode(node, TVC_UNKNOWN);
				break;
			}
		}
	}
	if (notifies)
	{
		notifyTree(TVN_ITEMEXPANDED, action, nullptr, node, POINT());
	}
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}

BOOL Win32TreeView::selectNode(Win32TreeNode* node, UINT action)
{
	if (node == m_selected)
	{
		if (node != nullptr)
		{
			ensureVisible(node);
		}
		return TRUE;
	}
	if (notifyTree(TVN_SELCHANGING, action, m_selected, node, POINT()) != 0)
	{
		return FALSE;
	}

	Win32TreeNode* previous = m_selected;
	if (previous != nullptr)
	{
		previous->state &= ~TVIS_SELECTED;
	}
	m_selected = node;
	if (node != nullptr)
	{
		node->state |= TVIS_SELECTED;
		ensureVisible(node);
	}
	notifyTree(TVN_SELCHANGED, action, previous, node, POINT());
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}

void Win32TreeView::ensureVisible(Win32TreeNode* node)
{
	for (Win32TreeNode* ancestor = node->parent; ancestor != nullptr && ancestor != &m_root; ancestor = ancestor->parent)
	{
		if (!ancestor->isExpanded())
		{
			expandNode(ancestor, TVE_EXPAND, false);
		}
	}
	m_scrollTarget = node;
	m_scrollTargetToTop = false;
}

void Win32TreeView::setDropHighlight(Win32TreeNode* node)
{
	if (m_dropHighlight != nullptr)
	{
		m_dropHighlight->state &= ~TVIS_DROPHILITED;
	}
	m_dropHighlight = node;
	if (node != nullptr)
	{
		node->state |= TVIS_DROPHILITED;
	}
	InvalidateRect(hwnd(), nullptr, TRUE);
}

BOOL Win32TreeView::selectItem(UINT relation, HTREEITEM handle)
{
	Win32TreeNode* node = find(handle);
	if (handle != nullptr && node == nullptr)
	{
		return FALSE;
	}
	switch (relation)
	{
	case TVGN_CARET:
		return selectNode(node, TVC_UNKNOWN);
	case TVGN_DROPHILITE:
		setDropHighlight(node);
		return TRUE;
	case TVGN_FIRSTVISIBLE:
		if (node != nullptr)
		{
			ensureVisible(node);
			m_scrollTargetToTop = true;
		}
		return TRUE;
	default:
		return FALSE;
	}
}

HTREEITEM Win32TreeView::hitTest(TVHITTESTINFO* info)
{
	if (info == nullptr)
	{
		return nullptr;
	}
	info->hItem = nullptr;
	info->flags = TVHT_NOWHERE;
	for (const auto& entry : m_nodes)
	{
		const Win32TreeNode* node = entry.second.get();
		if (!node->isLaidOut || !PtInRect(&node->rowRect, info->pt))
		{
			continue;
		}
		info->hItem = node->handle;
		info->flags = PtInRect(&node->buttonRect, info->pt) ? TVHT_ONITEMBUTTON
			: PtInRect(&node->stateRect, info->pt) ? TVHT_ONITEMSTATEICON
			: PtInRect(&node->iconRect, info->pt) ? TVHT_ONITEMICON
			: PtInRect(&node->textRect, info->pt) ? TVHT_ONITEMLABEL
			: info->pt.x < node->buttonRect.left ? TVHT_ONITEMINDENT : TVHT_ONITEMRIGHT;
		return node->handle;
	}
	return nullptr;
}

BOOL Win32TreeView::itemRect(BOOL textOnly, RECT* rect)
{
	if (rect == nullptr)
	{
		return FALSE;
	}
	HTREEITEM handle = nullptr;
	memcpy(&handle, rect, sizeof(handle));
	const Win32TreeNode* node = find(handle);
	if (node == nullptr || !node->isLaidOut)
	{
		return FALSE;
	}
	*rect = textOnly ? node->textRect : node->rowRect;
	return TRUE;
}

LRESULT Win32TreeView::handleItemMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = true;
	const HTREEITEM handle = reinterpret_cast<HTREEITEM>(lParam);
	switch (message)
	{
	case TVM_INSERTITEMA:
		return reinterpret_cast<LRESULT>(insertItem(reinterpret_cast<const TVINSERTSTRUCTA*>(lParam)));
	case TVM_DELETEITEM:
		return deleteItem(handle);
	case TVM_GETITEMA:
		return getItem(reinterpret_cast<TVITEMA*>(lParam));
	case TVM_SETITEMA:
		return setItem(reinterpret_cast<const TVITEMA*>(lParam));
	case TVM_GETCOUNT:
		return static_cast<LRESULT>(m_nodes.size());
	case TVM_GETNEXTITEM:
		return reinterpret_cast<LRESULT>(nextItem(static_cast<UINT>(wParam), handle));
	case TVM_EXPAND:
		return expandNode(find(handle), static_cast<UINT>(wParam), false);
	case TVM_SORTCHILDREN:
		return sortChildren(handle, wParam != 0, nullptr, 0);
	case TVM_SORTCHILDRENCB:
	{
		const TVSORTCB* sort = reinterpret_cast<const TVSORTCB*>(lParam);
		return sort != nullptr ? sortChildren(sort->hParent, wParam != 0, sort->lpfnCompare, sort->lParam) : FALSE;
	}
	case TVM_GETITEMSTATE:
	{
		const Win32TreeNode* node = find(reinterpret_cast<HTREEITEM>(wParam));
		return node != nullptr ? (node->state & static_cast<UINT>(lParam)) : 0;
	}
	default:
		handled = false;
		return 0;
	}
}

LRESULT Win32TreeView::handleViewMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = true;
	switch (message)
	{
	case TVM_SELECTITEM:
		return selectItem(static_cast<UINT>(wParam), reinterpret_cast<HTREEITEM>(lParam));
	case TVM_ENSUREVISIBLE:
	{
		Win32TreeNode* node = find(reinterpret_cast<HTREEITEM>(lParam));
		if (node != nullptr)
		{
			ensureVisible(node);
		}
		return node != nullptr ? TRUE : FALSE;
	}
	case TVM_HITTEST:
		return reinterpret_cast<LRESULT>(hitTest(reinterpret_cast<TVHITTESTINFO*>(lParam)));
	case TVM_GETITEMRECT:
		return itemRect(static_cast<BOOL>(wParam), reinterpret_cast<RECT*>(lParam));
	case TVM_SETIMAGELIST:
		return reinterpret_cast<LRESULT>(std::exchange(wParam == TVSIL_STATE ? m_stateImages : m_normalImages, reinterpret_cast<HIMAGELIST>(lParam)));
	case TVM_GETIMAGELIST:
		return reinterpret_cast<LRESULT>(wParam == TVSIL_STATE ? m_stateImages : m_normalImages);
	case TVM_GETINDENT:
		return m_indent;
	case TVM_SETINDENT:
		m_indent = std::max(1, static_cast<int>(wParam));
		return 0;
	case TVM_GETITEMHEIGHT:
		return itemHeight();
	case TVM_SETITEMHEIGHT:
		return std::exchange(m_customItemHeight, static_cast<int>(static_cast<short>(wParam)));
	case TVM_SETBKCOLOR:
		return static_cast<LRESULT>(std::exchange(m_backgroundColor, static_cast<COLORREF>(lParam)));
	case TVM_SETTEXTCOLOR:
		return static_cast<LRESULT>(std::exchange(m_textColor, static_cast<COLORREF>(lParam)));
	case TVM_GETVISIBLECOUNT:
		return m_window.height() / std::max(1, itemHeight());
	case TVM_EDITLABELA:
		return reinterpret_cast<LRESULT>(beginLabelEdit(reinterpret_cast<HTREEITEM>(lParam)));
	case TVM_GETEDITCONTROL:
		return reinterpret_cast<LRESULT>(m_labelEdit);
	case TVM_ENDEDITLABELNOW:
		endLabelEdit(wParam == 0);
		return TRUE;
	default:
		handled = false;
		return 0;
	}
}

LRESULT Win32TreeView::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
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
	case WM_MOUSEMOVE:
	case WM_CAPTURECHANGED:
		return handleMouse(message, wParam, lParam);
	case WM_MOUSEWHEEL:
		return 0;
	case WM_KEYDOWN:
		return handleKey(wParam);
	case WM_CHAR:
		return handleCharacter(static_cast<char>(wParam));
	case WM_GETDLGCODE:
		return DLGC_WANTARROWS | DLGC_WANTCHARS;
	case WM_SETFOCUS:
	case WM_KILLFOCUS:
	{
		NMHDR header = {};
		notifyParent(header, message == WM_SETFOCUS ? NM_SETFOCUS : NM_KILLFOCUS);
		return 0;
	}
	case WM_DESTROY:
		deleteItem(TVI_ROOT);
		return 0;
	default:
		return Win32Control::handleMessage(message, wParam, lParam);
	}
}
