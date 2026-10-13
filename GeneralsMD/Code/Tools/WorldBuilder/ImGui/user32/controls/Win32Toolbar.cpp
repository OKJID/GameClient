#include "Win32Toolbar.h"

#include <algorithm>
#include <utility>

namespace
{

Win32ImagePtr copyOfBitmapImage(HGDIOBJ handle)
{
	Win32GdiObject* object = Win32Gdi::object(handle);
	if (object == nullptr || object->type != Win32GdiObjectType::Bitmap)
	{
		return nullptr;
	}
	const Win32Image& source = *static_cast<Win32Bitmap*>(object)->image;
	auto copy = std::make_shared<Win32Image>();
	copy->width = source.width;
	copy->height = source.height;
	copy->pixels = source.pixels;
	return copy;
}

Win32ImagePtr loadBitmapImage(const TBADDBITMAP& source)
{
	if (source.hInst == nullptr)
	{
		return copyOfBitmapImage(reinterpret_cast<HGDIOBJ>(source.nID));
	}
	const HANDLE loaded = LoadImageA(source.hInst, MAKEINTRESOURCE(source.nID), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
	Win32ImagePtr image = copyOfBitmapImage(loaded);
	DeleteObject(loaded);
	return image;
}

}

int Win32Toolbar::indexOfCommand(WPARAM commandId) const
{
	for (int index = 0; index < buttonCount(); ++index)
	{
		if (m_buttons[static_cast<size_t>(index)].idCommand == static_cast<int>(commandId))
		{
			return index;
		}
	}
	return -1;
}

TBBUTTON* Win32Toolbar::buttonByCommand(WPARAM commandId)
{
	const int index = indexOfCommand(commandId);
	return index >= 0 ? &m_buttons[static_cast<size_t>(index)] : nullptr;
}

LRESULT Win32Toolbar::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		m_notifyWindow = reinterpret_cast<const CREATESTRUCTA*>(lParam)->hwndParent;
		return Win32Control::handleMessage(message, wParam, lParam);
	case TB_SETPARENT:
		return reinterpret_cast<LRESULT>(std::exchange(m_notifyWindow, reinterpret_cast<HWND>(wParam)));
	case TB_BUTTONSTRUCTSIZE:
		return 0;
	case TB_ADDBITMAP:
		return addBitmap(static_cast<int>(wParam), reinterpret_cast<const TBADDBITMAP*>(lParam));
	case TB_REPLACEBITMAP:
		return replaceBitmap(reinterpret_cast<const TBREPLACEBITMAP*>(lParam));
	case TB_ADDBUTTONS:
		return insertButtons(buttonCount(), static_cast<int>(wParam), reinterpret_cast<const TBBUTTON*>(lParam));
	case TB_INSERTBUTTON:
		return insertButtons(static_cast<int>(wParam), 1, reinterpret_cast<const TBBUTTON*>(lParam));
	case TB_DELETEBUTTON:
		return deleteButton(static_cast<int>(wParam));
	case TB_BUTTONCOUNT:
		return buttonCount();
	case TB_COMMANDTOINDEX:
		return indexOfCommand(wParam);
	case WM_ERASEBKGND:
		return (style() & TBSTYLE_TRANSPARENT) != 0 ? FALSE : Win32Control::eraseWithSystemColor(reinterpret_cast<HDC>(wParam), COLOR_BTNFACE);
	case WM_LBUTTONDOWN:
	case WM_MOUSEMOVE:
	case WM_LBUTTONUP:
	case WM_CAPTURECHANGED:
		return handleMouse(message, lParam);
	default:
		return handleButtonMessage(message, wParam, lParam);
	}
}

