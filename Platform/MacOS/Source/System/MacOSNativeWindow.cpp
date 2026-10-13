// MacOSNativeWindow.cpp — maps a window handle to the NSWindow the renderer draws into

#include "MacOSNativeWindow.h"

static MacOSNativeWindowResolver s_nativeWindowResolver = nullptr;
static bool s_backBufferUsesBackingScale = false;

void MacOS_SetBackBufferUsesBackingScale(bool usesBackingScale)
{
	s_backBufferUsesBackingScale = usesBackingScale;
}

bool MacOS_BackBufferUsesBackingScale()
{
	return s_backBufferUsesBackingScale;
}

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
