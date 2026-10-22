// MacOSHotkeyEdits.h — parses the unsaved hotkey edits the launcher hands over (hotkey-dump-edit.json)

#pragma once

#include <map>
#include <string>

struct MacOSGlobalKeyEdit
{
	std::string key;
	std::string modifiers;
};

struct MacOSHotkeyEdits
{
	std::map<std::string, std::string> hotkeys;
	std::map<std::string, MacOSGlobalKeyEdit> globals;
};

bool MacOS_ParseHotkeyEdits( const std::string& json, MacOSHotkeyEdits& edits );