LRESULT Win32Toolbar::handleButtonMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case TB_GETBUTTON:
	{
		const int index = static_cast<int>(wParam);
		TBBUTTON* button = reinterpret_cast<TBBUTTON*>(lParam);
		if (index < 0 || index >= buttonCount() || button == nullptr)
		{
			return FALSE;
		}
		*button = m_buttons[static_cast<size_t>(index)];
		return TRUE;
	}
	case TB_SETCMDID:
	{
		const int index = static_cast<int>(wParam);
		if (index < 0 || index >= buttonCount())
		{
			return FALSE;
		}
		m_buttons[static_cast<size_t>(index)].idCommand = static_cast<int>(lParam);
		return TRUE;
	}
	case TB_GETBITMAP:
	{
		const TBBUTTON* button = buttonByCommand(wParam);
		return button != nullptr ? button->iBitmap : 0;
	}
	case TB_GETBUTTONINFO:
		return getButtonInfo(wParam, reinterpret_cast<TBBUTTONINFOA*>(lParam));
	case TB_SETBUTTONINFO:
		return setButtonInfo(wParam, reinterpret_cast<const TBBUTTONINFOA*>(lParam));
	default:
		return handleStateMessage(message, wParam, lParam);
	}
}

LRESULT Win32Toolbar::handleStateMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	const bool isSet = LOWORD(lParam) != 0;
	switch (message)
	{
	case TB_ENABLEBUTTON:
		return setState(wParam, TBSTATE_ENABLED, isSet);
	case TB_CHECKBUTTON:
		return setState(wParam, TBSTATE_CHECKED, isSet);
	case TB_PRESSBUTTON:
		return setState(wParam, TBSTATE_PRESSED, isSet);
	case TB_HIDEBUTTON:
		return setState(wParam, TBSTATE_HIDDEN, isSet);
	case TB_INDETERMINATE:
		return setState(wParam, TBSTATE_INDETERMINATE, isSet);
	case TB_ISBUTTONENABLED:
		return queryState(wParam, TBSTATE_ENABLED);
	case TB_ISBUTTONCHECKED:
		return queryState(wParam, TBSTATE_CHECKED);
	case TB_ISBUTTONPRESSED:
		return queryState(wParam, TBSTATE_PRESSED);
	case TB_ISBUTTONHIDDEN:
		return queryState(wParam, TBSTATE_HIDDEN);
	case TB_ISBUTTONINDETERMINATE:
		return queryState(wParam, TBSTATE_INDETERMINATE);
	case TB_SETSTATE:
	{
		TBBUTTON* button = buttonByCommand(wParam);
		if (button == nullptr)
		{
			return FALSE;
		}
		button->fsState = static_cast<BYTE>(LOWORD(lParam));
		InvalidateRect(hwnd(), nullptr, TRUE);
		return TRUE;
	}
	case TB_GETSTATE:
	{
		const TBBUTTON* button = buttonByCommand(wParam);
		return button != nullptr ? button->fsState : -1;
	}
	default:
		return handleLayoutMessage(message, wParam, lParam);
	}
}

LRESULT Win32Toolbar::handleLayoutMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case TB_SETBITMAPSIZE:
		m_bitmapSize = { LOWORD(lParam), HIWORD(lParam) };
		return TRUE;
	case TB_SETBUTTONSIZE:
		m_buttonSize = { LOWORD(lParam), HIWORD(lParam) };
		return TRUE;
	case TB_GETBUTTONSIZE:
		return MAKELONG(m_buttonSize.cx, m_buttonSize.cy);
	case TB_GETITEMRECT:
	{
		const int index = static_cast<int>(wParam);
		RECT* rect = reinterpret_cast<RECT*>(lParam);
		if (index < 0 || index >= buttonCount() || rect == nullptr)
		{
			return FALSE;
		}
		*rect = itemRect(index);
		return TRUE;
	}
	case TB_GETRECT:
	{
		const int index = indexOfCommand(wParam);
		RECT* rect = reinterpret_cast<RECT*>(lParam);
		if (index < 0 || rect == nullptr)
		{
			return FALSE;
		}
		*rect = itemRect(index);
		return TRUE;
	}
	case TB_AUTOSIZE:
		autoSize();
		return 0;
	case TB_SETSTYLE:
		SetWindowLongA(hwnd(), GWL_STYLE, static_cast<LONG>(lParam));
		return 0;
	case TB_GETSTYLE:
		return static_cast<LRESULT>(style());
	case TB_SETEXTENDEDSTYLE:
		return static_cast<LRESULT>(std::exchange(m_extendedStyle, static_cast<DWORD>(lParam)));
	case TB_GETEXTENDEDSTYLE:
		return static_cast<LRESULT>(m_extendedStyle);
	default:
		return Win32Control::handleMessage(message, wParam, lParam);
	}
}

