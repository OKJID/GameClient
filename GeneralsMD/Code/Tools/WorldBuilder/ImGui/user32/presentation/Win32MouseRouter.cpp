#include "Win32MouseRouter.h"

#include "Win32Input.h"
#include "Win32InputQueue.h"
#include "Win32Messages.h"
#include "Win32WindowPresenter.h"

#include "imgui_internal.h"

namespace
{

const UINT kButtonDownMessages[] = { WM_LBUTTONDOWN, WM_RBUTTONDOWN, WM_MBUTTONDOWN };
const UINT kButtonUpMessages[] = { WM_LBUTTONUP, WM_RBUTTONUP, WM_MBUTTONUP };
const int kRoutedButtonCount = 3;

}

void Win32MouseRouter::route(const Win32WindowPresenter& presenter)
{
	if (!ImGui::IsMousePosValid())
	{
		m_hasLastPosition = false;
		return;
	}

	POINT position;
	GetCursorPos(&position);
	Win32Window* root = hoveredRoot(presenter);
	const bool deliversToWin32 = GetCapture() != nullptr || (root != nullptr && !isMouseUsedByImGui());
	routeMove(root, position, deliversToWin32);
	for (int button = 0; button < kRoutedButtonCount; ++button)
	{
		routeButton(button, root, position, deliversToWin32);
	}
	routeWheel(root, position, deliversToWin32);
	updateCursorOwnership(root, position, deliversToWin32);
	m_wasPopupOpen = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
}

Win32Window* Win32MouseRouter::hoveredRoot(const Win32WindowPresenter& presenter) const
{
	const ImGuiContext& context = *ImGui::GetCurrentContext();
	return context.HoveredWindow != nullptr ? presenter.windowAtImGuiRoot(context.HoveredWindow->RootWindow) : nullptr;
}

bool Win32MouseRouter::isMouseUsedByImGui() const
{
	const ImGuiContext& context = *ImGui::GetCurrentContext();
	const bool isDraggingItem = context.ActiveId != 0 && context.ActiveIdMouseButton != -1 && ImGui::IsMouseDown(context.ActiveIdMouseButton);
	return context.HoveredId != 0 || isDraggingItem || context.MovingWindow != nullptr;
}

void Win32MouseRouter::routeMove(Win32Window* root, POINT position, bool deliversToWin32)
{
	const bool hasMoved = !m_hasLastPosition || position.x != m_lastPosition.x || position.y != m_lastPosition.y;
	m_lastPosition = position;
	m_hasLastPosition = true;
	if (hasMoved && deliversToWin32)
	{
		postMouse(WM_MOUSEMOVE, root, position);
	}
}

void Win32MouseRouter::routeButton(ImGuiMouseButton button, Win32Window* root, POINT position, bool deliversToWin32)
{
	if (ImGui::IsMouseClicked(button))
	{
		m_isPressSwallowed[button] = m_wasPopupOpen;
		m_isPressDelivered[button] = !m_wasPopupOpen && deliversToWin32;
		if (m_isPressDelivered[button])
		{
			postMouse(kButtonDownMessages[button], root, position);
		}
		else if (!m_wasPopupOpen)
		{
			activateConsumedClick(root);
		}
	}

	if (!ImGui::IsMouseReleased(button))
	{
		return;
	}
	const bool deliversRelease = m_isPressDelivered[button] || (GetCapture() != nullptr && !m_isPressSwallowed[button]);
	m_isPressDelivered[button] = false;
	m_isPressSwallowed[button] = false;
	if (deliversRelease)
	{
		postMouse(kButtonUpMessages[button], root, position);
	}
}

void Win32MouseRouter::routeWheel(Win32Window* root, POINT position, bool deliversToWin32)
{
	const float wheel = ImGui::GetIO().MouseWheel;
	if (wheel == 0.0f)
	{
		return;
	}
	if (!deliversToWin32)
	{
		m_wheelRemainder = 0.0f;
		return;
	}

	m_wheelRemainder += wheel * static_cast<float>(WHEEL_DELTA);
	const int delta = static_cast<int>(m_wheelRemainder);
	if (delta == 0)
	{
		return;
	}
	m_wheelRemainder -= static_cast<float>(delta);
	postMouse(WM_MOUSEWHEEL, root, position, delta);
}

void Win32MouseRouter::activateConsumedClick(Win32Window* root) const
{
	Win32Window* topLevel = Win32TopLevelOf(root);
	if (topLevel == nullptr || !topLevel->isEnabled() || GetActiveWindow() == topLevel->hwnd)
	{
		return;
	}
	const HWND hwnd = topLevel->hwnd;
	Win32PostTask(hwnd, [hwnd]() { SetActiveWindow(hwnd); });
}

void Win32MouseRouter::updateCursorOwnership(Win32Window* root, POINT position, bool deliversToWin32) const
{
	const RECT client = root != nullptr ? Win32WindowManager::instance().clientRectOnScreen(root) : RECT();
	const bool ownsCursor = deliversToWin32 && (GetCapture() != nullptr || PtInRect(&client, position));
	ImGuiIO& io = ImGui::GetIO();
	if (ownsCursor)
	{
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		return;
	}
	io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
}

void Win32MouseRouter::postMouse(UINT message, Win32Window* root, POINT position, int wheelDelta) const
{
	Win32MouseInput input;
	input.message = message;
	input.screenPoint = position;
	input.keyState = Win32Input::mouseKeyFlags();
	input.wheelDelta = wheelDelta;
	input.rootWindow = root != nullptr ? root->hwnd : nullptr;
	Win32InputQueue::postMouse(input);
}
