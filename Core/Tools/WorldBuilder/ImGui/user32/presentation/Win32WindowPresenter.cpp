#include "Win32WindowPresenter.h"

#include "Win32Control.h"
#include "Win32ControlDrawing.h"
#include "Win32Gdi.h"
#include "Win32Messages.h"
#include "Win32Platform.h"

#include "imgui_internal.h"

#include <cmath>
#include <cstdio>
#include <string>

namespace
{

const char kHostWindowName[] = "##win32host";

Win32WindowManager& manager()
{
	return Win32WindowManager::instance();
}

RECT imguiWindowRect(const ImGuiWindow* window)
{
	RECT rect;
	const int left = static_cast<int>(std::floor(window->Pos.x));
	const int top = static_cast<int>(std::floor(window->Pos.y));
	SetRect(&rect, left, top, left + static_cast<int>(std::floor(window->Size.x)), top + static_cast<int>(std::floor(window->Size.y)));
	return rect;
}

void pushClip(const RECT& rect)
{
	ImGui::PushClipRect(ImVec2(static_cast<float>(rect.left), static_cast<float>(rect.top)),
		ImVec2(static_cast<float>(rect.right), static_cast<float>(rect.bottom)), false);
}

void subtractRect(const RECT& area, const RECT& hole, std::vector<RECT>& pieces)
{
	RECT cut;
	if (!IntersectRect(&cut, &area, &hole))
	{
		pieces.push_back(area);
		return;
	}
	const RECT bands[] = {
		{ area.left, area.top, area.right, cut.top },
		{ area.left, cut.bottom, area.right, area.bottom },
		{ area.left, cut.top, cut.left, cut.bottom },
		{ cut.right, cut.top, area.right, cut.bottom },
	};
	for (const RECT& band : bands)
	{
		if (!IsRectEmpty(&band))
		{
			pieces.push_back(band);
		}
	}
}

std::vector<RECT> regionsOutside(const RECT& area, const std::vector<RECT>& holes)
{
	std::vector<RECT> regions = { area };
	for (const RECT& hole : holes)
	{
		std::vector<RECT> remaining;
		for (const RECT& region : regions)
		{
			subtractRect(region, hole, remaining);
		}
		regions.swap(remaining);
	}
	return regions;
}

void collectNativeSurfaces(const Win32Window* window, std::vector<RECT>& surfaces)
{
	for (const Win32Window* child : window->children)
	{
		if (!child->isVisible() || child->presentation == Win32Presentation::OwnWindow)
		{
			continue;
		}
		if (child->presentsNativeSurface)
		{
			surfaces.push_back(Win32WindowManager::instance().clientRectOnScreen(child));
			continue;
		}
		collectNativeSurfaces(child, surfaces);
	}
}

bool hasFrame(const Win32Window* window)
{
	return (window->style & (WS_BORDER | WS_DLGFRAME | WS_THICKFRAME)) != 0;
}

std::string imguiWindowName(const Win32Window* window)
{
	char suffix[48];
	snprintf(suffix, sizeof(suffix), "###win32_%p", static_cast<void*>(window->hwnd));
	return window->text + suffix;
}

ImGuiWindowFlags floatingFlags(const Win32Window* window)
{
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
		| ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing;
	if ((window->style & WS_CAPTION) != WS_CAPTION)
	{
		flags |= ImGuiWindowFlags_NoTitleBar;
	}
	if ((window->style & WS_THICKFRAME) == 0 || !window->isEnabled())
	{
		flags |= ImGuiWindowFlags_NoResize;
	}
	if (!window->isEnabled())
	{
		flags |= ImGuiWindowFlags_NoMove;
	}
	return flags;
}

ImGuiWindowFlags hostFlags()
{
	return ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoNav
		| ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoFocusOnAppearing;
}

}

Win32Window* Win32WindowPresenter::windowAtImGuiRoot(const ImGuiWindow* root) const
{
	const auto found = m_windowsByRoot.find(root);
	return found != m_windowsByRoot.end() ? found->second : nullptr;
}

