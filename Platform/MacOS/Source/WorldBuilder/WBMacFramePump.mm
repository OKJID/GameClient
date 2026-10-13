#include "WBMacCocoa.h"

#include "WBMacFramePump.h"

#include "MacOSNativeWindow.h"
#include "MacOSSceneOverlay.h"
#include "Win32Renderer.h"
#include "imgui.h"
#include "imgui_impl_dx8wrapper.h"
#include "imgui_impl_osx.h"

#include "WW3D2/dx8wrapper.h"

namespace
{

const double kIdleWaitSeconds = 1.0 / 60.0;

WBMacFramePump* installedPump = nullptr;

}

WBMacFramePump::WBMacFramePump(NSWindow* window)
	: m_window(window)
{
}

void WBMacFramePump::install()
{
	installedPump = this;
	Win32SetFramePump(this);
	MacOS_SetNativeWindowResolver(&WBMacFramePump::resolveNativeWindow);
	MacOS_SetSceneOverlay(&WBMacFramePump::renderOverlay);
}

void WBMacFramePump::runFrame()
{
	@autoreleasepool
	{
		if (!hasRenderDevice())
		{
			pumpEvents(kIdleWaitSeconds);
			return;
		}

		pumpEvents(0.0);
		buildImGuiFrame();
		synchronizeTitle();
		InvalidateRect(m_deviceWindow, nullptr, FALSE);
	}
}

void* WBMacFramePump::resolveNativeWindow(HWND hwnd)
{
	installedPump->m_deviceWindow = hwnd;
	Win32SetNativeSurface(hwnd);
	return (__bridge void*)installedPump->m_window;
}

void WBMacFramePump::renderOverlay()
{
	ImDrawData* drawData = ImGui::GetDrawData();
	if (drawData == nullptr)
	{
		return;
	}
	ImGui_ImplDX8Wrapper_RenderDrawData(drawData);
}

bool WBMacFramePump::hasRenderDevice() const
{
	return m_deviceWindow != nullptr && IsWindow(m_deviceWindow) && DX8Wrapper::_Get_D3D_Device8() != nullptr;
}

void WBMacFramePump::pumpEvents(double waitSeconds) const
{
	NSDate* deadline = [NSDate dateWithTimeIntervalSinceNow:waitSeconds];
	NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:deadline inMode:NSDefaultRunLoopMode dequeue:YES];
	while (event != nil)
	{
		[NSApp sendEvent:event];
		event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
	}
}

void WBMacFramePump::buildImGuiFrame() const
{
	ImGui_ImplDX8Wrapper_NewFrame();
	ImGui_ImplOSX_NewFrame(m_window.contentView);
	ImGui::NewFrame();
	Win32Renderer::instance().buildFrame();
	ImGui::Render();
}

void WBMacFramePump::synchronizeTitle()
{
	const Win32Window* host = Win32WindowManager::instance().hostWindow();
	if (host == nullptr || host->text == m_title)
	{
		return;
	}
	m_title = host->text;
	m_window.title = [NSString stringWithUTF8String:m_title.c_str()];
}
