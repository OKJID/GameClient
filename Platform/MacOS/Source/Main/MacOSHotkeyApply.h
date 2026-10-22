// MacOSHotkeyApply.h — writes the launcher's hotkey edits into the profile's string table (-applyHotkeys <edit.json>)

#pragma once

namespace MacOSHotkeyApply
{
	constexpr const char* kFlag = "-applyHotkeys";

	bool apply( const char* editPath );
}
