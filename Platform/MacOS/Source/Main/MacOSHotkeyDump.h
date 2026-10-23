// MacOSHotkeyDump.h — hotkey table the launcher's hotkey editor reads (-dumpHotkeys <path>)

#pragma once

#include <string>
#include <vector>

namespace MacOSHotkeyDump
{
	constexpr const char* kFlag = "-dumpHotkeys";

	bool write( const std::string& path );
	std::vector<std::string> buttonLabels();
	std::string profileRoot();
	std::string stringsTargetPath();
	std::string defaultCommandMapPath();
}
