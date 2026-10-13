#include "StdAfx.h"
#include "WorldBuilderPlatform.h"

#include "StdDevice/Common/StdBIGFileSystem.h"

#include "MacOSAudioManager.h"
#include "MacOSCrashHandler.h"
#include "MacOSInstallPaths.h"
#include "MacOSLocalFileSystem.h"

extern const char* gAppPrefix;

void WorldBuilderInstallCrashHandler()
{
	MacOS_InstallCrashHandlers(gAppPrefix);
}

void WorldBuilderPrepareWorkingDirectory()
{
	MacOS_PrepareInstallPaths();
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
