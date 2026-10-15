#pragma once

#include "Win32Window.h"

#include "imgui.h"

#include <map>
#include <vector>

struct ImGuiWindow;

struct Win32PresentedWindow
{
	RECT appliedRect = {};
	bool hasAppliedRect = false;
	ImGuiWindow* imguiWindow = nullptr;
};

class Win32WindowPresenter
{
public:
	void present();
	Win32Window* windowAtImGuiRoot(const ImGuiWindow* root) const;

private:
	void synchronizeHost();
	void resizeNativeHost(const Win32Window& host);
	void presentHost(Win32Window* host);
	void presentFloating(Win32Window* window);
	void presentDetached(std::vector<Win32Window*>& detached);
	void drawContent(Win32Window* window, const RECT& clip, std::vector<Win32Window*>& detached);
	void drawTree(Win32Window* window, const RECT& parentClip, std::vector<Win32Window*>& detached);
	void drawDisplayList(const Win32Window* window, const RECT& clientRect, const RECT& clientClip);
	void drawNonClient(const Win32Window* window, const RECT& windowRect);
	void drawControl(Win32Window* window, const RECT& clientRect);
	void placeNextWindow(Win32Window* window, const RECT& rect);
	void applyUserGeometry(Win32Window* window, const RECT& rect);
	void synchronizeActivation();
	void keepOwnedWindowsAbove();
	bool isCoveredByRegularWindow(ImGuiWindow* imguiWindow) const;
	void forgetDestroyedWindows();

	std::map<HWND, Win32PresentedWindow> m_presented;
	std::map<const ImGuiWindow*, Win32Window*> m_windowsByRoot;
	HWND m_lastActive = nullptr;
	const ImGuiWindow* m_lastFocusedRoot = nullptr;
	bool m_hostResizePending = false;
};
