#pragma once

#include "Win32Control.h"

class Win32Static : public Win32Control
{
public:
	explicit Win32Static(Win32Window& window) : Win32Control(window) {}

	LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
	void draw(const Win32DrawContext& context) override;
	bool drawsOwnFocus() const override { return false; }

private:
	UINT staticType() const { return style() & SS_TYPEMASK; }
	bool isImage() const { return staticType() == SS_BITMAP || staticType() == SS_ICON; }

	void loadImageFromName(const CREATESTRUCTA* create);
	HANDLE setImage(UINT type, HANDLE image);
	void drawText(const Win32DrawContext& context);
	void drawImage(const Win32DrawContext& context);
	void drawFrame(const Win32DrawContext& context);

	HANDLE m_image = nullptr;
	UINT m_imageType = IMAGE_BITMAP;
};
