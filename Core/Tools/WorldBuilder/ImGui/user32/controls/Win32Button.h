#pragma once

#include "Win32Control.h"

#include "imgui.h"

class Win32Button : public Win32Control
{
public:
	explicit Win32Button(Win32Window& window) : Win32Control(window) {}

	LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
	void draw(const Win32DrawContext& context) override;

private:
	UINT buttonType() const { return style() & BS_TYPEMASK; }
	bool isCheckBox() const;
	bool isRadioButton() const;
	bool isPushLike() const;
	bool notifiesFocus() const { return (style() & BS_NOTIFY) != 0; }
	bool containsPoint(LPARAM point) const;

	UINT dialogCode() const;
	UINT buttonState() const;
	void click();
	void uncheckGroupRadios();
	HANDLE setImage(WPARAM type, LPARAM image);
	LRESULT handleMouse(UINT message, LPARAM lParam);
	LRESULT handleButtonMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);

	void drawPushButton(const Win32DrawContext& context, bool isDown);
	void drawCheckMark(const Win32DrawContext& context);
	void drawRadioMark(const Win32DrawContext& context);
	void drawLabel(const Win32DrawContext& context, float left);
	void drawGroupBox(const Win32DrawContext& context);
	bool drawImage(const Win32DrawContext& context, ImVec2 min, ImVec2 max);
	void drawFocus(const Win32DrawContext& context);

	UINT m_check = BST_UNCHECKED;
	bool m_isPushed = false;
	bool m_isTracking = false;
	HANDLE m_image = nullptr;
	UINT m_imageType = IMAGE_BITMAP;
};
