#include "Win32TabControl.h"

#include "Win32ControlDrawing.h"
#include "Win32Metrics.h"

#include <algorithm>
#include <cfloat>
#include <cstring>
#include <utility>

namespace
{

const int kTabPadding = 8;
const int kBodyMargin = 4;

}

int Win32TabControl::stripHeight() const
{
	return Win32Metrics::captionHeight() + 2;
}

void Win32TabControl::layoutTabs()
{
	int left = 2;
	for (Win32Tab& tab : m_tabs)
	{
		const float textWidth = Win32Metrics::systemFont()->CalcTextSizeA(Win32Metrics::systemFontSize(), FLT_MAX, 0.0f, tab.text.c_str()).x;
		const int width = static_cast<int>(textWidth) + kTabPadding * 2;
		SetRect(&tab.rect, left, 0, left + width, stripHeight());
		left += width;
	}
}

int Win32TabControl::insertTab(int index, const TCITEMA* item)
{
	if (item == nullptr)
	{
		return -1;
	}
	Win32Tab tab;
	tab.text = (item->mask & TCIF_TEXT) != 0 && item->pszText != nullptr ? item->pszText : "";
	tab.lParam = (item->mask & TCIF_PARAM) != 0 ? item->lParam : 0;
	tab.image = (item->mask & TCIF_IMAGE) != 0 ? item->iImage : -1;
	const size_t position = std::min(static_cast<size_t>(std::max(0, index)), m_tabs.size());
	m_tabs.insert(m_tabs.begin() + static_cast<long>(position), tab);
	m_selection = m_selection < 0 ? 0 : (static_cast<int>(position) <= m_selection ? m_selection + 1 : m_selection);
	layoutTabs();
	InvalidateRect(hwnd(), nullptr, TRUE);
	return static_cast<int>(position);
}

BOOL Win32TabControl::getTab(int index, TCITEMA* item) const
{
	if (item == nullptr || !isValid(index))
	{
		return FALSE;
	}
	const Win32Tab& tab = m_tabs[static_cast<size_t>(index)];
	if ((item->mask & TCIF_TEXT) != 0 && item->pszText != nullptr && item->cchTextMax > 0)
	{
		strlcpy(item->pszText, tab.text.c_str(), static_cast<size_t>(item->cchTextMax));
	}
	item->lParam = (item->mask & TCIF_PARAM) != 0 ? tab.lParam : item->lParam;
	item->iImage = (item->mask & TCIF_IMAGE) != 0 ? tab.image : item->iImage;
	return TRUE;
}

BOOL Win32TabControl::setTab(int index, const TCITEMA* item)
{
	if (item == nullptr || !isValid(index))
	{
		return FALSE;
	}
	Win32Tab& tab = m_tabs[static_cast<size_t>(index)];
	tab.text = (item->mask & TCIF_TEXT) != 0 && item->pszText != nullptr ? item->pszText : tab.text;
	tab.lParam = (item->mask & TCIF_PARAM) != 0 ? item->lParam : tab.lParam;
	tab.image = (item->mask & TCIF_IMAGE) != 0 ? item->iImage : tab.image;
	layoutTabs();
	return TRUE;
}

BOOL Win32TabControl::deleteTab(int index)
{
	if (!isValid(index))
	{
		return FALSE;
	}
	m_tabs.erase(m_tabs.begin() + index);
	m_selection = m_tabs.empty() ? -1 : std::min(m_selection, static_cast<int>(m_tabs.size()) - 1);
	layoutTabs();
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}

int Win32TabControl::setSelection(int index)
{
	const int previous = m_selection;
	if (!isValid(index))
	{
		return -1;
	}
	m_selection = index;
	InvalidateRect(hwnd(), nullptr, TRUE);
	return previous;
}

void Win32TabControl::selectByUser(int index)
{
	if (!isValid(index) || index == m_selection)
	{
		return;
	}
	NMHDR header = {};
	if (notifyParent(header, TCN_SELCHANGING) != 0)
	{
		return;
	}
	setSelection(index);
	notifyParent(header, TCN_SELCHANGE);
}

int Win32TabControl::hitTest(TCHITTESTINFO* info) const
{
	if (info == nullptr)
	{
		return -1;
	}
	for (size_t index = 0; index < m_tabs.size(); ++index)
	{
		if (PtInRect(&m_tabs[index].rect, info->pt))
		{
			info->flags = TCHT_ONITEMLABEL;
			return static_cast<int>(index);
		}
	}
	info->flags = TCHT_NOWHERE;
	return -1;
}