void Win32WindowPresenter::present()
{
	forgetDestroyedWindows();
	m_windowsByRoot.clear();
	synchronizeHost();

	Win32Window* host = manager().hostWindow();
	if (host != nullptr && host->isVisible())
	{
		presentHost(host);
	}

	const std::vector<Win32Window*> topLevel = manager().topLevelWindows();
	for (auto it = topLevel.rbegin(); it != topLevel.rend(); ++it)
	{
		Win32Window* window = *it;
		if (window == host || !window->isVisible() || (window->style & WS_MINIMIZE) != 0)
		{
			continue;
		}
		presentFloating(window);
	}

	keepOwnedWindowsAbove();
	synchronizeActivation();
}

void Win32WindowPresenter::synchronizeHost()
{
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const int width = static_cast<int>(displaySize.x);
	const int height = static_cast<int>(displaySize.y);
	manager().setHostSize(width, height);

	Win32Window* host = manager().hostWindow();
	if (host == nullptr)
	{
		return;
	}
	RECT target;
	SetRect(&target, 0, 0, width, height);
	if (EqualRect(&host->rect, &target) || m_hostResizePending)
	{
		return;
	}

	Win32PresentedWindow& presented = m_presented[host->hwnd];
	if (presented.hasAppliedRect && EqualRect(&presented.appliedRect, &target))
	{
		presented.appliedRect = host->rect;
		resizeNativeHost(*host);
		return;
	}

	presented.appliedRect = target;
	presented.hasAppliedRect = true;
	m_hostResizePending = true;
	const HWND hwnd = host->hwnd;
	Win32PostTask(hwnd, [this, hwnd, width, height]()
	{
		m_hostResizePending = false;
		SetWindowPos(hwnd, nullptr, 0, 0, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
	});
}

void Win32WindowPresenter::resizeNativeHost(const Win32Window& host)
{
	Win32Platform* platform = Win32GetPlatform();
	if (platform == nullptr)
	{
		return;
	}
	platform->resizeHostWindow(host.rect.right - host.rect.left, host.rect.bottom - host.rect.top);
}

void Win32WindowPresenter::presentHost(Win32Window* host)
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::Begin(kHostWindowName, nullptr, hostFlags());
	ImGui::PopStyleVar(3);

	ImGuiWindow* imguiWindow = ImGui::GetCurrentWindow();
	m_presented[host->hwnd].imguiWindow = imguiWindow;
	m_windowsByRoot[imguiWindow] = host;
	if (ImGui::FindWindowDisplayIndex(imguiWindow) != 0)
	{
		ImGui::BringWindowToDisplayBack(imguiWindow);
	}

	std::vector<Win32Window*> detached;
	drawContent(host, manager().windowRectOnScreen(host), detached);
	ImGui::End();
	presentDetached(detached);
}

void Win32WindowPresenter::presentFloating(Win32Window* window)
{
	const RECT rect = manager().windowRectOnScreen(window);
	placeNextWindow(window, rect);

	bool isOpen = true;
	const bool hasCloseBox = (window->style & WS_SYSMENU) != 0 && (window->style & WS_CAPTION) == WS_CAPTION;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1.0f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, hasFrame(window) ? ImGui::GetStyle().WindowBorderSize : 0.0f);
	const bool showsContent = ImGui::Begin(imguiWindowName(window).c_str(), hasCloseBox ? &isOpen : nullptr, floatingFlags(window));
	ImGui::PopStyleVar(3);

	ImGuiWindow* imguiWindow = ImGui::GetCurrentWindow();
	m_presented[window->hwnd].imguiWindow = imguiWindow;
	m_windowsByRoot[imguiWindow] = window;

	std::vector<Win32Window*> detached;
	if (showsContent)
	{
		drawContent(window, rect, detached);
	}
	applyUserGeometry(window, imguiWindowRect(imguiWindow));
	ImGui::End();

	if (!isOpen)
	{
		PostMessageA(window->hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
	}
	presentDetached(detached);
}

