#include "Win32TreeView.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <strings.h>

namespace
{

const int kDragThreshold = 4;

Win32TreeView* treeOf(HWND labelEdit)
{
	Win32Window* tree = Win32FindWindow(GetParent(labelEdit));
	return tree != nullptr ? dynamic_cast<Win32TreeView*>(tree->control.get()) : nullptr;
}

LRESULT labelEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	const WNDPROC original = reinterpret_cast<WNDPROC>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
	Win32TreeView* tree = treeOf(hwnd);
	if (message == WM_GETDLGCODE)
	{
		return CallWindowProcA(original, hwnd, message, wParam, lParam) | DLGC_WANTALLKEYS;
	}
	if (message == WM_KEYDOWN && (wParam == VK_RETURN || wParam == VK_ESCAPE) && tree != nullptr)
	{
		tree->endLabelEdit(wParam == VK_RETURN);
		return 0;
	}
	if (message == WM_CHAR && (wParam == '\r' || wParam == 0x1B))
	{
		return 0;
	}

	const LRESULT result = CallWindowProcA(original, hwnd, message, wParam, lParam);
	if (message == WM_KILLFOCUS && tree != nullptr)
	{
		tree->endLabelEdit(true);
	}
	return result;
}

POINT pointFrom(LPARAM lParam)
{
	POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	return point;
}

}

void Win32TreeView::toggleStateImage(Win32TreeNode* node)
{
	const UINT current = (node->state & TVIS_STATEIMAGEMASK) >> 12;
	const UINT next = current == 1 ? 2 : 1;
	node->state = (node->state & ~TVIS_STATEIMAGEMASK) | INDEXTOSTATEIMAGEMASK(next);
	InvalidateRect(hwnd(), nullptr, TRUE);
}

LRESULT Win32TreeView::handleLeftButtonUp(LPARAM lParam)
{
	Win32TreeNode* pressed = m_pressedNode;
	const bool wasPressed = m_isLeftPressed;
	m_isLeftPressed = false;
	m_pressedNode = nullptr;
	if (GetCapture() == hwnd())
	{
		ReleaseCapture();
	}
	if (!wasPressed)
	{
		return 0;
	}

	NMHDR header = {};
	if (notifyParent(header, NM_CLICK) != 0 || pressed == nullptr || find(pressed->handle) == nullptr)
	{
		return 0;
	}

	TVHITTESTINFO info = {};
	info.pt = pointFrom(lParam);
	hitTest(&info);
	if (info.hItem != pressed->handle)
	{
		return 0;
	}
	if ((info.flags & TVHT_ONITEMSTATEICON) != 0 && (style() & TVS_CHECKBOXES) != 0)
	{
		toggleStateImage(pressed);
		return 0;
	}
	const bool editsLabel = m_wasSelectedOnPress && (info.flags & TVHT_ONITEMLABEL) != 0 && (style() & TVS_EDITLABELS) != 0;
	if (editsLabel)
	{
		beginLabelEdit(pressed->handle);
		return 0;
	}
	selectNode(pressed, TVC_BYMOUSE);
	return 0;
}

LRESULT Win32TreeView::handleRightButtonUp(LPARAM lParam)
{
	if (!m_isRightPressed)
	{
		return 0;
	}
	m_isRightPressed = false;
	NMHDR header = {};
	if (notifyParent(header, NM_RCLICK) != 0)
	{
		return 0;
	}

	POINT screen = pointFrom(lParam);
	ClientToScreen(hwnd(), &screen);
	SendMessageA(hwnd(), WM_CONTEXTMENU, reinterpret_cast<WPARAM>(hwnd()), MAKELPARAM(screen.x, screen.y));
	return 0;
}

LRESULT Win32TreeView::handleDoubleClick(LPARAM lParam)
{
	TVHITTESTINFO info = {};
	info.pt = pointFrom(lParam);
	Win32TreeNode* node = find(hitTest(&info));
	if (node != nullptr && (info.flags & TVHT_ONITEMBUTTON) == 0)
	{
		selectNode(node, TVC_BYMOUSE);
	}

	NMHDR header = {};
	if (notifyParent(header, NM_DBLCLK) != 0 || node == nullptr)
	{
		return 0;
	}
	expandNode(node, TVE_TOGGLE, true);
	return 0;
}

