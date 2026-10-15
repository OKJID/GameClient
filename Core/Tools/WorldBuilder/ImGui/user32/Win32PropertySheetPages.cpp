#include "Win32PropertySheet.h"

#include "Win32Dialog.h"
#include "Win32Resources.h"

#include <algorithm>

namespace
{

const DWORD kRemovedPageStyles = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_DISABLED | WS_VISIBLE | DS_MODALFRAME | DS_CENTER;

const Win32DialogTemplate* templateOf(const PROPSHEETPAGEA& page)
{
	UINT id = 0;
	return Win32Resources::instance().resolveName(page.pszTemplate, id) ? Win32Resources::instance().dialog(id) : nullptr;
}

Win32DialogTemplate childPageTemplate(const Win32DialogTemplate& source)
{
	Win32DialogTemplate pageTemplate = source;
	pageTemplate.style = (source.style & ~kRemovedPageStyles) | WS_CHILD | DS_CONTROL | DS_3DLOOK;
	pageTemplate.exStyle |= WS_EX_CONTROLPARENT;
	pageTemplate.x = 0;
	pageTemplate.y = 0;
	pageTemplate.caption.clear();
	pageTemplate.menuName.clear();
	return pageTemplate;
}

bool createPage(HWND sheet, Win32PropertySheetState& state, Win32PropertySheetPage& sheetPage)
{
	PROPSHEETPAGEA& page = sheetPage.page;
	const Win32DialogTemplate* source = templateOf(page);
	if (source == nullptr)
	{
		return false;
	}
	if ((page.dwFlags & PSP_USECALLBACK) != 0 && page.pfnCallback != nullptr && page.pfnCallback(nullptr, PSPCB_CREATE, &page) == 0)
	{
		return false;
	}

	sheetPage.hwnd = Win32Dialogs::createFromTemplate(page.hInstance, childPageTemplate(*source), sheet, page.pfnDlgProc, reinterpret_cast<LPARAM>(&page));
	if (sheetPage.hwnd == nullptr)
	{
		return false;
	}
	const RECT& rect = state.pageRect;
	SetWindowPos(sheetPage.hwnd, nullptr, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE);
	return true;
}

void releasePage(Win32PropertySheetPage& sheetPage)
{
	if (sheetPage.hwnd != nullptr)
	{
		DestroyWindow(sheetPage.hwnd);
		sheetPage.hwnd = nullptr;
	}
	PROPSHEETPAGEA& page = sheetPage.page;
	if (sheetPage.isReleased || (page.dwFlags & PSP_USECALLBACK) == 0 || page.pfnCallback == nullptr)
	{
		return;
	}
	sheetPage.isReleased = true;
	page.pfnCallback(nullptr, PSPCB_RELEASE, &page);
}

void focusFirstControl(HWND page)
{
	const HWND control = GetNextDlgTabItem(page, nullptr, FALSE);
	if (control != nullptr)
	{
		SetFocus(control);
	}
}

}

void Win32PropertySheets::insertTab(HWND tab, int index, const PROPSHEETPAGEA& page)
{
	const std::string title = pageTitle(page);
	TCITEMA item = {};
	item.mask = TCIF_TEXT;
	item.pszText = const_cast<LPSTR>(title.c_str());
	SendMessageA(tab, TCM_INSERTITEMA, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(&item));
}

std::string Win32PropertySheets::resourceText(LPCSTR text)
{
	if (text == nullptr)
	{
		return std::string();
	}
	if (!IS_INTRESOURCE(text))
	{
		return text;
	}
	const char* resourceString = Win32Resources::instance().string(LOWORD(reinterpret_cast<uintptr_t>(text)));
	return resourceString != nullptr ? resourceString : std::string();
}

std::string Win32PropertySheets::pageTitle(const PROPSHEETPAGEA& page)
{
	if ((page.dwFlags & PSP_USETITLE) != 0)
	{
		return resourceText(page.pszTitle);
	}
	const Win32DialogTemplate* source = templateOf(page);
	return source != nullptr ? source->caption : std::string();
}

