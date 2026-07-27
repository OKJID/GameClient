#pragma once

#include "StdDevice/Common/StdLocalFileSystem.h"

#include <string>
#include <vector>
#include <filesystem>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

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
	std::filesystem::path resolveInSearchPaths(const Char *filename, Int access) const;

	static std::string makeCacheKey(const std::string& path);
	Bool isInsideMissingPath(const std::string& key) const;
	void rememberMissingPath(const std::filesystem::path& path) const;
	void forgetCachedPath(const Char *filename);
	void clearPathCache();

	std::vector<std::string> m_searchPaths;

	mutable std::unordered_map<std::string, std::string> m_resolvedPaths;
	mutable std::unordered_set<std::string> m_missingPaths;
	mutable std::shared_mutex m_cacheMutex;
};