LRESULT Win32Toolbar::setState(WPARAM commandId, BYTE mask, bool isSet)
{
	TBBUTTON* button = buttonByCommand(commandId);
	if (button == nullptr)
	{
		return FALSE;
	}
	const BYTE state = isSet ? static_cast<BYTE>(button->fsState | mask) : static_cast<BYTE>(button->fsState & ~mask);
	if (state != button->fsState)
	{
		button->fsState = state;
		InvalidateRect(hwnd(), nullptr, TRUE);
	}
	return TRUE;
}

LRESULT Win32Toolbar::queryState(WPARAM commandId, BYTE mask) const
{
	const int index = indexOfCommand(commandId);
	if (index < 0)
	{
		return -1;
	}
	return (m_buttons[static_cast<size_t>(index)].fsState & mask) != 0 ? TRUE : FALSE;
}

LRESULT Win32Toolbar::addBitmap(int imageCount, const TBADDBITMAP* source)
{
	if (source == nullptr || imageCount <= 0)
	{
		return -1;
	}
	Win32ImagePtr image = loadBitmapImage(*source);
	if (image == nullptr)
	{
		return -1;
	}
	const int firstIndex = m_imageCount;
	m_imageBlocks.push_back({ *source, image, firstIndex, imageCount });
	m_imageCount += imageCount;
	InvalidateRect(hwnd(), nullptr, TRUE);
	return firstIndex;
}

LRESULT Win32Toolbar::replaceBitmap(const TBREPLACEBITMAP* replacement)
{
	if (replacement == nullptr)
	{
		return FALSE;
	}
	const auto block = std::find_if(m_imageBlocks.begin(), m_imageBlocks.end(), [&](const ImageBlock& candidate) {
		return candidate.source.hInst == replacement->hInstOld && candidate.source.nID == replacement->nIDOld;
	});
	const TBADDBITMAP source = { replacement->hInstNew, replacement->nIDNew };
	Win32ImagePtr image = block != m_imageBlocks.end() ? loadBitmapImage(source) : nullptr;
	if (image == nullptr)
	{
		return FALSE;
	}
	block->source = source;
	block->image = image;
	if (replacement->nButtons > 0)
	{
		block->count = replacement->nButtons;
	}
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}

LRESULT Win32Toolbar::insertButtons(int index, int count, const TBBUTTON* buttons)
{
	if (buttons == nullptr || count <= 0)
	{
		return FALSE;
	}
	const int position = index < 0 || index > buttonCount() ? buttonCount() : index;
	m_buttons.insert(m_buttons.begin() + position, buttons, buttons + count);
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}

LRESULT Win32Toolbar::deleteButton(int index)
{
	if (index < 0 || index >= buttonCount())
	{
		return FALSE;
	}
	m_buttons.erase(m_buttons.begin() + index);
	m_hotItem = -1;
	m_pressedItem = -1;
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}

