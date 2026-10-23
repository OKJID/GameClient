// MacOSHotkeyExport.h — saves the applied hotkeys of the profile as a .csf or a .big (-exportHotkeys <path>)

#pragma once

#include <string>

namespace MacOSHotkeyExport
{
	constexpr const char* kFlag = "-exportHotkeys";

	bool write( const std::string& path );
}