LRESULT Win32TreeView::handleMouse(UINT message, WPARAM, LPARAM lParam)
{
	switch (message)
	{
	case WM_LBUTTONDOWN:
	{
		takeFocus();
		TVHITTESTINFO info = {};
		info.pt = pointFrom(lParam);
		Win32TreeNode* node = find(hitTest(&info));
		if (node != nullptr && (info.flags & TVHT_ONITEMBUTTON) != 0)
		{
			expandNode(node, TVE_TOGGLE, true);
			return 0;
		}
		m_pressedNode = node;
		m_pressPoint = info.pt;
		m_isLeftPressed = true;
		m_wasSelectedOnPress = node != nullptr && node == m_selected;
		SetCapture(hwnd());
		return 0;
	}
	case WM_LBUTTONUP:
		return handleLeftButtonUp(lParam);
	case WM_LBUTTONDBLCLK:
		return handleDoubleClick(lParam);
	case WM_RBUTTONDOWN:
		takeFocus();
		m_isRightPressed = true;
		return 0;
	case WM_RBUTTONUP:
		return handleRightButtonUp(lParam);
	case WM_MOUSEMOVE:
	{
		const POINT point = pointFrom(lParam);
		const bool movedEnough = std::abs(point.x - m_pressPoint.x) > kDragThreshold || std::abs(point.y - m_pressPoint.y) > kDragThreshold;
		if (!m_isLeftPressed || m_pressedNode == nullptr || !movedEnough || (style() & TVS_DISABLEDRAGDROP) != 0)
		{
			return 0;
		}
		Win32TreeNode* dragged = m_pressedNode;
		m_isLeftPressed = false;
		m_pressedNode = nullptr;
		ReleaseCapture();
		notifyTree(TVN_BEGINDRAG, 0, nullptr, dragged, m_pressPoint);
		return 0;
	}
	default:
		m_isLeftPressed = false;
		return 0;
	}
}

LRESULT Win32TreeView::handleKey(WPARAM key)
{
	NMTVKEYDOWN keyDown = {};
	keyDown.wVKey = static_cast<WORD>(key);
	notifyParent(keyDown.hdr, TVN_KEYDOWN);

	const int page = std::max(1, m_window.height() / std::max(1, itemHeight()) - 1);
	switch (key)
	{
	case VK_UP:
		return selectRelative(-1);
	case VK_DOWN:
		return selectRelative(1);
	case VK_PRIOR:
		return selectRelative(-page);
	case VK_NEXT:
		return selectRelative(page);
	case VK_HOME:
		return selectNode(find(nextItem(TVGN_ROOT, nullptr)), TVC_BYKEYBOARD);
	case VK_END:
		return selectNode(find(visibleRelation(TVGN_LASTVISIBLE, nullptr)), TVC_BYKEYBOARD);
	case VK_LEFT:
		return collapseOrSelectParent();
	case VK_RIGHT:
		return expandOrSelectChild();
	case VK_ADD:
	case VK_SUBTRACT:
		return m_selected != nullptr ? expandNode(m_selected, key == VK_ADD ? TVE_EXPAND : TVE_COLLAPSE, true) : 0;
	case VK_F2:
		return (style() & TVS_EDITLABELS) != 0 && m_selected != nullptr ? reinterpret_cast<LRESULT>(beginLabelEdit(m_selected->handle)) : 0;
	case VK_SPACE:
		if ((style() & TVS_CHECKBOXES) != 0 && m_selected != nullptr)
		{
			toggleStateImage(m_selected);
		}
		return 0;
	case VK_RETURN:
	{
		NMHDR header = {};
		return notifyParent(header, NM_RETURN);
	}
	default:
		return 0;
	}
}

LRESULT Win32TreeView::selectRelative(int delta)
{
	if (m_selected == nullptr)
	{
		return selectNode(find(nextItem(TVGN_ROOT, nullptr)), TVC_BYKEYBOARD);
	}

	HTREEITEM target = m_selected->handle;
	for (int step = 0; step < std::abs(delta); ++step)
	{
		const HTREEITEM next = visibleNeighbour(target, delta < 0 ? -1 : 1);
		if (next == nullptr)
		{
			break;
		}
		target = next;
	}
	return selectNode(find(target), TVC_BYKEYBOARD);
}

