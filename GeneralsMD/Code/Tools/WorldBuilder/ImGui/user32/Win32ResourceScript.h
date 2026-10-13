#pragma once

#include <string>
#include <vector>

struct Win32ResourceToken
{
	std::string text;
	bool isQuoted = false;
	char separator = ' ';
};

std::vector<std::string> Win32SplitLines(const std::string& contents);
std::vector<Win32ResourceToken> Win32TokenizeResourceScript(const std::string& contents);
std::string Win32UnescapeResourceString(const std::string& text);
