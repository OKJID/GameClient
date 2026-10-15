#include "Win32KeyboardRouter.h"

#include "Win32Input.h"
#include "Win32InputQueue.h"

#include "imgui_internal.h"

namespace
{

const LPARAM kRepeatCount = 1;
const LPARAM kPreviousStateDown = static_cast<LPARAM>(1) << 30;
const LPARAM kTransitionUp = static_cast<LPARAM>(1) << 31;
const ImWchar kFirstPrintableCharacter = 0x20;
const ImWchar kDeleteCharacter = 0x7F;
const ImWchar kFirstNonAnsiCharacter = 0x100;

int virtualKeyOf(ImGuiKey key)
{
	switch (key)
	{
	case ImGuiKey_LeftShift:
	case ImGuiKey_RightShift:
		return VK_SHIFT;
	case ImGuiKey_LeftCtrl:
	case ImGuiKey_RightCtrl:
	case ImGuiKey_LeftSuper:
	case ImGuiKey_RightSuper:
		return VK_CONTROL;
	case ImGuiKey_LeftAlt:
	case ImGuiKey_RightAlt:
		return VK_MENU;
	case ImGuiKey_KeypadEnter:
		return VK_RETURN;
	default:
		return Win32Input::virtualKeyFor(key);
	}
}

bool isTextInputActive()
{
	const ImGuiContext& context = *ImGui::GetCurrentContext();
	return context.ActiveId != 0 && context.ActiveId == context.InputTextState.ID;
}

}

void Win32KeyboardRouter::route()
{
	const ImGuiIO& io = ImGui::GetIO();
	const bool isControlDown = io.KeyCtrl || io.KeySuper;
	for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; ++key)
	{
		routeKey(static_cast<ImGuiKey>(key), io.KeyAlt, isControlDown);
	}
	routeCharacters();
}

void Win32KeyboardRouter::routeKey(ImGuiKey key, bool isAltDown, bool isControlDown)
{
	const int virtualKey = virtualKeyOf(key);
	if (virtualKey == 0)
	{
		return;
	}

	const bool usesSystemKeys = (isAltDown && !isControlDown) || virtualKey == VK_F10;
	if (ImGui::IsKeyPressed(key, true))
	{
		const LPARAM previousState = ImGui::IsKeyPressed(key, false) ? 0 : kPreviousStateDown;
		Win32InputQueue::postKeyboard(usesSystemKeys ? WM_SYSKEYDOWN : WM_KEYDOWN, static_cast<WPARAM>(virtualKey), kRepeatCount | previousState);
	}
	if (ImGui::IsKeyReleased(key))
	{
		Win32InputQueue::postKeyboard(usesSystemKeys ? WM_SYSKEYUP : WM_KEYUP, static_cast<WPARAM>(virtualKey), kRepeatCount | kPreviousStateDown | kTransitionUp);
	}
}

void Win32KeyboardRouter::routeCharacters()
{
	if (isTextInputActive())
	{
		return;
	}
	for (const ImWchar character : ImGui::GetIO().InputQueueCharacters)
	{
		if (character < kFirstPrintableCharacter || character == kDeleteCharacter || character >= kFirstNonAnsiCharacter)
		{
			continue;
		}
		Win32InputQueue::postKeyboard(WM_CHAR, static_cast<WPARAM>(character), kRepeatCount);
	}
}
