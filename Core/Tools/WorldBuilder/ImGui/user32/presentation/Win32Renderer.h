#pragma once

#include "Win32KeyboardRouter.h"
#include "Win32MenuPresenter.h"
#include "Win32MouseRouter.h"
#include "Win32WindowPresenter.h"

class Win32Renderer
{
public:
	static Win32Renderer& instance();

	void configure(ImGuiIO& io) const;
	void buildFrame();

private:
	Win32Renderer() = default;

	Win32WindowPresenter m_windows;
	Win32MenuPresenter m_menus;
	Win32MouseRouter m_mouse;
	Win32KeyboardRouter m_keyboard;
};
