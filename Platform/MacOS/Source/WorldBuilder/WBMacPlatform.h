#pragma once

#include "Win32Platform.h"

@class NSWindow;

class WBMacPlatform : public Win32Platform
{
public:
	explicit WBMacPlatform(NSWindow* window);

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

	void resizeHostWindow(int width, int height) override;

private:
	NSWindow* m_window;
	bool m_isCursorHidden = false;
};
