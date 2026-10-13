#include "Win32Input.h"

#include "Win32Resources.h"

#include <cmath>
#include <map>
#include <memory>
#include <vector>

namespace
{

struct KeyMapping
{
	int virtualKey;
	ImGuiKey key;
};

const KeyMapping kKeyMappings[] = {
	{ VK_BACK, ImGuiKey_Backspace }, { VK_TAB, ImGuiKey_Tab }, { VK_RETURN, ImGuiKey_Enter }, { VK_ESCAPE, ImGuiKey_Escape },
	{ VK_SPACE, ImGuiKey_Space }, { VK_PRIOR, ImGuiKey_PageUp }, { VK_NEXT, ImGuiKey_PageDown }, { VK_END, ImGuiKey_End },
	{ VK_HOME, ImGuiKey_Home }, { VK_LEFT, ImGuiKey_LeftArrow }, { VK_UP, ImGuiKey_UpArrow }, { VK_RIGHT, ImGuiKey_RightArrow },
	{ VK_DOWN, ImGuiKey_DownArrow }, { VK_INSERT, ImGuiKey_Insert }, { VK_DELETE, ImGuiKey_Delete },
	{ VK_NUMPAD0, ImGuiKey_Keypad0 }, { VK_NUMPAD1, ImGuiKey_Keypad1 }, { VK_NUMPAD2, ImGuiKey_Keypad2 }, { VK_NUMPAD3, ImGuiKey_Keypad3 },
	{ VK_NUMPAD4, ImGuiKey_Keypad4 }, { VK_NUMPAD5, ImGuiKey_Keypad5 }, { VK_NUMPAD6, ImGuiKey_Keypad6 }, { VK_NUMPAD7, ImGuiKey_Keypad7 },
	{ VK_NUMPAD8, ImGuiKey_Keypad8 }, { VK_NUMPAD9, ImGuiKey_Keypad9 }, { VK_MULTIPLY, ImGuiKey_KeypadMultiply },
	{ VK_ADD, ImGuiKey_KeypadAdd }, { VK_SUBTRACT, ImGuiKey_KeypadSubtract }, { VK_DECIMAL, ImGuiKey_KeypadDecimal },
	{ VK_DIVIDE, ImGuiKey_KeypadDivide },
	{ VK_F1, ImGuiKey_F1 }, { VK_F2, ImGuiKey_F2 }, { VK_F3, ImGuiKey_F3 }, { VK_F4, ImGuiKey_F4 }, { VK_F5, ImGuiKey_F5 },
	{ VK_F6, ImGuiKey_F6 }, { VK_F7, ImGuiKey_F7 }, { VK_F8, ImGuiKey_F8 }, { VK_F9, ImGuiKey_F9 }, { VK_F10, ImGuiKey_F10 },
	{ VK_F11, ImGuiKey_F11 }, { VK_F12, ImGuiKey_F12 },
	{ VK_OEM_1, ImGuiKey_Semicolon }, { VK_OEM_PLUS, ImGuiKey_Equal }, { VK_OEM_COMMA, ImGuiKey_Comma }, { VK_OEM_MINUS, ImGuiKey_Minus },
	{ VK_OEM_PERIOD, ImGuiKey_Period }, { VK_OEM_2, ImGuiKey_Slash }, { VK_OEM_3, ImGuiKey_GraveAccent },
	{ VK_OEM_4, ImGuiKey_LeftBracket }, { VK_OEM_5, ImGuiKey_Backslash }, { VK_OEM_6, ImGuiKey_RightBracket },
	{ VK_OEM_7, ImGuiKey_Apostrophe },
	{ VK_LSHIFT, ImGuiKey_LeftShift }, { VK_RSHIFT, ImGuiKey_RightShift }, { VK_LCONTROL, ImGuiKey_LeftCtrl },
	{ VK_RCONTROL, ImGuiKey_RightCtrl }, { VK_LMENU, ImGuiKey_LeftAlt }, { VK_RMENU, ImGuiKey_RightAlt },
};

struct AcceleratorState
{
	std::map<HACCEL, std::vector<Win32AcceleratorTemplate>> tables;
	uintptr_t nextHandle = 0x40010;
};

AcceleratorState& acceleratorState()
{
	static AcceleratorState state;
	return state;
}

bool hasImGuiContext()
{
	return ImGui::GetCurrentContext() != nullptr;
}

bool isModifierDown(int virtualKey)
{
	const ImGuiIO& io = ImGui::GetIO();
	switch (virtualKey)
	{
	case VK_SHIFT:
		return io.KeyShift;
	case VK_CONTROL:
		return io.KeyCtrl || io.KeySuper;
	case VK_MENU:
		return io.KeyAlt;
	case VK_LBUTTON:
		return io.MouseDown[ImGuiMouseButton_Left];
	case VK_RBUTTON:
		return io.MouseDown[ImGuiMouseButton_Right];
	case VK_MBUTTON:
		return io.MouseDown[ImGuiMouseButton_Middle];
	default:
		return false;
	}
}

bool isModifierKey(int virtualKey)
{
	return virtualKey == VK_SHIFT || virtualKey == VK_CONTROL || virtualKey == VK_MENU || virtualKey == VK_LBUTTON
		|| virtualKey == VK_RBUTTON || virtualKey == VK_MBUTTON;
}

bool matchesAccelerator(const Win32AcceleratorTemplate& accelerator, const MSG* message)
{
	const bool isKeyMessage = message->message == WM_KEYDOWN || message->message == WM_SYSKEYDOWN;
	const bool matchesAsVirtualKey = accelerator.isVirtualKey || accelerator.control;
	if (matchesAsVirtualKey)
	{
		return isKeyMessage && message->wParam == accelerator.virtualKey
			&& Win32Input::isVirtualKeyDown(VK_SHIFT) == accelerator.shift
			&& Win32Input::isVirtualKeyDown(VK_CONTROL) == accelerator.control
			&& Win32Input::isVirtualKeyDown(VK_MENU) == accelerator.alt;
	}

	const bool isCharMessage = message->message == WM_CHAR || message->message == WM_SYSCHAR;
	return isCharMessage && message->wParam == accelerator.virtualKey && Win32Input::isVirtualKeyDown(VK_MENU) == accelerator.alt;
}

}

