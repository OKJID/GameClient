/*
**	Command & Conquer Generals(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "StdAfx.h"
#include "WorldBuilderPlatform.h"

#include "MilesAudioDevice/MilesAudioManager.h"

#include <io.h>
#include "Win32Device/GameClient/Win32Mouse.h"
#include "Win32Device/Common/Win32LocalFileSystem.h"
#include "Win32Device/Common/Win32BIGFileSystem.h"

#include "Common/StackDump.h"

Win32Mouse *TheWin32Mouse = nullptr;

/////////////////////////////////////////////////////////////////////////////
// Handler for unhandled win32 exceptions.

static LONG WINAPI UnHandledExceptionFilter(struct _EXCEPTION_POINTERS* e_info)
{
	DumpExceptionInfo(e_info->ExceptionRecord->ExceptionCode, e_info);
	return EXCEPTION_EXECUTE_HANDLER;
}

void WorldBuilderInstallCrashHandler()
{
	SetUnhandledExceptionFilter(UnHandledExceptionFilter);
}

void WorldBuilderPrepareWorkingDirectory()
{
	// Set the current directory to the app directory.
	char buf[_MAX_PATH];
	GetModuleFileName(nullptr, buf, sizeof(buf));
	if (char *pEnd = strrchr(buf, '\\')) {
		*pEnd = 0;
	}
	::SetCurrentDirectory(buf);
}

void WorldBuilderRestoreWorkingDirectory()
{
	WorldBuilderPrepareWorkingDirectory();
}

LocalFileSystem* WorldBuilderCreateLocalFileSystem()
{
	return new Win32LocalFileSystem;
}

ArchiveFileSystem* WorldBuilderCreateArchiveFileSystem()
{
	return new Win32BIGFileSystem;
}

AudioManager* WorldBuilderCreateAudioManager()
{
	return new MilesAudioManager();
}

Bool WorldBuilderGameHidesUserMapsWithOfficialNames()
{
	return FALSE;
}
