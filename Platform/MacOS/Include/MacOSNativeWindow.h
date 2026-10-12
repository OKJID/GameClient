#pragma once

#include <windows.h>

typedef void* (*MacOSNativeWindowResolver)(HWND hwnd);

void MacOS_SetNativeWindowResolver(MacOSNativeWindowResolver resolver);
void* MacOS_ResolveNativeWindow(HWND hwnd);
