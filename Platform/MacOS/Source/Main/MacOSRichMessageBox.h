// MacOSRichMessageBox.h — the game's OK message box whose body understands **bold**, `monospace` and {gold}color{/}

#pragma once

#include "Common/UnicodeString.h"
#include "GameClient/GameWindow.h"

class MacOSRichText;

class MacOSRichMessageBox
{
public:
	void show( const UnicodeString& title, const UnicodeString& markup );
	void close();

private:
	bool isOpen() const;
	void releaseText();

	GameWindow* m_box = nullptr;
	GameWindow* m_body = nullptr;
	MacOSRichText* m_text = nullptr;
};
