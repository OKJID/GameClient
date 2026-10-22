// MacOSHotkeyDump.h — hotkey table the launcher's hotkey editor reads (-dumpHotkeys <path>)

#pragma once

#include <string>

namespace MacOSHotkeyDump
{
	constexpr const char* kFlag = "-dumpHotkeys";

	bool write( const char* path );
	std::string stringsTargetPath();
}
