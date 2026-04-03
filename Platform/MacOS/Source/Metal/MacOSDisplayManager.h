#pragma once
// MacOSDisplayManager.h — Display mode enumeration for Metal backend

#import <AppKit/AppKit.h>
#include <vector>

struct DisplayMode {
	unsigned w;
	unsigned h;
	unsigned hz;
};

class MacOSDisplayManager {
public:
	static MacOSDisplayManager& instance() {
		static MacOSDisplayManager mgr;
		return mgr;
	}

	const std::vector<DisplayMode>& getAvailableModes() {
		if (m_modes.empty()) {
			enumerateModes();
		}
		return m_modes;
	}

	DisplayMode getCurrentDesktopMode() {
		NSScreen* screen = [NSScreen mainScreen];
		NSRect frame = [screen frame];
		CGFloat scale = [screen backingScaleFactor];
		DisplayMode mode;
		mode.w = (unsigned)(frame.size.width * scale);
		mode.h = (unsigned)(frame.size.height * scale);
		mode.hz = 60;
		return mode;
	}

private:
	MacOSDisplayManager() = default;
	std::vector<DisplayMode> m_modes;

	void enumerateModes() {
		NSScreen* screen = [NSScreen mainScreen];
		NSRect frame = [screen frame];
		CGFloat scale = [screen backingScaleFactor];

		DisplayMode desktop;
		desktop.w = (unsigned)(frame.size.width * scale);
		desktop.h = (unsigned)(frame.size.height * scale);
		desktop.hz = 60;
		m_modes.push_back(desktop);

		unsigned commonWidths[] = {800, 1024, 1280, 1440, 1600, 1920};
		unsigned commonHeights[] = {600, 768, 720, 900, 900, 1080};
		for (int i = 0; i < 6; i++) {
			if (commonWidths[i] < desktop.w && commonHeights[i] < desktop.h) {
				DisplayMode m;
				m.w = commonWidths[i];
				m.h = commonHeights[i];
				m.hz = 60;
				m_modes.push_back(m);
			}
		}
	}
};
