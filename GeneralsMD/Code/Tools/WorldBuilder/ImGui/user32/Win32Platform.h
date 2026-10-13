#pragma once

#include "Win32Api.h"

#include <string>
#include <vector>

struct Win32Image;

struct Win32FileFilter
{
	std::string description;
	std::vector<std::string> extensions;
};

struct Win32FileDialogRequest
{
	bool isOpenDialog = true;
	std::string title;
	std::string initialDirectory;
	std::string initialFileName;
	std::string defaultExtension;
	std::vector<Win32FileFilter> filters;
	bool confirmOverwrite = false;
	bool fileMustExist = false;
};

class Win32Platform
{
public:
	virtual ~Win32Platform() = default;

	virtual void* createCursor(const Win32Image& image, POINT hotSpot) = 0;
	virtual void* systemCursor(UINT systemCursorId) = 0;
	virtual void setCursor(void* platformCursor) = 0;
	virtual void setCursorVisible(bool visible) = 0;
	virtual void destroyCursor(void* platformCursor) = 0;

	virtual void beep() = 0;
	virtual bool playSoundFile(const std::string& path, bool waitUntilFinished) = 0;
	virtual void stopSounds() = 0;

	virtual bool chooseFile(const Win32FileDialogRequest& request, std::string& path) = 0;

	virtual std::string fontFilePath(const std::string& faceName) = 0;
};

void Win32SetPlatform(Win32Platform* platform);
Win32Platform* Win32GetPlatform();
