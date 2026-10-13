#pragma once

#include <string>

std::string Win32NativePath(const std::string& enginePath);
std::string Win32EnginePath(const std::string& nativePath);
std::string Win32JoinPath(const std::string& directory, const std::string& fileName);
