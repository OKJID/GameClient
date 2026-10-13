#include "Win32Paths.h"
#include "Win32Platform.h"

#include <algorithm>

namespace
{

struct SystemState
{
	Win32Platform* platform = nullptr;
	int comInitializations = 0;
};

SystemState& state()
{
	static SystemState systemState;
	return systemState;
}

}

void Win32SetPlatform(Win32Platform* platform)
{
	state().platform = platform;
}

Win32Platform* Win32GetPlatform()
{
	return state().platform;
}

BOOL Beep(DWORD, DWORD)
{
	if (state().platform == nullptr)
	{
		return FALSE;
	}
	state().platform->beep();
	return TRUE;
}

BOOL MessageBeep(UINT)
{
	return Beep(0, 0);
}

BOOL PlaySoundA(LPCSTR sound, HMODULE, DWORD flags)
{
	Win32Platform* platform = state().platform;
	if (platform == nullptr)
	{
		return FALSE;
	}
	if ((flags & SND_PURGE) != 0 || sound == nullptr)
	{
		platform->stopSounds();
	}
	if (sound == nullptr || (flags & SND_FILENAME) == 0)
	{
		return sound == nullptr ? TRUE : FALSE;
	}
	return platform->playSoundFile(Win32NativePath(sound), (flags & SND_ASYNC) == 0) ? TRUE : FALSE;
}

HRESULT CoInitialize(void*)
{
	++state().comInitializations;
	return state().comInitializations == 1 ? S_OK : S_FALSE;
}

void CoUninitialize()
{
	state().comInitializations = std::max(0, state().comInitializations - 1);
}
