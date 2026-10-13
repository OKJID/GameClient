#include "Win32Button.h"

#include "Win32Class.h"
#include "Win32ControlDrawing.h"
#include "Win32Gdi.h"

#include <algorithm>

namespace
{

bool isAutoRadio(const Win32Window* window)
{
	const bool isButtonClass = window->windowClass != nullptr && window->windowClass->controlFactory != nullptr && window->control != nullptr;
	return isButtonClass && (window->style & BS_TYPEMASK) == BS_AUTORADIOBUTTON;
}

}

bool Win32Button::isCheckBox() const
{
	const UINT type = buttonType();
	return type == BS_CHECKBOX || type == BS_AUTOCHECKBOX || type == BS_3STATE || type == BS_AUTO3STATE;
}

bool Win32Button::isRadioButton() const
{
	const UINT type = buttonType();
	return type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON;
}

bool Win32Button::isPushLike() const
{
	return (style() & BS_PUSHLIKE) != 0 && (isCheckBox() || isRadioButton());
}

bool Win32Button::containsPoint(LPARAM point) const
{
	RECT client;
	GetClientRect(hwnd(), &client);
	const POINT position = { GET_X_LPARAM(point), GET_Y_LPARAM(point) };
	return PtInRect(&client, position) != FALSE;
}

UINT Win32Button::dialogCode() const
{
	switch (buttonType())
	{
	case BS_DEFPUSHBUTTON:
		return DLGC_BUTTON | DLGC_DEFPUSHBUTTON;
	case BS_PUSHBUTTON:
		return DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON;
	case BS_RADIOBUTTON:
	case BS_AUTORADIOBUTTON:
		return DLGC_BUTTON | DLGC_RADIOBUTTON;
	case BS_GROUPBOX:
		return DLGC_STATIC;
	default:
		return DLGC_BUTTON;
	}
}

UINT Win32Button::buttonState() const
{
	UINT state = m_check;
	state |= m_isPushed ? BST_PUSHED : 0;
	state |= GetFocus() == hwnd() ? BST_FOCUS : 0;
	return state;
}

void Win32Button::uncheckGroupRadios()
{
	Win32Window* parent = m_window.parent;
	if (parent == nullptr)
	{
		return;
	}

	const std::vector<Win32Window*> siblings = parent->children;
	const auto position = std::find(siblings.begin(), siblings.end(), &m_window);
	auto groupStart = position;
	while (groupStart != siblings.begin() && ((*groupStart)->style & WS_GROUP) == 0)
	{
		--groupStart;
	}
	for (auto it = groupStart; it != siblings.end(); ++it)
	{
		if (it != groupStart && ((*it)->style & WS_GROUP) != 0)
		{
			break;
		}
		if (*it != &m_window && isAutoRadio(*it))
		{
			SendMessageA((*it)->hwnd, BM_SETCHECK, BST_UNCHECKED, 0);
		}
	}
}

void Win32Button::click()
{
	switch (buttonType())
	{
	case BS_AUTOCHECKBOX:
		m_check = m_check == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED;
		break;
	case BS_AUTO3STATE:
		m_check = (m_check + 1) % 3;
		break;
	case BS_AUTORADIOBUTTON:
		uncheckGroupRadios();
		m_check = BST_CHECKED;
		break;
	default:
		break;
	}
	notifyCommand(BN_CLICKED);
}

HANDLE Win32Button::setImage(WPARAM type, LPARAM image)
{
	const HANDLE previous = m_image;
	m_image = reinterpret_cast<HANDLE>(image);
	m_imageType = static_cast<UINT>(type);
	return previous;
}

LRESULT Win32Button::handleMouse(UINT message, LPARAM lParam)
{
	switch (message)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		takeFocus();
		SetCapture(hwnd());
		m_isTracking = true;
		m_isPushed = true;
		if (message == WM_LBUTTONDBLCLK && (isRadioButton() || notifiesFocus()))
		{
			notifyCommand(BN_DOUBLECLICKED);
		}
		return 0;
	case WM_MOUSEMOVE:
		m_isPushed = m_isTracking && containsPoint(lParam);
		return 0;
	case WM_LBUTTONUP:
	{
		const bool clicks = m_isTracking && containsPoint(lParam);
		m_isTracking = false;
		m_isPushed = false;
		ReleaseCapture();
		if (clicks)
		{
			click();
		}
		return 0;
	}
	case WM_CAPTURECHANGED:
		m_isTracking = false;
		m_isPushed = false;
		return 0;
	default:
		return 0;
	}
}

LRESULT Win32Button::handleButtonMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = true;
	switch (message)
	{
	case BM_GETCHECK:
		return isCheckBox() || isRadioButton() ? m_check : BST_UNCHECKED;
	case BM_SETCHECK:
		m_check = static_cast<UINT>(wParam) & (BST_CHECKED | BST_INDETERMINATE);
		return 0;
	case BM_GETSTATE:
		return buttonState();
	case BM_SETSTATE:
		m_isPushed = wParam != 0;
		return 0;
	case BM_SETSTYLE:
		m_window.style = (m_window.style & 0xFFFF0000u) | LOWORD(wParam);
		return 0;
	case BM_CLICK:
		if (IsWindowEnabled(hwnd()))
		{
			click();
		}
		return 0;
	case BM_GETIMAGE:
		return reinterpret_cast<LRESULT>(m_image);
	case BM_SETIMAGE:
		return reinterpret_cast<LRESULT>(setImage(wParam, lParam));
	case WM_GETDLGCODE:
		return dialogCode();
	default:
		handled = false;
		return 0;
	}
}

LRESULT Win32Button::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	bool handled = false;
	const LRESULT result = handleButtonMessage(message, wParam, lParam, handled);
	if (handled)
	{
		return result;
	}

	switch (message)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONUP:
	case WM_MOUSEMOVE:
	case WM_CAPTURECHANGED:
		return handleMouse(message, lParam);
	case WM_KEYDOWN:
		m_isPushed = m_isPushed || wParam == VK_SPACE;
		return 0;
	case WM_KEYUP:
		if (wParam == VK_SPACE && m_isPushed)
		{
			m_isPushed = false;
			click();
		}
		return 0;
	case WM_SETFOCUS:
		if (notifiesFocus())
		{
			notifyCommand(BN_SETFOCUS);
		}
		return 0;
	case WM_KILLFOCUS:
		m_isPushed = false;
		if (notifiesFocus())
		{
			notifyCommand(BN_KILLFOCUS);
		}
		return 0;
	default:
		return Win32Control::handleMessage(message, wParam, lParam);
	}
}