SIZE Win32PropertySheets::largestPageSize(const Win32PropertySheetState& state)
{
	SIZE size = {};
	for (const std::unique_ptr<Win32PropertySheetPage>& sheetPage : state.pages)
	{
		const Win32DialogTemplate* source = templateOf(sheetPage->page);
		if (source == nullptr)
		{
			continue;
		}
		const RECT rect = Win32Dialogs::templateRectInPixels(0, 0, source->cx, source->cy);
		size.cx = std::max(size.cx, rect.right);
		size.cy = std::max(size.cy, rect.bottom);
	}
	return size;
}

LRESULT Win32PropertySheets::notifyPage(HWND sheet, HWND page, UINT code, LPARAM lParam)
{
	PSHNOTIFY notify = {};
	notify.hdr.hwndFrom = sheet;
	notify.hdr.code = code;
	notify.lParam = lParam;
	SetWindowLongPtrA(page, DWLP_MSGRESULT, 0);
	return SendMessageA(page, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&notify));
}

bool Win32PropertySheets::killActivePage(HWND sheet, Win32PropertySheetState& state)
{
	if (state.activeIndex < 0)
	{
		return true;
	}
	return notifyPage(sheet, state.pages[static_cast<size_t>(state.activeIndex)]->hwnd, PSN_KILLACTIVE, 0) == 0;
}

bool Win32PropertySheets::activatePage(HWND sheet, Win32PropertySheetState& state, int index)
{
	if (index < 0 || index >= static_cast<int>(state.pages.size()))
	{
		return false;
	}
	Win32PropertySheetPage& sheetPage = *state.pages[static_cast<size_t>(index)];
	if (sheetPage.hwnd == nullptr && !createPage(sheet, state, sheetPage))
	{
		return false;
	}
	if (notifyPage(sheet, sheetPage.hwnd, PSN_SETACTIVE, 0) == -1)
	{
		return false;
	}

	if (state.activeIndex >= 0 && state.activeIndex != index)
	{
		ShowWindow(state.pages[static_cast<size_t>(state.activeIndex)]->hwnd, SW_HIDE);
	}
	state.activeIndex = index;
	SendMessageA(state.tab, TCM_SETCURSEL, static_cast<WPARAM>(index), 0);
	ShowWindow(sheetPage.hwnd, SW_SHOW);
	focusFirstControl(sheetPage.hwnd);
	return true;
}

void Win32PropertySheets::appendPage(Win32PropertySheetState& state, const PROPSHEETPAGEA& page, HPROPSHEETPAGE handle)
{
	auto sheetPage = std::make_unique<Win32PropertySheetPage>();
	sheetPage->page = page;
	sheetPage->handle = handle;
	state.pages.push_back(std::move(sheetPage));
	if (state.tab != nullptr)
	{
		insertTab(state.tab, static_cast<int>(state.pages.size()) - 1, page);
	}
}

bool Win32PropertySheets::removePage(HWND sheet, Win32PropertySheetState& state, int index)
{
	if (index < 0 || index >= static_cast<int>(state.pages.size()))
	{
		return false;
	}
	const bool wasActive = index == state.activeIndex;
	Win32PropertySheetPage& sheetPage = *state.pages[static_cast<size_t>(index)];
	releasePage(sheetPage);
	DestroyPropertySheetPage(sheetPage.handle);
	state.pages.erase(state.pages.begin() + index);
	SendMessageA(state.tab, TCM_DELETEITEM, static_cast<WPARAM>(index), 0);
	if (state.activeIndex > index)
	{
		--state.activeIndex;
	}
	if (wasActive)
	{
		state.activeIndex = -1;
		activatePage(sheet, state, std::min(index, static_cast<int>(state.pages.size()) - 1));
	}
	return true;
}

void Win32PropertySheets::destroyPages(Win32PropertySheetState& state)
{
	state.activeIndex = -1;
	for (std::unique_ptr<Win32PropertySheetPage>& sheetPage : state.pages)
	{
		releasePage(*sheetPage);
	}
}

int Win32PropertySheets::pageIndexOf(const Win32PropertySheetState& state, HWND page)
{
	for (size_t index = 0; index < state.pages.size(); ++index)
	{
		if (state.pages[index]->hwnd == page)
		{
			return static_cast<int>(index);
		}
	}
	return -1;
}

int Win32PropertySheets::pageIndexOf(const Win32PropertySheetState& state, HPROPSHEETPAGE handle)
{
	for (size_t index = 0; index < state.pages.size(); ++index)
	{
		if (state.pages[index]->handle == handle)
		{
			return static_cast<int>(index);
		}
	}
	return -1;
}
