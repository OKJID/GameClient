#pragma once

#include "Win32Window.h"

#include "imgui.h"

class Win32WindowPresenter;

class Win32MouseRouter
{
public:
	void route(const Win32WindowPresenter& presenter);

private:
	Win32Window* hoveredRoot(const Win32WindowPresenter& presenter) const;
	bool isMouseUsedByImGui() const;
	void routeMove(Win32Window* root, POINT position, bool deliversToWin32);
	void routeButton(ImGuiMouseButton button, Win32Window* root, POINT position, bool deliversToWin32);
	void routeWheel(Win32Window* root, POINT position, bool deliversToWin32);
	void activateConsumedClick(Win32Window* root) const;
	void updateCursorOwnership(Win32Window* root, POINT position, bool deliversToWin32) const;
	void postMouse(UINT message, Win32Window* root, POINT position, int wheelDelta = 0) const;

	POINT m_lastPosition = {};
	bool m_hasLastPosition = false;
	bool m_isPressDelivered[ImGuiMouseButton_COUNT] = {};
	bool m_isPressSwallowed[ImGuiMouseButton_COUNT] = {};
	bool m_wasPopupOpen = false;
	float m_wheelRemainder = 0.0f;
};
