#pragma once

#include "WBMacIterationPool.h"
#include "Win32Messages.h"

@class NSWindow;

class WBMacFramePump : public Win32FramePump
{
public:
	explicit WBMacFramePump(NSWindow* window);

	void install();
	void runFrame() override;

private:
	static void* resolveNativeWindow(HWND hwnd);
	static void renderOverlay();

	bool hasRenderDevice() const;
	void pumpEvents(double waitSeconds) const;
	void buildImGuiFrame() const;
	void synchronizeTitle();

	NSWindow* m_window;
	HWND m_deviceWindow = nullptr;
	std::string m_title;
	WBMacIterationPool m_iterationPool;
};
