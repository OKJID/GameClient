#pragma once

#include "Win32Api.h"

#include "imgui.h"

class Win32KeyboardRouter
{
public:
	void route();

private:
	void routeKey(ImGuiKey key, bool isAltDown, bool isControlDown);
	void routeCharacters();
};
