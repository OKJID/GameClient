#pragma once

#include "StdDevice/Common/StdLocalFileSystem.h"

#include <string>
#include <vector>
#include <filesystem>

class MacOSLocalFileSystem : public StdLocalFileSystem
{
public:
	MacOSLocalFileSystem();
	virtual ~MacOSLocalFileSystem() override;

	virtual void init() override;

	virtual File * openFile(const Char *filename, Int access = File::NONE, size_t bufferSize = File::BUFFERSIZE) override;
	virtual Bool doesFileExist(const Char *filename) const override;
	virtual void getFileListInDirectory(const AsciiString& currentDirectory, const AsciiString& originalDirectory, const AsciiString& searchName, FilenameList &filenameList, Bool searchSubdirectories) const override;
	virtual Bool getFileInfo(const AsciiString& filename, FileInfo *fileInfo) const override;
	virtual Bool createDirectory(AsciiString directory) override;
	virtual AsciiString normalizePath(const AsciiString& filePath) const override;

	void addSearchPath(const AsciiString& path);

protected:
	std::filesystem::path fixFilenameFromWindowsPath(const Char *filename, Int access) const;
	std::filesystem::path resolveWithSearchPaths(const Char *filename, Int access) const;

	std::vector<std::string> m_searchPaths;
};
