// MacOSHotkeyText.h — the hotkey marker inside a command label: "[&X]" or "&" before a letter

#pragma once

#include <string>

namespace MacOSHotkeyText
{
	std::wstring withoutMarker( std::wstring text );
	std::wstring withKey( const std::wstring& text, wchar_t key );
}
