#pragma once
// MacOSDisplayManager.h — Display mode enumeration and screen queries for Metal backend
//
// Provides mode enumeration (used by MetalInterface8::GetAdapterModeCount/EnumAdapterModes)
// and current desktop mode queries. Resolution CHANGES are handled by DX8Wrapper
// (Resize_And_Position_Window / Set_Device_Resolution) to mirror the Windows flow.

#include <vector>

struct DisplayMode {
	int w;
	int h;
	int hz;
};

class MacOSDisplayManager {
public:
	static MacOSDisplayManager& instance() {
		static MacOSDisplayManager mgr;
		return mgr;
	}

	const std::vector<DisplayMode>& getAvailableModes();
	DisplayMode getCurrentDesktopMode() const;

private:
	MacOSDisplayManager() = default;
	std::vector<DisplayMode> m_modes;
	bool m_modesEnumerated = false;

	void enumerateModes();
};