LRESULT Win32TreeView::collapseOrSelectParent()
{
	if (m_selected == nullptr)
	{
		return 0;
	}
	if (m_selected->isExpanded())
	{
		return expandNode(m_selected, TVE_COLLAPSE, true);
	}
	return m_selected->parent != &m_root ? selectNode(m_selected->parent, TVC_BYKEYBOARD) : 0;
}

LRESULT Win32TreeView::expandOrSelectChild()
{
	if (m_selected == nullptr)
	{
		return 0;
	}
	if (!m_selected->isExpanded() && m_selected->hasChildren())
	{
		return expandNode(m_selected, TVE_EXPAND, true);
	}
	return !m_selected->children.empty() ? selectNode(m_selected->children.front(), TVC_BYKEYBOARD) : 0;
}

LRESULT Win32TreeView::handleCharacter(char symbol)
{
	if (!std::isprint(static_cast<unsigned char>(symbol)) || symbol == ' ')
	{
		return 0;
	}

	const std::vector<Win32TreeRow> rows = visibleRows();
	const int start = rowIndexOf(m_selected);
	for (size_t step = 1; step <= rows.size(); ++step)
	{
		Win32TreeNode* candidate = rows[(static_cast<size_t>(start + 1) + step - 1) % rows.size()].node;
		if (!candidate->text.empty() && strncasecmp(candidate->text.c_str(), &symbol, 1) == 0)
		{
			selectNode(candidate, TVC_BYKEYBOARD);
			return 0;
		}
	}
	return 0;
}

HWND Win32TreeView::beginLabelEdit(HTREEITEM handle)
{
	Win32TreeNode* node = find(handle);
	if (node == nullptr)
	{
		return nullptr;
	}
	if (m_labelEdit != nullptr)
	{
		endLabelEdit(true);
	}

	NMTVDISPINFOA info = {};
	fillNotifyItem(info.item, node);
	info.item.mask |= TVIF_TEXT;
	info.item.pszText = const_cast<LPSTR>(node->text.c_str());
	info.item.cchTextMax = static_cast<int>(node->text.size()) + 1;
	if (notifyParent(info.hdr, TVN_BEGINLABELEDITA) != 0 || find(handle) == nullptr)
	{
		return nullptr;
	}

	ensureVisible(node);
	const RECT rect = node->isLaidOut ? node->textRect : RECT{ 0, 0, m_window.width(), itemHeight() };
	const int width = std::max(static_cast<int>(rect.right - rect.left) + 40, 80);
	m_labelEdit = CreateWindowExA(0, "Edit", node->text.c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
		rect.left, rect.top, width, rect.bottom - rect.top + 2, hwnd(), reinterpret_cast<HMENU>(1), nullptr, nullptr);
	m_labelEditNode = node;
	const WNDPROC original = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(m_labelEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(labelEditProc)));
	SetWindowLongPtrA(m_labelEdit, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(original));
	SetFocus(m_labelEdit);
	SendMessageA(m_labelEdit, EM_SETSEL, 0, -1);
	return m_labelEdit;
}

void Win32TreeView::endLabelEdit(bool accept)
{
	if (m_labelEdit == nullptr || m_isEndingLabelEdit)
	{
		return;
	}
	m_isEndingLabelEdit = true;

	char buffer[1024] = {};
	GetWindowTextA(m_labelEdit, buffer, sizeof(buffer));
	Win32TreeNode* node = m_labelEditNode;
	NMTVDISPINFOA info = {};
	fillNotifyItem(info.item, node);
	info.item.mask |= TVIF_TEXT;
	info.item.pszText = accept ? buffer : nullptr;
	info.item.cchTextMax = accept ? static_cast<int>(sizeof(buffer)) : 0;
	const bool keepsText = notifyParent(info.hdr, TVN_ENDLABELEDITA) != 0;
	if (accept && keepsText && find(node->handle) != nullptr)
	{
		node->text = buffer;
	}

	const HWND edit = m_labelEdit;
	m_labelEdit = nullptr;
	m_labelEditNode = nullptr;
	DestroyWindow(edit);
	m_isEndingLabelEdit = false;
	InvalidateRect(hwnd(), nullptr, TRUE);
}
