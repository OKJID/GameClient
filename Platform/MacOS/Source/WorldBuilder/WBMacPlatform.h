#pragma once

#include "Win32Platform.h"

class WBMacPlatform : public Win32Platform
{
public:
	void* createCursor(const Win32Image& image, POINT hotSpot) override;
	void* systemCursor(UINT systemCursorId) override;
	void setCursor(void* platformCursor) override;
	void setCursorVisible(bool visible) override;
	void destroyCursor(void* platformCursor) override;

	void beep() override;
	bool playSoundFile(const std::string& path, bool waitUntilFinished) override;
	void stopSounds() override;

	bool chooseFile(const Win32FileDialogRequest& request, std::string& path) override;

	std::string fontFilePath(const std::string& faceName) override;

private:
	bool m_isCursorHidden = false;
};