BOOL Win32TabControl::adjustRect(bool toWindow, RECT* rect) const
{
	if (rect == nullptr)
	{
		return FALSE;
	}
	if (toWindow)
	{
		rect->left -= kBodyMargin;
		rect->right += kBodyMargin;
		rect->top -= stripHeight() + kBodyMargin;
		rect->bottom += kBodyMargin;
		return TRUE;
	}
	rect->left += kBodyMargin;
	rect->right -= kBodyMargin;
	rect->top += stripHeight() + kBodyMargin;
	rect->bottom -= kBodyMargin;
	return TRUE;
}

LRESULT Win32TabControl::handleTabMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = true;
	const int index = static_cast<int>(static_cast<INT_PTR>(wParam));
	switch (message)
	{
	case TCM_INSERTITEMA:
		return insertTab(index, reinterpret_cast<const TCITEMA*>(lParam));
	case TCM_GETITEMA:
		return getTab(index, reinterpret_cast<TCITEMA*>(lParam));
	case TCM_SETITEMA:
		return setTab(index, reinterpret_cast<const TCITEMA*>(lParam));
	case TCM_DELETEITEM:
		return deleteTab(index);
	case TCM_DELETEALLITEMS:
		m_tabs.clear();
		m_selection = -1;
		return TRUE;
	case TCM_GETITEMCOUNT:
		return static_cast<LRESULT>(m_tabs.size());
	case TCM_GETCURSEL:
	case TCM_GETCURFOCUS:
		return m_selection;
	case TCM_SETCURSEL:
	case TCM_SETCURFOCUS:
		return setSelection(index);
	case TCM_GETITEMRECT:
		if (!isValid(index) || reinterpret_cast<RECT*>(lParam) == nullptr)
		{
			return FALSE;
		}
		*reinterpret_cast<RECT*>(lParam) = m_tabs[static_cast<size_t>(index)].rect;
		return TRUE;
	case TCM_HITTEST:
		return hitTest(reinterpret_cast<TCHITTESTINFO*>(lParam));
	case TCM_ADJUSTRECT:
		return adjustRect(wParam != 0, reinterpret_cast<RECT*>(lParam));
	case TCM_SETIMAGELIST:
		return reinterpret_cast<LRESULT>(std::exchange(m_imageList, reinterpret_cast<HIMAGELIST>(lParam)));
	case TCM_GETIMAGELIST:
		return reinterpret_cast<LRESULT>(m_imageList);
	default:
		handled = false;
		return 0;
	}
}

LRESULT Win32TabControl::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	bool handled = false;
	const LRESULT result = handleTabMessage(message, wParam, lParam, handled);
	if (handled)
	{
		return result;
	}

	switch (message)
	{
	case WM_LBUTTONDOWN:
	{
		takeFocus();
		TCHITTESTINFO info = {};
		info.pt.x = GET_X_LPARAM(lParam);
		info.pt.y = GET_Y_LPARAM(lParam);
		selectByUser(hitTest(&info));
		return 0;
	}
	case WM_KEYDOWN:
		if (wParam == VK_LEFT || wParam == VK_RIGHT)
		{
			selectByUser(m_selection + (wParam == VK_LEFT ? -1 : 1));
		}
		return 0;
	case WM_GETDLGCODE:
		return DLGC_WANTARROWS;
	default:
		return Win32Control::handleMessage(message, wParam, lParam);
	}
}

void Win32TabControl::draw(const Win32DrawContext& context)
{
	layoutTabs();
	const ImVec2 origin = Win32ControlDrawing::screenPoint(context, 0, 0);
	const ImVec2 size = Win32ControlDrawing::clientSize(context);
	const float strip = static_cast<float>(stripHeight());
	context.drawList->AddRect(ImVec2(origin.x, origin.y + strip), ImVec2(origin.x + size.x, origin.y + size.y), Win32ControlDrawing::color(ImGuiCol_Border));
	for (size_t index = 0; index < m_tabs.size(); ++index)
	{
		const Win32Tab& tab = m_tabs[index];
		const bool isSelected = static_cast<int>(index) == m_selection;
		const ImVec2 min(origin.x + tab.rect.left, origin.y + tab.rect.top + (isSelected ? 0.0f : 2.0f));
		const ImVec2 max(origin.x + tab.rect.right, origin.y + tab.rect.bottom);
		context.drawList->AddRectFilled(min, max, Win32ControlDrawing::color(isSelected ? ImGuiCol_TabSelected : ImGuiCol_Tab), ImGui::GetStyle().TabRounding, ImDrawFlags_RoundCornersTop);
		const float textX = min.x + kTabPadding;
		const float textY = min.y + (max.y - min.y - ImGui::GetFontSize()) * 0.5f;
		context.drawList->AddText(ImVec2(textX, textY), Win32ControlDrawing::color(context.isEnabled ? ImGuiCol_Text : ImGuiCol_TextDisabled), tab.text.c_str());
	}
}