LRESULT Win32Toolbar::getButtonInfo(WPARAM commandId, TBBUTTONINFOA* info) const
{
	const int index = info != nullptr && (info->dwMask & TBIF_BYINDEX) != 0 ? static_cast<int>(commandId) : indexOfCommand(commandId);
	if (info == nullptr || index < 0 || index >= buttonCount())
	{
		return -1;
	}
	const TBBUTTON& button = m_buttons[static_cast<size_t>(index)];
	info->idCommand = button.idCommand;
	info->iImage = button.iBitmap;
	info->fsState = button.fsState;
	info->fsStyle = button.fsStyle;
	info->lParam = button.dwData;
	info->cx = static_cast<WORD>(isSeparator(button) ? separatorWidth(button) : m_buttonSize.cx);
	if ((info->dwMask & TBIF_TEXT) != 0 && info->pszText != nullptr && info->cchText > 0)
	{
		info->pszText[0] = '\0';
	}
	return index;
}

LRESULT Win32Toolbar::setButtonInfo(WPARAM commandId, const TBBUTTONINFOA* info)
{
	const int index = info != nullptr && (info->dwMask & TBIF_BYINDEX) != 0 ? static_cast<int>(commandId) : indexOfCommand(commandId);
	if (info == nullptr || index < 0 || index >= buttonCount())
	{
		return FALSE;
	}
	TBBUTTON& button = m_buttons[static_cast<size_t>(index)];
	const DWORD mask = info->dwMask;
	button.idCommand = (mask & TBIF_COMMAND) != 0 ? info->idCommand : button.idCommand;
	button.iBitmap = (mask & TBIF_IMAGE) != 0 ? info->iImage : button.iBitmap;
	button.fsState = (mask & TBIF_STATE) != 0 ? info->fsState : button.fsState;
	button.fsStyle = (mask & TBIF_STYLE) != 0 ? info->fsStyle : button.fsStyle;
	button.dwData = (mask & TBIF_LPARAM) != 0 ? info->lParam : button.dwData;
	if ((mask & TBIF_SIZE) != 0 && isSeparator(button))
	{
		button.iBitmap = info->cx;
	}
	InvalidateRect(hwnd(), nullptr, TRUE);
	return TRUE;
}

void Win32Toolbar::autoSize()
{
	if ((style() & CCS_NORESIZE) != 0)
	{
		return;
	}
	const SIZE content = contentSize();
	const UINT flags = SWP_NOZORDER | SWP_NOACTIVATE;
	if ((style() & CCS_NOPARENTALIGN) != 0)
	{
		SetWindowPos(hwnd(), nullptr, 0, 0, content.cx, content.cy, flags | SWP_NOMOVE);
		return;
	}
	RECT parentClient = {};
	GetClientRect(parentHwnd(), &parentClient);
	const bool atBottom = (style() & CCS_BOTTOM) == CCS_BOTTOM;
	const int top = (style() & CCS_NOMOVEY) != 0 ? m_window.rect.top : (atBottom ? parentClient.bottom - content.cy : 0);
	SetWindowPos(hwnd(), nullptr, 0, top, parentClient.right, content.cy, flags);
}

LRESULT Win32Toolbar::handleMouse(UINT message, LPARAM lParam)
{
	const POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	switch (message)
	{
	case WM_LBUTTONDOWN:
		pressItem(itemAt(point));
		return 0;
	case WM_MOUSEMOVE:
		trackPress(itemAt(point));
		updateHotItem(itemAt(point));
		return 0;
	case WM_LBUTTONUP:
		releaseItem(point);
		return 0;
	default:
		if (m_pressedItem >= 0)
		{
			m_buttons[static_cast<size_t>(m_pressedItem)].fsState &= ~TBSTATE_PRESSED;
			m_pressedItem = -1;
			InvalidateRect(hwnd(), nullptr, TRUE);
		}
		return 0;
	}
}

void Win32Toolbar::pressItem(int index)
{
	if (index < 0)
	{
		return;
	}
	TBBUTTON& button = m_buttons[static_cast<size_t>(index)];
	if (isSeparator(button) || (button.fsState & TBSTATE_ENABLED) == 0)
	{
		return;
	}
	m_pressedItem = index;
	button.fsState |= TBSTATE_PRESSED;
	SetCapture(hwnd());
	InvalidateRect(hwnd(), nullptr, TRUE);
}

