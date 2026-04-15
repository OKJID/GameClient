/**
 * MacOSDisplayManager.mm — Display mode enumeration implementation
 *
 * Enumerates available display modes via CGDisplayCopyAllDisplayModes
 * plus standard gaming resolutions. Used by MetalInterface8 for
 * GetAdapterModeCount/EnumAdapterModes.
 *
 * Resolution CHANGES are NOT handled here — they go through the
 * standard DX8Wrapper path (Resize_And_Position_Window / Set_Device_Resolution)
 * to mirror the tested Windows flow.
 */
#ifdef __APPLE__

#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>

#include "MacOSDisplayManager.h"
#include <algorithm>
#include <cstdio>
#include <set>

// ─── Mode enumeration ─────────────────────────────────────

void MacOSDisplayManager::enumerateModes() {
	if (m_modesEnumerated) return;
	m_modesEnumerated = true;
	m_modes.clear();

	std::set<std::pair<int,int>> seen;

	NSScreen* screen = [NSScreen mainScreen];
	int screenW = screen ? (int)screen.frame.size.width : 1920;
	int screenH = screen ? (int)screen.frame.size.height : 1080;

	CGDirectDisplayID display = CGMainDisplayID();
	CFArrayRef allModes = CGDisplayCopyAllDisplayModes(display, nullptr);
	if (allModes) {
		CFIndex count = CFArrayGetCount(allModes);
		for (CFIndex i = 0; i < count; i++) {
			CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(allModes, i);
			int w = (int)CGDisplayModeGetWidth(mode);
			int h = (int)CGDisplayModeGetHeight(mode);
			double hz = CGDisplayModeGetRefreshRate(mode);
			if (hz < 1.0) hz = 60.0;

			if (w < 800 || h < 600) continue;
			if (w > screenW || h > screenH) continue;

			auto key = std::make_pair(w, h);
			if (seen.insert(key).second) {
				m_modes.push_back({ w, h, (int)hz });
			}
		}
		CFRelease(allModes);
	}



	static const DisplayMode standardModes[] = {
		{ 800,  600,  60 },
		{ 1024, 768,  60 },
		{ 1152, 864,  60 },
		{ 1280, 720,  60 },
		{ 1280, 800,  60 },
		{ 1280, 960,  60 },
		{ 1280, 1024, 60 },
		{ 1366, 768,  60 },
		{ 1440, 900,  60 },
		{ 1600, 900,  60 },
		{ 1600, 1200, 60 },
		{ 1680, 1050, 60 },
		{ 1920, 1080, 60 },
		{ 1920, 1200, 60 },
		{ 2560, 1440, 60 },
		{ 2560, 1600, 60 },
		{ 3840, 2160, 60 },
	};

	for (const auto& mode : standardModes) {
		if (mode.w <= screenW && mode.h <= screenH) {
			auto key = std::make_pair(mode.w, mode.h);
			if (seen.insert(key).second) {
				m_modes.push_back(mode);
			}
		}
	}

	std::sort(m_modes.begin(), m_modes.end(), [](const DisplayMode& a, const DisplayMode& b) {
		int areaA = a.w * a.h;
		int areaB = b.w * b.h;
		if (areaA != areaB) return areaA < areaB;
		return a.w < b.w;
	});

	if (m_modes.empty()) {
		m_modes.push_back({ 800, 600, 60 });
	}

	printf("[MacOSDisplayManager] Enumerated %zu display modes:\n", m_modes.size());
	for (const auto& mode : m_modes) {
		printf("  %d x %d @ %d Hz\n", mode.w, mode.h, mode.hz);
	}
	fflush(stdout);
}

const std::vector<DisplayMode>& MacOSDisplayManager::getAvailableModes() {
	enumerateModes();
	return m_modes;
}

DisplayMode MacOSDisplayManager::getCurrentDesktopMode() const {
	NSScreen* screen = [NSScreen mainScreen];
	if (screen) {
		return { (int)screen.frame.size.width, (int)screen.frame.size.height, 60 };
	}
	return { 800, 600, 60 };
}

#endif // __APPLE__