void Win32WindowPresenter::presentDetached(std::vector<Win32Window*>& detached)
{
	for (Win32Window* window : detached)
	{
		presentFloating(window);
	}
}

void Win32WindowPresenter::drawContent(Win32Window* window, const RECT& clip, std::vector<Win32Window*>& detached)
{
	const bool blocksInput = Win32IsInputBlocked(window);
	if (blocksInput)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
	}
	drawTree(window, clip, detached);
	ImGui::SetCursorScreenPos(ImGui::GetWindowPos());
	ImGui::Dummy(ImVec2(0.0f, 0.0f));
	if (blocksInput)
	{
		ImGui::PopItemFlag();
	}
}

void Win32WindowPresenter::drawTree(Win32Window* window, const RECT& parentClip, std::vector<Win32Window*>& detached)
{
	const RECT windowRect = manager().windowRectOnScreen(window);
	RECT windowClip;
	if (!IntersectRect(&windowClip, &parentClip, &windowRect))
	{
		return;
	}
	if (window->isChild())
	{
		pushClip(windowClip);
		drawNonClient(window, windowRect);
		ImGui::PopClipRect();
	}

	const RECT clientRect = manager().clientRectOnScreen(window);
	RECT clientClip;
	if (!IntersectRect(&clientClip, &windowClip, &clientRect))
	{
		return;
	}

	pushClip(clientClip);
	drawDisplayList(window, clientRect, clientClip);
	if (window->control != nullptr && window->controlPaints)
	{
		drawControl(window, clientRect);
	}
	for (auto it = window->children.rbegin(); it != window->children.rend(); ++it)
	{
		Win32Window* child = *it;
		if (!child->isVisible())
		{
			continue;
		}
		if (child->presentation == Win32Presentation::OwnWindow)
		{
			detached.push_back(child);
			continue;
		}
		drawTree(child, clientClip, detached);
	}
	ImGui::PopClipRect();
}

void Win32WindowPresenter::drawDisplayList(const Win32Window* window, const RECT& clientRect, const RECT& clientClip)
{
	if (window->displayList == nullptr)
	{
		return;
	}
	std::vector<RECT> surfaces;
	collectNativeSurfaces(window, surfaces);
	const ImVec2 origin(static_cast<float>(clientRect.left), static_cast<float>(clientRect.top));
	for (const RECT& region : regionsOutside(clientClip, surfaces))
	{
		pushClip(region);
		window->displayList->replay(ImGui::GetWindowDrawList(), origin);
		ImGui::PopClipRect();
	}
}

void Win32WindowPresenter::drawNonClient(const Win32Window* window, const RECT& windowRect)
{
	if (window->nonClientDisplayList == nullptr)
	{
		return;
	}
	const ImVec2 origin(static_cast<float>(windowRect.left), static_cast<float>(windowRect.top));
	window->nonClientDisplayList->replay(ImGui::GetWindowDrawList(), origin);
}

void Win32WindowPresenter::drawControl(Win32Window* window, const RECT& clientRect)
{
	Win32DrawContext context;
	context.drawList = ImGui::GetWindowDrawList();
	context.clientOrigin.x = clientRect.left;
	context.clientOrigin.y = clientRect.top;
	context.clientSize.cx = clientRect.right - clientRect.left;
	context.clientSize.cy = clientRect.bottom - clientRect.top;
	context.isEnabled = window->isEnabled();
	context.hasFocus = GetFocus() == window->hwnd;

	ImGui::PushID(window->hwnd);
	if (!context.isEnabled)
	{
		ImGui::BeginDisabled();
	}
	window->control->draw(context);
	if (!context.isEnabled)
	{
		ImGui::EndDisabled();
	}
	ImGui::PopID();
}

void Win32WindowPresenter::placeNextWindow(Win32Window* window, const RECT& rect)
{
	Win32PresentedWindow& presented = m_presented[window->hwnd];
	if (presented.hasAppliedRect && EqualRect(&presented.appliedRect, &rect))
	{
		return;
	}
	ImGui::SetNextWindowPos(ImVec2(static_cast<float>(rect.left), static_cast<float>(rect.top)), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(static_cast<float>(rect.right - rect.left), static_cast<float>(rect.bottom - rect.top)), ImGuiCond_Always);
	presented.appliedRect = rect;
	presented.hasAppliedRect = true;
}

