// MacOSNativeWindow.cpp — maps a window handle to the NSWindow the renderer draws into

#include "MacOSNativeWindow.h"

static MacOSNativeWindowResolver s_nativeWindowResolver = nullptr;

void MacOS_SetNativeWindowResolver(MacOSNativeWindowResolver resolver)
{
	s_nativeWindowResolver = resolver;
}

void* MacOS_ResolveNativeWindow(HWND hwnd)
{
	if (s_nativeWindowResolver == nullptr)
	{
		return hwnd;
	}

	return s_nativeWindowResolver(hwnd);
}
