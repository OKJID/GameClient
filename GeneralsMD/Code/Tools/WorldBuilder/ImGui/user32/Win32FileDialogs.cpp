#include "Win32CommonDialogs.h"
#include "Win32Paths.h"
#include "Win32Platform.h"

#include <algorithm>
#include <cstring>

namespace
{

std::vector<std::string> extensionsOf(const std::string& patterns)
{
	std::vector<std::string> extensions;
	size_t start = 0;
	while (start <= patterns.size())
	{
		const size_t end = std::min(patterns.find(';', start), patterns.size());
		const std::string pattern = patterns.substr(start, end - start);
		const size_t dot = pattern.find_last_of('.');
		extensions.push_back(dot != std::string::npos ? pattern.substr(dot + 1) : pattern);
		start = end + 1;
	}
	return extensions;
}

std::vector<Win32FileFilter> parseFilters(LPCSTR filter)
{
	std::vector<Win32FileFilter> filters;
	for (LPCSTR current = filter; current != nullptr && *current != '\0';)
	{
		Win32FileFilter entry;
		entry.description = current;
		current += entry.description.size() + 1;
		if (*current == '\0')
		{
			break;
		}
		const std::string patterns = current;
		current += patterns.size() + 1;
		entry.extensions = extensionsOf(patterns);
		filters.push_back(entry);
	}
	return filters;
}

Win32FileDialogRequest requestFor(const OPENFILENAMEA& ofn, bool isOpenDialog)
{
	Win32FileDialogRequest request;
	request.isOpenDialog = isOpenDialog;
	request.title = ofn.lpstrTitle != nullptr ? ofn.lpstrTitle : (isOpenDialog ? "Open" : "Save As");
	request.defaultExtension = ofn.lpstrDefExt != nullptr ? ofn.lpstrDefExt : "";
	request.filters = parseFilters(ofn.lpstrFilter);
	request.confirmOverwrite = (ofn.Flags & OFN_OVERWRITEPROMPT) != 0;
	request.fileMustExist = (ofn.Flags & OFN_FILEMUSTEXIST) != 0;

	const std::string file = ofn.lpstrFile != nullptr ? ofn.lpstrFile : "";
	const size_t separator = file.find_last_of('\\');
	request.initialFileName = separator == std::string::npos ? file : file.substr(separator + 1);
	std::string directory = separator != std::string::npos ? file.substr(0, separator) : std::string();
	if (directory.empty() && ofn.lpstrInitialDir != nullptr)
	{
		directory = ofn.lpstrInitialDir;
	}
	if (directory.empty())
	{
		char current[MAX_PATH] = {};
		GetCurrentDirectoryA(sizeof(current), current);
		directory = current;
	}
	request.initialDirectory = Win32NativePath(directory);
	return request;
}

std::string withDefaultExtension(const std::string& path, LPCSTR defaultExtension)
{
	const size_t nameStart = path.find_last_of('\\') + 1;
	const size_t dot = path.find_last_of('.');
	const bool hasExtension = dot != std::string::npos && dot >= nameStart;
	if (hasExtension || defaultExtension == nullptr || *defaultExtension == '\0')
	{
		return path;
	}
	return path + "." + defaultExtension;
}

bool storeResult(OPENFILENAMEA& ofn, const std::string& path)
{
	if (ofn.lpstrFile == nullptr || path.size() + 1 > ofn.nMaxFile)
	{
		Win32SetCommonDialogError(FNERR_BUFFERTOOSMALL);
		if (ofn.lpstrFile != nullptr && ofn.nMaxFile >= sizeof(WORD))
		{
			const WORD required = static_cast<WORD>(path.size() + 1);
			memcpy(ofn.lpstrFile, &required, sizeof(required));
		}
		return false;
	}
	memcpy(ofn.lpstrFile, path.c_str(), path.size() + 1);
	const size_t nameStart = path.find_last_of('\\') + 1;
	const size_t dot = path.find_last_of('.');
	ofn.nFileOffset = static_cast<WORD>(nameStart);
	ofn.nFileExtension = static_cast<WORD>(dot != std::string::npos && dot >= nameStart ? dot + 1 : 0);
	ofn.Flags &= ~OFN_READONLY;
	if (ofn.lpstrFileTitle != nullptr && ofn.nMaxFileTitle > 0)
	{
		strlcpy(ofn.lpstrFileTitle, path.c_str() + nameStart, ofn.nMaxFileTitle);
	}
	return true;
}

BOOL runFileDialog(LPOPENFILENAME ofn, bool isOpenDialog)
{
	Win32SetCommonDialogError(0);
	Win32Platform* platform = Win32GetPlatform();
	if (ofn == nullptr || platform == nullptr)
	{
		Win32SetCommonDialogError(CDERR_DIALOGFAILURE);
		return FALSE;
	}
	std::string nativePath;
	if (!platform->chooseFile(requestFor(*ofn, isOpenDialog), nativePath))
	{
		return FALSE;
	}
	const std::string path = withDefaultExtension(Win32EnginePath(nativePath), ofn->lpstrDefExt);
	if (!storeResult(*ofn, path))
	{
		return FALSE;
	}
	if ((ofn->Flags & OFN_NOCHANGEDIR) == 0)
	{
		SetCurrentDirectoryA(path.substr(0, ofn->nFileOffset).c_str());
	}
	return TRUE;
}

}

BOOL GetOpenFileNameA(LPOPENFILENAME openFileName)
{
	return runFileDialog(openFileName, true);
}

BOOL GetSaveFileNameA(LPOPENFILENAME openFileName)
{
	return runFileDialog(openFileName, false);
}
