#include "MacOSLocalFileSystem.h"
#include "StdDevice/Common/StdLocalFile.h"
#include "Common/GameMemory.h"
#include <algorithm>
#include <cctype>
#include <strings.h>

MacOSLocalFileSystem::MacOSLocalFileSystem() : StdLocalFileSystem()
{
}

MacOSLocalFileSystem::~MacOSLocalFileSystem()
{
}

void MacOSLocalFileSystem::init()
{
	const char* installPath = getenv("GENERALS_ACTIVE_INSTALL_PATH");
	if (installPath && installPath[0]) {
		addSearchPath(AsciiString(installPath));
	}

	// [OKJI] Only the running game's own install is searched, strictly following the
	// "Windows Flow": Zero Hour NEVER scans the Vanilla Base directory for loose files.
	// Reading Vanilla loose files replaces the expected ZeroHour .big archive data,
	// causing INI parsing crashes due to discarded Vanilla enums in the C++ executable.
	// The same holds in reverse for the Vanilla build.
}

std::filesystem::path MacOSLocalFileSystem::fixFilenameFromWindowsPath(const Char *filename, Int access) const
{
	std::string fixedFilename(filename);

	// Slash Boundary (Inbound): Replace backslashes with forward slashes
	std::replace(fixedFilename.begin(), fixedFilename.end(), '\\', '/');

	const Bool readOnly = !(access & File::WRITE);

	if (readOnly && isInsideMissingPath(makeCacheKey(fixedFilename))) {
		return std::filesystem::path();
	}

	std::filesystem::path path(std::move(fixedFilename));

	std::error_code ec;
	if (!std::filesystem::exists(path, ec) &&
		((!(access & File::WRITE)) || ((access & File::WRITE) && !std::filesystem::exists(path.parent_path(), ec))))
	{
		std::filesystem::path pathFixed;
		std::filesystem::path pathCurrent;
		for (const auto& p : path)
		{
			std::filesystem::path pathFixedPart;
			if (pathCurrent.empty())
			{
				pathFixed /= p;
				pathCurrent /= p;
				continue;
			}

			if (std::filesystem::exists(pathCurrent / p, ec))
			{
				pathFixedPart = p;
			}
			else if (std::filesystem::exists(pathFixed / p, ec))
			{
				pathFixedPart = p;
			}
			else
			{
				for (auto& entry : std::filesystem::directory_iterator(pathFixed, ec))
				{
					if (strcasecmp(entry.path().filename().string().c_str(), p.string().c_str()) == 0)
					{
						pathFixedPart = entry.path().filename();
						break;
					}
				}
			}

			if (pathFixedPart.empty())
			{
				if (readOnly)
				{
					rememberMissingPath(pathFixed / p);
					return std::filesystem::path();
				}
				pathFixedPart = p;
			}
			pathFixed /= pathFixedPart;
			pathCurrent /= p;
		}
		path = pathFixed;
	}

	return path;
}