namespace Win32Input
{

ImGuiKey imGuiKeyFor(int virtualKey)
{
	if (virtualKey >= 'A' && virtualKey <= 'Z')
	{
		return static_cast<ImGuiKey>(ImGuiKey_A + (virtualKey - 'A'));
	}
	if (virtualKey >= '0' && virtualKey <= '9')
	{
		return static_cast<ImGuiKey>(ImGuiKey_0 + (virtualKey - '0'));
	}
	for (const KeyMapping& mapping : kKeyMappings)
	{
		if (mapping.virtualKey == virtualKey)
		{
			return mapping.key;
		}
	}
	return ImGuiKey_None;
}

int virtualKeyFor(ImGuiKey key)
{
	if (key >= ImGuiKey_A && key <= ImGuiKey_Z)
	{
		return 'A' + (key - ImGuiKey_A);
	}
	if (key >= ImGuiKey_0 && key <= ImGuiKey_9)
	{
		return '0' + (key - ImGuiKey_0);
	}
	for (const KeyMapping& mapping : kKeyMappings)
	{
		if (mapping.key == key)
		{
			return mapping.virtualKey;
		}
	}
	return 0;
}

bool isVirtualKeyDown(int virtualKey)
{
	if (!hasImGuiContext())
	{
		return false;
	}
	if (isModifierKey(virtualKey))
	{
		return isModifierDown(virtualKey);
	}
	const ImGuiKey key = imGuiKeyFor(virtualKey);
	return key != ImGuiKey_None && ImGui::IsKeyDown(key);
}

WPARAM mouseKeyFlags()
{
	WPARAM flags = 0;
	flags |= isVirtualKeyDown(VK_LBUTTON) ? MK_LBUTTON : 0;
	flags |= isVirtualKeyDown(VK_RBUTTON) ? MK_RBUTTON : 0;
	flags |= isVirtualKeyDown(VK_MBUTTON) ? MK_MBUTTON : 0;
	flags |= isVirtualKeyDown(VK_SHIFT) ? MK_SHIFT : 0;
	flags |= isVirtualKeyDown(VK_CONTROL) ? MK_CONTROL : 0;
	return flags;
}

}

SHORT GetAsyncKeyState(int virtualKey)
{
	return Win32Input::isVirtualKeyDown(virtualKey) ? static_cast<SHORT>(0x8000) : 0;
}

SHORT GetKeyState(int virtualKey)
{
	return GetAsyncKeyState(virtualKey);
}

BOOL GetCursorPos(LPPOINT point)
{
	if (point == nullptr)
	{
		return FALSE;
	}
	if (!hasImGuiContext() || !ImGui::IsMousePosValid())
	{
		*point = POINT();
		return TRUE;
	}

	const ImVec2 position = ImGui::GetIO().MousePos;
	point->x = static_cast<LONG>(std::floor(position.x));
	point->y = static_cast<LONG>(std::floor(position.y));
	return TRUE;
}

HACCEL LoadAcceleratorsA(HINSTANCE, LPCSTR tableName)
{
	UINT id = 0;
	if (!Win32Resources::instance().resolveName(tableName, id))
	{
		return nullptr;
	}
	const std::vector<Win32AcceleratorTemplate>* table = Win32Resources::instance().accelerators(id);
	if (table == nullptr)
	{
		return nullptr;
	}

	const HACCEL handle = reinterpret_cast<HACCEL>(acceleratorState().nextHandle);
	acceleratorState().nextHandle += 0x10;
	acceleratorState().tables[handle] = *table;
	return handle;
}

int TranslateAcceleratorA(HWND hwnd, HACCEL table, LPMSG message)
{
	const auto found = acceleratorState().tables.find(table);
	if (found == acceleratorState().tables.end() || message == nullptr || hwnd == nullptr)
	{
		return 0;
	}

	for (const Win32AcceleratorTemplate& accelerator : found->second)
	{
		if (matchesAccelerator(accelerator, message))
		{
			SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(accelerator.id, 1), 0);
			return 1;
		}
	}
	return 0;
}

BOOL DestroyAcceleratorTable(HACCEL table)
{
	return acceleratorState().tables.erase(table) != 0 ? TRUE : FALSE;
}
