#pragma once

#include <string>

@class NSImage;

bool WBMacIsModLaunch(int argc, char* argv[]);
NSImage* WBMacApplicationIcon(bool isModLaunch);
NSImage* WBMacSplashImage(const std::string& gameSplashPath, bool isModLaunch);
std::string WBMacIdentityName();
std::string WBMacWindowTitle(const std::string& frameTitle);
