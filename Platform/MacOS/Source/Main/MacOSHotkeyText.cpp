// MacOSHotkeyText.cpp — the hotkey marker inside a command label: "[&X]" or "&" before a letter

#include "MacOSHotkeyText.h"

#include <cwctype>

namespace
{
	const wchar_t* const kBracketOpen = L"[&";
	const size_t kBracketLength = 4;

	size_t bracketMarkerAt( const std::wstring& text )
	{
		const size_t bracket = text.find( kBracketOpen );
		if ( bracket == std::wstring::npos || bracket + kBracketLength > text.size() )
			return std::wstring::npos;

		if ( text[bracket + kBracketLength - 1] != L']' )
			return std::wstring::npos;

		return bracket;
	}

	size_t firstLetterAt( const std::wstring& text, wchar_t key )
	{
		const wint_t wanted = std::towupper( key );
		for ( size_t i = 0; i < text.size(); ++i )
		{
			if ( std::towupper( text[i] ) == wanted )
				return i;
		}
		return std::wstring::npos;
	}
}

std::wstring MacOSHotkeyText::withoutMarker( std::wstring text )
{
	const size_t bracket = bracketMarkerAt( text );
	if ( bracket != std::wstring::npos )
	{
		const size_t start = ( bracket > 0 && text[bracket - 1] == L' ' ) ? bracket - 1 : bracket;
		text.erase( start, bracket + kBracketLength - start );
		return text;
	}

	const size_t ampersand = text.find( L'&' );
	if ( ampersand != std::wstring::npos )
		text.erase( ampersand, 1 );

	return text;
}

std::wstring MacOSHotkeyText::withKey( const std::wstring& text, wchar_t key )
{
	const size_t bracket = bracketMarkerAt( text );
	if ( bracket != std::wstring::npos && key != 0 )
	{
		std::wstring result = text;
		result[bracket + 2] = key;
		return result;
	}

	std::wstring plain = withoutMarker( text );
	if ( key == 0 )
		return plain;

	const size_t letter = firstLetterAt( plain, key );
	if ( letter != std::wstring::npos )
	{
		plain.insert( letter, 1, L'&' );
		return plain;
	}

	return plain + L" [&" + key + L"]";
}
