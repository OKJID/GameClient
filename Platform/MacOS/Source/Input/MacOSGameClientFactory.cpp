#include "W3DDevice/GameClient/W3DGameClient.h"
#include "GameClient/VideoPlayer.h"
#include "MacOSKeyboard.h"
#include "MacOSMouse.h"

// Macros to match Windows behavior (see "global cheat for the WndProc()" in W3DGameClient.h)
MacOSKeyboard *TheMacOSKeyboard = nullptr;
MacOSMouse *TheMacOSMouse = nullptr;

Keyboard *W3DGameClient::createKeyboard()
{
    TheMacOSKeyboard = NEW MacOSKeyboard;
    return TheMacOSKeyboard;
}

Mouse *W3DGameClient::createMouse()
{
    TheMacOSMouse = NEW MacOSMouse;
    return TheMacOSMouse;
}

VideoPlayerInterface *W3DGameClient::createVideoPlayer()
{
    // Return base VideoPlayer (Null Object pattern) because Bink is not available on macOS.
    // This prevents EXC_BAD_ACCESS when GameClient calls TheVideoPlayer->reset() or update().
    return NEW VideoPlayer;
}
