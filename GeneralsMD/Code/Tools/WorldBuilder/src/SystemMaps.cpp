/*
**	Command & Conquer Generals Zero Hour(tm)
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
#include "SystemMaps.h"

#include "Common/GlobalData.h"
#include "Common/System/NativeFileSystem.h"
#include "Common/file.h"
#include "GameClient/GameText.h"
#include "GameClient/MapUtil.h"

static const char *const MapsDirectory = "Maps\\";
static const char *const MapExtension = ".map";

static AsciiString makeMapPath(const AsciiString &directory, const AsciiString &name)
{
	AsciiString path;
	path.format("%s%s\\%s%s", directory.str(), name.str(), name.str(), MapExtension);
	return path;
}

static AsciiString makeUserMapsDirectory()
{
	AsciiString directory = TheGlobalData->getPath_UserData();
	directory.concat(MapsDirectory);
	return directory;
}

static AsciiString getFileName(const AsciiString &path)
{
	const char *separator = path.reverseFind('\\');
	if (separator == nullptr)
		return path;

	return AsciiString(separator + 1);
}

static AsciiString getSystemMapName(const AsciiString &path)
{
	AsciiString name = getFileName(path);
	if (!name.endsWithNoCase(MapExtension))
		return AsciiString::TheEmptyString;

	name.truncateBy(strlen(MapExtension));
	if (path.compareNoCase(makeMapPath(MapsDirectory, name)) != 0)
		return AsciiString::TheEmptyString;

	return name;
}

static AsciiString getModId()
{
	if (TheGlobalData->m_modId.isNotEmpty())
		return TheGlobalData->m_modId;

	AsciiString modDirectory = TheGlobalData->m_modDir;
	modDirectory.truncateBy(1);
	AsciiString modId = getFileName(modDirectory);
	modId.toLower();
	return modId;
}

static Bool isFromMod(const AsciiString &name)
{
	File *file = TheFileSystem->openFileInMod(makeMapPath(MapsDirectory, name).str(), File::READ | File::BINARY);
	if (file == nullptr)
		return FALSE;

	file->close();
	return TRUE;
}

static AsciiString getSourceId(const AsciiString &name)
{
	if (isFromMod(name))
		return getModId();

#if RTS_ZEROHOUR
	return AsciiString("zh");
#else
	return AsciiString("generals");
#endif
}

static AsciiString makeCopyFileName(const AsciiString &fileName, const AsciiString &name, const AsciiString &copyName)
{
	const char *extension = fileName.reverseFind('.');
	if (extension == nullptr)
		return fileName;

	AsciiString stem = fileName;
	stem.truncateBy(strlen(extension));
	if (stem.compareNoCase(name) != 0)
		return fileName;

	AsciiString copyFileName = copyName;
	copyFileName.concat(extension);
	return copyFileName;
}

static UnicodeString getNameWithoutPlayerCount(const MapMetaData &mapData)
{
	UnicodeString name = mapData.m_displayName;
	if (mapData.m_numPlayers < 2)
		return name;

	UnicodeString playerCount;
	playerCount.format(L" (%d)", mapData.m_numPlayers);
	if (name.endsWith(playerCount))
		name.truncateBy(playerCount.getLength());

	return name;
}

static Bool isOfficialGameName(const UnicodeString &gameName)
{
	for (MapCache::const_iterator it = TheMapCache->begin(); it != TheMapCache->end(); ++it)
	{
		if (it->second.m_isOfficial && getNameWithoutPlayerCount(it->second) == gameName)
			return TRUE;
	}

	return FALSE;
}

static Bool writeFile(const AsciiString &path, const char *data, Int size)
{
	FILE *file = NativeFileSystem::fopen(path, "wb");
	if (file == nullptr)
		return FALSE;

	const Bool written = size == 0 || fwrite(data, 1, size, file) == (size_t)size;
	fclose(file);
	return written;
}

static Bool copyFile(const AsciiString &sourcePath, const AsciiString &targetPath)
{
	File *source = TheFileSystem->openFile(sourcePath.str(), File::READ | File::BINARY);
	if (source == nullptr)
		return FALSE;

	const Int size = source->size();
	char *data = source->readEntireAndClose();
	if (data == nullptr && size != 0)
		return FALSE;

	const Bool copied = writeFile(targetPath, data, size);
	delete[] data;
	return copied;
}

void SystemMaps::getNames(FilenameList &names)
{
	FilenameList mapPaths;
	TheFileSystem->getFileListInDirectory(MapsDirectory, "*.map", mapPaths, TRUE);

	for (FilenameListIter it = mapPaths.begin(); it != mapPaths.end(); ++it)
	{
		const AsciiString name = getSystemMapName(*it);
		if (name.isNotEmpty())
			names.insert(name);
	}
}

Bool SystemMaps::isOfficialName(const AsciiString &name)
{
	FilenameList names;
	getNames(names);
	return names.find(name) != names.end();
}

AsciiString SystemMaps::getOfficialGameName(const AsciiString &mapName)
{
	if (mapName.isEmpty() || TheMapCache == nullptr)
		return AsciiString::TheEmptyString;

	const UnicodeString gameName = TheGameText->fetch(mapName);
	if (!isOfficialGameName(gameName))
		return AsciiString::TheEmptyString;

	AsciiString officialGameName;
	officialGameName.translate(gameName);
	return officialGameName;
}

AsciiString SystemMaps::getCopyName(const AsciiString &name)
{
	AsciiString copyName;
	copyName.format("[system-%s] %s", getSourceId(name).str(), name.str());
	return copyName;
}

AsciiString SystemMaps::getUserMapPath(const AsciiString &name)
{
	return makeMapPath(makeUserMapsDirectory(), name);
}

Bool SystemMaps::copyToUserMaps(const AsciiString &name, const AsciiString &copyName)
{
	AsciiString sourceDirectory;
	sourceDirectory.format("%s%s\\", MapsDirectory, name.str());

	FilenameList files;
	TheFileSystem->getFileListInDirectory(sourceDirectory, "*.*", files, FALSE);
	if (files.empty())
		return FALSE;

	AsciiString targetDirectory = makeUserMapsDirectory();
	targetDirectory.concat(copyName);
	targetDirectory.concat('\\');
	NativeFileSystem::create_directories(targetDirectory);
	if (!NativeFileSystem::is_directory(targetDirectory.str()))
		return FALSE;

	for (FilenameListIter it = files.begin(); it != files.end(); ++it)
	{
		AsciiString targetPath = targetDirectory;
		targetPath.concat(makeCopyFileName(getFileName(*it), name, copyName));
		if (!copyFile(*it, targetPath))
			return FALSE;
	}

	return TRUE;
}