std::string MacOSLocalFileSystem::makeCacheKey(const std::string& path)
{
	std::string key(path);
	std::replace(key.begin(), key.end(), '\\', '/');
	std::transform(key.begin(), key.end(), key.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return key;
}

Bool MacOSLocalFileSystem::isInsideMissingPath(const std::string& key) const
{
	std::shared_lock<std::shared_mutex> lock(m_cacheMutex);

	if (m_missingPaths.empty()) {
		return FALSE;
	}

	for (std::string::size_type slash = key.find('/'); slash != std::string::npos; slash = key.find('/', slash + 1)) {
		if (m_missingPaths.count(key.substr(0, slash)) != 0) {
			return TRUE;
		}
	}

	return m_missingPaths.count(key) != 0;
}

void MacOSLocalFileSystem::rememberMissingPath(const std::filesystem::path& path) const
{
	std::unique_lock<std::shared_mutex> lock(m_cacheMutex);
	m_missingPaths.insert(makeCacheKey(path.string()));
}

void MacOSLocalFileSystem::forgetCachedPath(const Char *filename)
{
	std::unique_lock<std::shared_mutex> lock(m_cacheMutex);
	m_resolvedPaths.erase(makeCacheKey(filename));
	m_missingPaths.clear();
}

void MacOSLocalFileSystem::clearPathCache()
{
	std::unique_lock<std::shared_mutex> lock(m_cacheMutex);
	m_resolvedPaths.clear();
	m_missingPaths.clear();
}

std::filesystem::path MacOSLocalFileSystem::resolveInSearchPaths(const Char *filename, Int access) const
{
	std::filesystem::path path = fixFilenameFromWindowsPath(filename, access);
	if (!path.empty()) {
		return path;
	}

	if (access & File::WRITE) {
		return path;
	}

	std::string fixedRelative(filename);
	std::replace(fixedRelative.begin(), fixedRelative.end(), '\\', '/');

	for (const auto& searchPath : m_searchPaths) {
		std::string fullPath = searchPath + fixedRelative;
		std::filesystem::path resolved = fixFilenameFromWindowsPath(fullPath.c_str(), access);
		if (!resolved.empty()) {
			return resolved;
		}
	}

	return std::filesystem::path();
}

std::filesystem::path MacOSLocalFileSystem::resolveWithSearchPaths(const Char *filename, Int access) const
{
	if (access & File::WRITE) {
		return resolveInSearchPaths(filename, access);
	}

	const std::string key = makeCacheKey(filename);

	{
		std::shared_lock<std::shared_mutex> lock(m_cacheMutex);
		auto cached = m_resolvedPaths.find(key);
		if (cached != m_resolvedPaths.end()) {
			return cached->second.empty()
				? std::filesystem::path()
				: std::filesystem::path(cached->second);
		}
	}

	std::filesystem::path resolved = resolveInSearchPaths(filename, access);

	{
		std::unique_lock<std::shared_mutex> lock(m_cacheMutex);
		m_resolvedPaths[key] = resolved.string();
	}

	return resolved;
}

void MacOSLocalFileSystem::addSearchPath(const AsciiString& path)
{
	if (path.isEmpty()) {
		return;
	}

	std::string normalized = path.str();
	std::replace(normalized.begin(), normalized.end(), '\\', '/');

	if (normalized.back() != '/') {
		normalized += '/';
	}

	for (const auto& existing : m_searchPaths) {
		if (existing == normalized) {
			return;
		}
	}

	printf("MacOSLocalFileSystem::addSearchPath - '%s'\n", normalized.c_str());
	fflush(stdout);
	m_searchPaths.push_back(std::move(normalized));
	clearPathCache();
}

File * MacOSLocalFileSystem::openFile(const Char *filename, Int access, size_t bufferSize)
{
	if (strlen(filename) <= 0) {
		return nullptr;
	}

	std::filesystem::path path = resolveWithSearchPaths(filename, access);

	if (path.empty()) {
		return nullptr;
	}

	if (access & File::WRITE) {
		std::filesystem::path dir = path.parent_path();
		std::error_code ec;
		if (!std::filesystem::exists(dir, ec) || ec) {
			if(!std::filesystem::create_directories(dir, ec) || ec) {
				return nullptr;
			}
		}
	}

	StdLocalFile *file = newInstance( StdLocalFile );

	if (file->open(path.string().c_str(), access, bufferSize) == FALSE) {
		deleteInstance(file);
		file = nullptr;
	} else {
		file->deleteOnClose();
	}

	if (file != nullptr && (access & File::WRITE)) {
		forgetCachedPath(filename);
	}

	return file;
}

Bool MacOSLocalFileSystem::doesFileExist(const Char *filename) const
{
	std::filesystem::path path = resolveWithSearchPaths(filename, 0);
	if(path.empty()) {
		return FALSE;
	}

	std::error_code ec;
	return std::filesystem::exists(path, ec);
}

Bool MacOSLocalFileSystem::getFileInfo(const AsciiString& filename, FileInfo *fileInfo) const
{
	std::filesystem::path path = resolveWithSearchPaths(filename.str(), 0);

	if(path.empty()) {
		return FALSE;
	}

	std::error_code ec;
	auto file_size = std::filesystem::file_size(path, ec);
	if (ec)
	{
		return FALSE;
	}

	auto write_time = std::filesystem::last_write_time(path, ec);
	if (ec)
	{
		return FALSE;
	}

	auto time = write_time.time_since_epoch().count();
	fileInfo->timestampHigh = time >> 32;
	fileInfo->timestampLow = time & UINT32_MAX;
	fileInfo->sizeHigh      = file_size >> 32;
	fileInfo->sizeLow  = file_size & UINT32_MAX;

	return TRUE;
}

void MacOSLocalFileSystem::getFileListInDirectory(const AsciiString& currentDirectory, const AsciiString& originalDirectory, const AsciiString& searchName, FilenameList &filenameList, Bool searchSubdirectories) const
{
	AsciiString asciisearch;
	asciisearch = originalDirectory;
	asciisearch.concat(currentDirectory);
	auto searchExt = std::filesystem::path(searchName.str()).extension();
	if (asciisearch.isEmpty()) {
		asciisearch = ".";
	}

	std::vector<std::filesystem::path> allPaths;
	std::filesystem::path cwdPath = fixFilenameFromWindowsPath(asciisearch.str(), 0);
	if (!cwdPath.empty()) {
		allPaths.push_back(cwdPath);
	}

	std::string fixedRelative(asciisearch.str());
	std::replace(fixedRelative.begin(), fixedRelative.end(), '\\', '/');

	for (const auto& searchPath : m_searchPaths) {
		std::string fullPath = searchPath + fixedRelative;
		std::filesystem::path resolved = fixFilenameFromWindowsPath(fullPath.c_str(), 0);
		if (!resolved.empty()) {
			bool exists = false;
			for (const auto& existing : allPaths) {
				if (existing == resolved) { exists = true; break; }
			}
			if (!exists) {
				allPaths.push_back(resolved);
			}
		}
	}

	if (allPaths.empty()) {
		return;
	}

	for (const auto& fixedPath : allPaths) {
		std::string fixedDirectory = fixedPath.string();
		Bool done = FALSE;
		std::error_code ec;

		auto iter = std::filesystem::directory_iterator(fixedDirectory.c_str(), ec);
		done = iter == std::filesystem::directory_iterator();

		if (!ec) {
			while (!done)	{
				std::string filenameStr = iter->path().filename().string();
				std::string ext = iter->path().extension().string();
				bool extMatch = strcasecmp(ext.c_str(), searchExt.string().c_str()) == 0;

				// Entries the process cannot stat (system files under a denied TCC scope) must be
				// skipped: the throwing overload would abort startup instead of ignoring them.
				std::error_code statEc;
				const bool isDirectory = iter->is_directory(statEc);

				if (!statEc && !isDirectory && extMatch &&
					(strcmp(filenameStr.c_str(), ".") != 0 && strcmp(filenameStr.c_str(), "..") != 0)) {
					
					AsciiString newFilename = asciisearch;
					if (newFilename.str()[newFilename.getLength()-1] != '\\' && newFilename.str()[newFilename.getLength()-1] != '/') {
						newFilename.concat('\\');
					}
					newFilename.concat(filenameStr.c_str());
					
					// Slash Boundary (Outbound): ensure engine sees only backslashes
					std::string outStr = newFilename.str();
					std::replace(outStr.begin(), outStr.end(), '/', '\\');
					
					AsciiString finalOut(outStr.c_str());

					if (filenameList.find(finalOut) == filenameList.end()) {
						filenameList.insert(finalOut);
					}
				}

				std::error_code iterEc;
				iter.increment(iterEc);
				
				if (iterEc) {
					break;
				}

				done = iter == std::filesystem::directory_iterator();
			}
		}

		if (searchSubdirectories) {
			auto subIter = std::filesystem::directory_iterator(fixedDirectory, ec);
			if (ec) {
				continue;
			}

			done = subIter == std::filesystem::directory_iterator();

			while (!done) {
				std::string filenameStr = subIter->path().filename().string();
				// A name carrying a separator is not a real subdirectory but a Windows path
				// that leaked past the translation boundary. Recursing into it would re-read
				// it as a path and walk outside the install directory.
				const bool nameIsLeakedPath = filenameStr.find('\\') != std::string::npos;

				std::error_code subStatEc;
				const bool subIsDirectory = subIter->is_directory(subStatEc);

				if(!subStatEc && subIsDirectory && !nameIsLeakedPath &&
					(strcmp(filenameStr.c_str(), ".") != 0 && strcmp(filenameStr.c_str(), "..") != 0)) {
					AsciiString tempsearchstr = currentDirectory;
					if (!tempsearchstr.isEmpty()) {
						tempsearchstr.concat("\\");
					}
					tempsearchstr.concat(filenameStr.c_str());

					getFileListInDirectory(tempsearchstr, originalDirectory, searchName, filenameList, searchSubdirectories);
				}

				std::error_code subIterEc;
				subIter.increment(subIterEc);
				if (subIterEc) {
					break;
				}

				done = subIter == std::filesystem::directory_iterator();
			}
		}
	}
}

Bool MacOSLocalFileSystem::createDirectory(AsciiString directory)
{
	if (directory.isEmpty() || directory.getLength() >= _MAX_DIR) {
		return FALSE;
	}

	std::string fixedDirectory(directory.str());
	std::replace(fixedDirectory.begin(), fixedDirectory.end(), '\\', '/');

	std::filesystem::path path(std::move(fixedDirectory));

	std::error_code ec;
	std::filesystem::create_directories(path, ec);
	if (ec) {
		return FALSE;
	}

	clearPathCache();

	return TRUE;
}

AsciiString MacOSLocalFileSystem::normalizePath(const AsciiString& filePath) const
{
	std::string pathStr(filePath.str());
	std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

	std::filesystem::path normalized(pathStr);
	std::string result = normalized.lexically_normal().string();

	std::replace(result.begin(), result.end(), '/', '\\');

	return AsciiString(result.c_str());
}
