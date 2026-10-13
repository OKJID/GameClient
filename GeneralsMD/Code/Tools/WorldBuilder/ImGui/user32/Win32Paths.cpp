#include "Win32Paths.h"

#include "Win32Api.h"

#include <cstring>
#include <vector>

namespace
{

bool isSeparator(char ch)
{
	return ch == '\\' || ch == '/';
}

std::string absoluteEnginePath(LPCSTR fileName)
{
	if (isSeparator(fileName[0]))
	{
		return fileName;
	}
	char currentDirectory[MAX_PATH] = {};
	::GetCurrentDirectoryA(sizeof(currentDirectory), currentDirectory);
	return Win32JoinPath(currentDirectory, fileName);
}

std::vector<std::string> canonicalComponents(const std::string& path)
{
	std::vector<std::string> components;
	size_t start = 0;
	while (start <= path.size())
	{
		size_t end = start;
		while (end < path.size() && !isSeparator(path[end]))
		{
			++end;
		}
		const std::string component = path.substr(start, end - start);
		if (component == "..")
		{
			if (!components.empty())
			{
				components.pop_back();
			}
		}
		else if (!component.empty() && component != ".")
		{
			components.push_back(component);
		}
		start = end + 1;
	}
	return components;
}

}

std::string Win32NativePath(const std::string& enginePath)
{
	std::string path = enginePath;
	ConvertToNativePath(path.data());
	return path;
}

std::string Win32EnginePath(const std::string& nativePath)
{
	std::string path = nativePath;
	ConvertToEnginePath(path.data());
	return path;
}

std::string Win32JoinPath(const std::string& directory, const std::string& fileName)
{
	if (directory.empty() || directory.back() == '\\')
	{
		return directory + fileName;
	}
	return directory + "\\" + fileName;
}

DWORD GetFullPathNameA(LPCSTR fileName, DWORD bufferLength, LPSTR buffer, LPSTR* filePart)
{
	if (fileName == nullptr || fileName[0] == '\0')
	{
		return 0;
	}
	std::string fullPath;
	for (const std::string& component : canonicalComponents(absoluteEnginePath(fileName)))
	{
		fullPath += "\\" + component;
	}
	if (fullPath.empty())
	{
		fullPath = "\\";
	}

	if (buffer == nullptr || bufferLength <= fullPath.size())
	{
		return static_cast<DWORD>(fullPath.size() + 1);
	}
	memcpy(buffer, fullPath.c_str(), fullPath.size() + 1);
	if (filePart != nullptr)
	{
		const bool namesDirectory = isSeparator(fileName[strlen(fileName) - 1]);
		*filePart = namesDirectory ? nullptr : strrchr(buffer, '\\') + 1;
	}
	return static_cast<DWORD>(fullPath.size());
}
