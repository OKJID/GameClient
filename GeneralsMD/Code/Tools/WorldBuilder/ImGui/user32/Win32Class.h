#pragma once

#include "Win32Window.h"

#include <functional>

class Win32Control;

typedef std::unique_ptr<Win32Control> (*Win32ControlFactory)(Win32Window& window);

struct Win32WindowClass
{
	std::string name;
	UINT style = 0;
	WNDPROC windowProc = nullptr;
	int classExtra = 0;
	int windowExtra = 0;
	HINSTANCE instance = nullptr;
	HICON icon = nullptr;
	HCURSOR cursor = nullptr;
	HBRUSH background = nullptr;
	std::string menuName;
	Win32ControlFactory controlFactory = nullptr;
};

class Win32ClassRegistry
{
public:
	static Win32ClassRegistry& instance();

	Win32WindowClass* find(LPCSTR name);
	ATOM registerClass(const WNDCLASSA& windowClass);
	bool unregisterClass(LPCSTR name);
	void registerControlClass(const char* name, WNDPROC windowProc, Win32ControlFactory factory, UINT style);

private:
	Win32ClassRegistry();

	static std::string normalizedName(LPCSTR name);

	std::map<std::string, std::unique_ptr<Win32WindowClass>> m_classes;
	std::map<ATOM, Win32WindowClass*> m_atoms;
	ATOM m_nextAtom = 0xC000;
};

#define WC_DIALOG_NAME "#32770"
