#include "W3DDevice/GameClient/W3DGameClient.h"
#include "MacOSKeyboard.h"
#include "MacOSMouse.h"

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

