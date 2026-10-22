// MacOSCommandMapText.h — sets the key of one CommandMap block inside CommandMap.ini text

#pragma once

#include <string>

struct MacOSCommandMapEntry
{
	std::string name;
	std::string key;
	std::string modifiers;
	std::string transition;
	std::string usableIn;
	std::string category;
};

namespace MacOSCommandMapText
{
	std::string withEntry( const std::string& ini, const MacOSCommandMapEntry& entry );
}
