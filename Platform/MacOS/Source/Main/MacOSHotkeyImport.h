// MacOSHotkeyImport.h — turns a community hotkey table into a launcher hotkey preset (-importHotkeys <.csf|.big> -importHotkeysTo <preset.json>)

#pragma once

#include <string>

namespace MacOSHotkeyImport
{
	constexpr const char* kFlag = "-importHotkeys";
	constexpr const char* kOutputFlag = "-importHotkeysTo";

	bool write( const std::string& sourcePath, const std::string& presetPath );
}
