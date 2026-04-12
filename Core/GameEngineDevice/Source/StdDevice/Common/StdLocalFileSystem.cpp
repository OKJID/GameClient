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

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

///////// StdLocalFileSystem.cpp /////////////////////////
// Stephan Vedder, April 2025
////////////////////////////////////////////////////////////

#include "Common/AsciiString.h"
#include "Common/GameMemory.h"
#include "Common/PerfTimer.h"
#include "StdDevice/Common/StdLocalFileSystem.h"
#include "StdDevice/Common/StdLocalFile.h"

#include <algorithm>
#include "Common/System/NativeFileSystem.h"

StdLocalFileSystem::StdLocalFileSystem() : LocalFileSystem()
{
}

StdLocalFileSystem::~StdLocalFileSystem() {
}

File * StdLocalFileSystem::openFile(const Char *filename, Int access, size_t bufferSize)
{
	// sanity check
	if (!filename || strlen(filename) <= 0) {
		return nullptr;
	}

	if (access & File::WRITE) {
		// if opening the file for writing, we need to make sure the directory is there
		// Extract directory path by finding the last slash or backslash
		std::string pathStr(filename);
		size_t lastSlash = pathStr.find_last_of("\\/");
		if (lastSlash != std::string::npos) {
			std::string dir = pathStr.substr(0, lastSlash);
			if (!NativeFileSystem::exists(dir)) {
				if (!NativeFileSystem::create_directories(dir)) {
					DEBUG_LOG(("StdLocalFileSystem::openFile - Error creating directory %s", dir.c_str()));
					return nullptr;
				}
			}
		}
	}

	StdLocalFile *file = newInstance( StdLocalFile );

	if (file->open(filename, access, bufferSize) == FALSE) {
		deleteInstance(file);
		file = nullptr;
	} else {
		file->deleteOnClose();
	}

	return file;
}

void StdLocalFileSystem::update()
{
}

void StdLocalFileSystem::init()
{
}

void StdLocalFileSystem::reset()
{
}

Bool StdLocalFileSystem::doesFileExist(const Char *filename) const
{
	if (!filename || strlen(filename) <= 0) {
		return FALSE;
	}
	return NativeFileSystem::exists(std::string(filename));
}

void StdLocalFileSystem::getFileListInDirectory(const AsciiString& currentDirectory, const AsciiString& originalDirectory, const AsciiString& searchName, FilenameList & filenameList, Bool searchSubdirectories) const
{
	AsciiString asciisearch = originalDirectory;
	asciisearch.concat(currentDirectory);
	if (asciisearch.isEmpty()) {
		asciisearch = ".";
	}

	std::string searchExt = "";
	const char* dot = strrchr(searchName.str(), '.');
	if (dot) {
		searchExt = dot;
	}

	std::vector<std::string> outFiles;
	// Do not use recursive internally because we must construct the paths manually for the engine.
	NativeFileSystem::list_files(asciisearch.str(), searchExt, false, outFiles);

	for (const std::string& filenameStr : outFiles) {
		AsciiString newFilename = originalDirectory;
		newFilename.concat(currentDirectory);
		newFilename.concat(filenameStr.c_str());
		if (filenameList.find(newFilename) == filenameList.end()) {
			filenameList.insert(newFilename);
		}
	}

	if (searchSubdirectories) {
		std::vector<std::string> subDirs;
		NativeFileSystem::list_directories(asciisearch.str(), subDirs);
		
		for (const std::string& dirName : subDirs) {
			AsciiString tempsearchstr(currentDirectory);
			tempsearchstr.concat(dirName.c_str());
			tempsearchstr.concat('\\');

			// recursively add files in subdirectories if required.
			getFileListInDirectory(tempsearchstr, originalDirectory, searchName, filenameList, searchSubdirectories);
		}
	}
}

Bool StdLocalFileSystem::getFileInfo(const AsciiString& filename, FileInfo *fileInfo) const
{
	if(filename.isEmpty()) {
		return FALSE;
	}

	uint32_t sh = 0, sl = 0, th = 0, tl = 0;
	if (NativeFileSystem::get_file_info(filename, sh, sl, th, tl)) {
		fileInfo->sizeHigh = sh;
		fileInfo->sizeLow = sl;
		fileInfo->timestampHigh = th;
		fileInfo->timestampLow = tl;
		return TRUE;
	}
	return FALSE;
}

Bool StdLocalFileSystem::createDirectory(AsciiString directory)
{
	if (directory.isEmpty() || directory.getLength() >= _MAX_DIR) {
		return FALSE;
	}
	return NativeFileSystem::create_directory(directory);
}

AsciiString StdLocalFileSystem::normalizePath(const AsciiString& filePath) const
{
	return AsciiString(NativeFileSystem::normalize_path(filePath.str()).c_str());
}