void Win32WindowPresenter::applyUserGeometry(Win32Window* window, const RECT& rect)
{
	Win32PresentedWindow& presented = m_presented[window->hwnd];
	if (EqualRect(&presented.appliedRect, &rect))
	{
		return;
	}
	presented.appliedRect = rect;

	RECT target = rect;
	if (window->isChild() && window->parent != nullptr)
	{
		const POINT origin = manager().clientOriginOnScreen(window->parent);
		OffsetRect(&target, -origin.x, -origin.y);
	}
	const HWND hwnd = window->hwnd;
	Win32PostTask(hwnd, [hwnd, target]() {
		SetWindowPos(hwnd, nullptr, target.left, target.top, target.right - target.left, target.bottom - target.top, SWP_NOZORDER | SWP_NOACTIVATE);
	});
}

void Win32WindowPresenter::synchronizeActivation()
{
	const ImGuiContext& context = *ImGui::GetCurrentContext();
	const HWND active = GetActiveWindow();
	const ImGuiWindow* focusedRoot = context.NavWindow != nullptr ? context.NavWindow->RootWindow : nullptr;
	const Win32Window* focusedTopLevel = Win32TopLevelOf(windowAtImGuiRoot(focusedRoot));
	const bool activationChanged = active != m_lastActive;
	const bool focusIsBlocked = focusedTopLevel != nullptr && !focusedTopLevel->isEnabled();
	m_lastActive = active;
	if (!activationChanged && !focusIsBlocked)
	{
		return;
	}

	const auto found = m_presented.find(active);
	if (found != m_presented.end() && found->second.imguiWindow != nullptr)
	{
		ImGui::FocusWindow(found->second.imguiWindow);
	}
}

void Win32WindowPresenter::keepOwnedWindowsAbove()
{
	for (Win32Window* window : manager().topLevelWindows())
	{
		const bool isTopMost = (window->exStyle & WS_EX_TOPMOST) != 0;
		if (window->owner == nullptr && !isTopMost)
		{
			continue;
		}
		const auto presented = m_presented.find(window->hwnd);
		const auto owner = window->owner != nullptr ? m_presented.find(window->owner->hwnd) : m_presented.end();
		if (presented == m_presented.end() || presented->second.imguiWindow == nullptr || !window->isVisible())
		{
			continue;
		}
		ImGuiWindow* imguiWindow = presented->second.imguiWindow;
		const bool isBelowOwner = owner != m_presented.end() && owner->second.imguiWindow != nullptr
			&& ImGui::FindWindowDisplayIndex(imguiWindow) < ImGui::FindWindowDisplayIndex(owner->second.imguiWindow);
		if (isBelowOwner || (isTopMost && isCoveredByRegularWindow(imguiWindow)))
		{
			ImGui::BringWindowToDisplayFront(imguiWindow);
		}
	}
}

bool Win32WindowPresenter::isCoveredByRegularWindow(ImGuiWindow* imguiWindow) const
{
	const ImGuiContext& context = *ImGui::GetCurrentContext();
	for (int index = ImGui::FindWindowDisplayIndex(imguiWindow) + 1; index < context.Windows.Size; ++index)
	{
		const Win32Window* above = windowAtImGuiRoot(context.Windows[index]);
		if (above != nullptr && (above->exStyle & WS_EX_TOPMOST) == 0)
		{
			return true;
		}
	}
	return false;
}

void Win32WindowPresenter::forgetDestroyedWindows()
{
	for (auto it = m_presented.begin(); it != m_presented.end();)
	{
		it = IsWindow(it->first) ? std::next(it) : m_presented.erase(it);
	}
	if (!IsWindow(m_lastActive))
	{
		m_lastActive = nullptr;
	}
}