void Win32Toolbar::trackPress(int index)
{
	if (m_pressedItem < 0)
	{
		return;
	}
	TBBUTTON& button = m_buttons[static_cast<size_t>(m_pressedItem)];
	const BYTE state = index == m_pressedItem ? static_cast<BYTE>(button.fsState | TBSTATE_PRESSED) : static_cast<BYTE>(button.fsState & ~TBSTATE_PRESSED);
	if (state != button.fsState)
	{
		button.fsState = state;
		InvalidateRect(hwnd(), nullptr, TRUE);
	}
}

void Win32Toolbar::releaseItem(POINT point)
{
	const int pressed = m_pressedItem;
	if (pressed < 0)
	{
		return;
	}
	const bool isInside = itemAt(point) == pressed;
	ReleaseCapture();
	m_buttons[static_cast<size_t>(pressed)].fsState &= ~TBSTATE_PRESSED;
	m_pressedItem = -1;
	InvalidateRect(hwnd(), nullptr, TRUE);
	if (!isInside)
	{
		return;
	}
	toggleCheck(pressed);
	const HWND parent = notifyWindow();
	if (parent != nullptr)
	{
		const int commandId = m_buttons[static_cast<size_t>(pressed)].idCommand;
		SendMessageA(parent, WM_COMMAND, MAKEWPARAM(commandId, BN_CLICKED), reinterpret_cast<LPARAM>(hwnd()));
	}
}

void Win32Toolbar::toggleCheck(int index)
{
	TBBUTTON& button = m_buttons[static_cast<size_t>(index)];
	if ((button.fsStyle & TBSTYLE_CHECK) == 0)
	{
		return;
	}
	if ((button.fsStyle & TBSTYLE_GROUP) == 0)
	{
		button.fsState ^= TBSTATE_CHECKED;
		return;
	}
	int first = index;
	while (first > 0 && (m_buttons[static_cast<size_t>(first - 1)].fsStyle & TBSTYLE_GROUP) != 0)
	{
		--first;
	}
	for (int member = first; member < buttonCount() && (m_buttons[static_cast<size_t>(member)].fsStyle & TBSTYLE_GROUP) != 0; ++member)
	{
		m_buttons[static_cast<size_t>(member)].fsState &= ~TBSTATE_CHECKED;
	}
	button.fsState |= TBSTATE_CHECKED;
}

void Win32Toolbar::updateHotItem(int index)
{
	if (index == m_hotItem)
	{
		return;
	}
	m_hotItem = index;
	m_hotSince = ImGui::GetTime();
	m_toolTipText.clear();
	if (index < 0 || (style() & TBSTYLE_TOOLTIPS) == 0 || isSeparator(m_buttons[static_cast<size_t>(index)]))
	{
		return;
	}
	m_toolTipText = requestToolTipText(m_buttons[static_cast<size_t>(index)]);
}

std::string Win32Toolbar::requestToolTipText(const TBBUTTON& button) const
{
	const HWND parent = notifyWindow();
	if (parent == nullptr)
	{
		return std::string();
	}
	NMTTDISPINFOA info = {};
	info.hdr.hwndFrom = hwnd();
	info.hdr.idFrom = static_cast<UINT_PTR>(button.idCommand);
	info.hdr.code = TTN_NEEDTEXTA;
	SendMessageA(parent, WM_NOTIFY, static_cast<WPARAM>(button.idCommand), reinterpret_cast<LPARAM>(&info));
	if (info.lpszText == nullptr)
	{
		return info.szText;
	}
	if (!IS_INTRESOURCE(info.lpszText))
	{
		return info.lpszText;
	}
	char text[256] = {};
	LoadStringA(info.hinst, static_cast<UINT>(reinterpret_cast<uintptr_t>(info.lpszText)), text, sizeof(text));
	return text;
}
