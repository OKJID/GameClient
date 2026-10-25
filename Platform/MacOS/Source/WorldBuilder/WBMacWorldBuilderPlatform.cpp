#include "StdAfx.h"
#include "WorldBuilderPlatform.h"

#include "StdDevice/Common/StdBIGFileSystem.h"

#include "MacOSAudioManager.h"
#include "MacOSCrashHandler.h"
#include "MacOSInstallPaths.h"
#include "MacOSLocalFileSystem.h"

#include <cstdlib>
#include <unistd.h>

extern const char* gAppPrefix;

void WorldBuilderInstallCrashHandler()
{
	MacOS_InstallCrashHandlers(gAppPrefix);
}

void WorldBuilderPrepareWorkingDirectory()
{
	MacOS_PrepareInstallPaths();
}

void WorldBuilderRestoreWorkingDirectory()
{
	const char* gamePath = getenv("GENERALS_ACTIVE_INSTALL_PATH");
	if (gamePath == nullptr || gamePath[0] == '\0')
		return;

	chdir(gamePath);
}

LocalFileSystem* WorldBuilderCreateLocalFileSystem()
{
	return new MacOSLocalFileSystem;
}

ArchiveFileSystem* WorldBuilderCreateArchiveFileSystem()
{
	return new StdBIGFileSystem;
}

AudioManager* WorldBuilderCreateAudioManager()
{
	return new MacOSAudioManager;
}

Bool WorldBuilderGameHidesUserMapsWithOfficialNames()
{
	return TRUE;
}
