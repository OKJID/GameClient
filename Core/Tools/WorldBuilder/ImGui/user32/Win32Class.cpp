#include "Win32Class.h"

#include <cctype>

Win32ClassRegistry& Win32ClassRegistry::instance()
{
	static Win32ClassRegistry registry;
	return registry;
}

Win32ClassRegistry::Win32ClassRegistry() = default;

std::string Win32ClassRegistry::normalizedName(LPCSTR name)
{
	std::string normalized(name);
	for (char& symbol : normalized)
	{
		symbol = static_cast<char>(std::tolower(static_cast<unsigned char>(symbol)));
	}
	return normalized;
}

Win32WindowClass* Win32ClassRegistry::find(LPCSTR name)
{
	if (name == nullptr)
	{
		return nullptr;
	}

	if (IS_INTRESOURCE(name))
	{
		const auto atom = m_atoms.find(static_cast<ATOM>(reinterpret_cast<uintptr_t>(name)));
		return atom == m_atoms.end() ? nullptr : atom->second;
	}

	const auto it = m_classes.find(normalizedName(name));
	return it == m_classes.end() ? nullptr : it->second.get();
}

ATOM Win32ClassRegistry::registerClass(const WNDCLASSA& windowClass)
{
	if (windowClass.lpszClassName == nullptr || find(windowClass.lpszClassName) != nullptr)
	{
		return 0;
	}

	auto registered = std::make_unique<Win32WindowClass>();
	registered->name = windowClass.lpszClassName;
	registered->style = windowClass.style;
	registered->windowProc = windowClass.lpfnWndProc;
	registered->classExtra = windowClass.cbClsExtra;
	registered->windowExtra = windowClass.cbWndExtra;
	registered->instance = windowClass.hInstance;
	registered->icon = windowClass.hIcon;
	registered->cursor = windowClass.hCursor;
	registered->background = windowClass.hbrBackground;
	registered->menuName = windowClass.lpszMenuName != nullptr && !IS_INTRESOURCE(windowClass.lpszMenuName) ? windowClass.lpszMenuName : "";

	const ATOM atom = m_nextAtom++;
	m_atoms[atom] = registered.get();
	m_classes[normalizedName(windowClass.lpszClassName)] = std::move(registered);
	return atom;
}

bool Win32ClassRegistry::unregisterClass(LPCSTR name)
{
	Win32WindowClass* windowClass = find(name);
	if (windowClass == nullptr)
	{
		return false;
	}

	for (auto it = m_atoms.begin(); it != m_atoms.end(); ++it)
	{
		if (it->second == windowClass)
		{
			m_atoms.erase(it);
			break;
		}
	}
	m_classes.erase(normalizedName(windowClass->name.c_str()));
	return true;
}

void Win32ClassRegistry::registerControlClass(const char* name, WNDPROC windowProc, Win32ControlFactory factory, UINT style)
{
	WNDCLASSA windowClass = {};
	windowClass.style = style | CS_GLOBALCLASS;
	windowClass.lpfnWndProc = windowProc;
	windowClass.lpszClassName = name;
	windowClass.hCursor = LoadCursorA(nullptr, IDC_ARROW);
	registerClass(windowClass);
	find(name)->controlFactory = factory;
}

ATOM RegisterClassA(const WNDCLASSA* windowClass)
{
	return windowClass != nullptr ? Win32ClassRegistry::instance().registerClass(*windowClass) : 0;
}

BOOL UnregisterClassA(LPCSTR className, HINSTANCE)
{
	return Win32ClassRegistry::instance().unregisterClass(className) ? TRUE : FALSE;
}

BOOL GetClassInfoA(HINSTANCE, LPCSTR className, WNDCLASSA* windowClass)
{
	const Win32WindowClass* found = Win32ClassRegistry::instance().find(className);
	if (found == nullptr || windowClass == nullptr)
	{
		return FALSE;
	}

	*windowClass = {};
	windowClass->style = found->style;
	windowClass->lpfnWndProc = found->windowProc;
	windowClass->cbClsExtra = found->classExtra;
	windowClass->cbWndExtra = found->windowExtra;
	windowClass->hInstance = found->instance;
	windowClass->hIcon = found->icon;
	windowClass->hCursor = found->cursor;
	windowClass->hbrBackground = found->background;
	windowClass->lpszMenuName = found->menuName.empty() ? nullptr : found->menuName.c_str();
	windowClass->lpszClassName = found->name.c_str();
	return TRUE;
}

int GetClassNameA(HWND hwnd, LPSTR className, int maxCount)
{
	const Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr || className == nullptr || maxCount <= 0)
	{
		return 0;
	}

	strlcpy(className, window->windowClass->name.c_str(), static_cast<size_t>(maxCount));
	return static_cast<int>(strlen(className));
}
