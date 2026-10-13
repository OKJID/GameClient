#include "Win32Profile.h"

#include "Win32Api.h"
#include "Win32Paths.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <strings.h>

namespace
{

struct Win32ProfileState
{
	std::string directory;
	std::map<std::string, std::unique_ptr<Win32ProfileFile>> files;
};

Win32ProfileState& state()
{
	static Win32ProfileState profileState;
	return profileState;
}

bool equalsNoCase(const std::string& first, const std::string& second)
{
	return strcasecmp(first.c_str(), second.c_str()) == 0;
}

std::string trim(const std::string& text)
{
	const size_t first = text.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
	{
		return std::string();
	}
	const size_t last = text.find_last_not_of(" \t\r\n");
	return text.substr(first, last - first + 1);
}

std::string unquoted(const std::string& text)
{
	const bool isQuoted = text.size() >= 2 && (text.front() == '"' || text.front() == '\'') && text.back() == text.front();
	return isQuoted ? text.substr(1, text.size() - 2) : text;
}

std::string resolvePath(LPCSTR fileName)
{
	const std::string path = fileName != nullptr ? fileName : "";
	return path.find('\\') == std::string::npos ? Win32JoinPath(state().directory, path) : path;
}

Win32ProfileFile& profileFor(LPCSTR fileName)
{
	const std::string path = resolvePath(fileName);
	std::unique_ptr<Win32ProfileFile>& file = state().files[path];
	if (file == nullptr)
	{
		file = std::make_unique<Win32ProfileFile>(path);
	}
	return *file;
}

DWORD copyString(const std::string& text, LPSTR buffer, DWORD bufferSize)
{
	const DWORD length = std::min<DWORD>(static_cast<DWORD>(text.size()), bufferSize - 1);
	memcpy(buffer, text.c_str(), length);
	buffer[length] = '\0';
	return length;
}

DWORD copyList(const std::vector<std::string>& names, LPSTR buffer, DWORD bufferSize)
{
	if (bufferSize < 2)
	{
		buffer[0] = '\0';
		return 0;
	}
	DWORD written = 0;
	for (const std::string& name : names)
	{
		if (written + name.size() + 2 > bufferSize)
		{
			buffer[bufferSize - 2] = '\0';
			buffer[bufferSize - 1] = '\0';
			return bufferSize - 2;
		}
		memcpy(buffer + written, name.c_str(), name.size() + 1);
		written += static_cast<DWORD>(name.size()) + 1;
	}
	buffer[written] = '\0';
	return written;
}

}

Win32ProfileFile::Win32ProfileFile(std::string path) :
	m_path(std::move(path))
{
	load();
}

void Win32ProfileFile::load()
{
	std::ifstream file(Win32NativePath(m_path));
	std::string line;
	Win32ProfileSection* current = nullptr;
	while (std::getline(file, line))
	{
		const std::string text = trim(line);
		if (text.empty() || text[0] == ';')
		{
			continue;
		}
		if (text.front() == '[' && text.back() == ']')
		{
			m_sections.push_back(Win32ProfileSection{ trim(text.substr(1, text.size() - 2)), {} });
			current = &m_sections.back();
			continue;
		}
		const size_t separator = text.find('=');
		if (current != nullptr && separator != std::string::npos)
		{
			current->entries.emplace_back(trim(text.substr(0, separator)), trim(text.substr(separator + 1)));
		}
	}
}

Win32ProfileSection* Win32ProfileFile::findSection(const std::string& section)
{
	const auto found = std::find_if(m_sections.begin(), m_sections.end(), [&](const Win32ProfileSection& candidate) { return equalsNoCase(candidate.name, section); });
	return found != m_sections.end() ? &*found : nullptr;
}

const Win32ProfileSection* Win32ProfileFile::findSection(const std::string& section) const
{
	const auto found = std::find_if(m_sections.begin(), m_sections.end(), [&](const Win32ProfileSection& candidate) { return equalsNoCase(candidate.name, section); });
	return found != m_sections.end() ? &*found : nullptr;
}

const std::string* Win32ProfileFile::value(const std::string& section, const std::string& key) const
{
	const Win32ProfileSection* found = findSection(section);
	if (found == nullptr)
	{
		return nullptr;
	}
	for (const auto& entry : found->entries)
	{
		if (equalsNoCase(entry.first, key))
		{
			return &entry.second;
		}
	}
	return nullptr;
}

std::vector<std::string> Win32ProfileFile::sectionNames() const
{
	std::vector<std::string> names;
	for (const Win32ProfileSection& section : m_sections)
	{
		names.push_back(section.name);
	}
	return names;
}

std::vector<std::string> Win32ProfileFile::keyNames(const std::string& section) const
{
	std::vector<std::string> names;
	const Win32ProfileSection* found = findSection(section);
	for (size_t index = 0; found != nullptr && index < found->entries.size(); ++index)
	{
		names.push_back(found->entries[index].first);
	}
	return names;
}

void Win32ProfileFile::setValue(const std::string& section, const std::string& key, const std::string& value)
{
	Win32ProfileSection* found = findSection(section);
	if (found == nullptr)
	{
		m_sections.push_back(Win32ProfileSection{ section, {} });
		found = &m_sections.back();
	}
	for (auto& entry : found->entries)
	{
		if (equalsNoCase(entry.first, key))
		{
			entry.second = value;
			return;
		}
	}
	found->entries.emplace_back(key, value);
}

void Win32ProfileFile::removeKey(const std::string& section, const std::string& key)
{
	Win32ProfileSection* found = findSection(section);
	if (found == nullptr)
	{
		return;
	}
	auto& entries = found->entries;
	entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const auto& entry) { return equalsNoCase(entry.first, key); }), entries.end());
}

void Win32ProfileFile::removeSection(const std::string& section)
{
	m_sections.erase(std::remove_if(m_sections.begin(), m_sections.end(),
		[&](const Win32ProfileSection& candidate) { return equalsNoCase(candidate.name, section); }), m_sections.end());
}

bool Win32ProfileFile::save() const
{
	std::ofstream file(Win32NativePath(m_path), std::ios::trunc);
	for (const Win32ProfileSection& section : m_sections)
	{
		file << '[' << section.name << "]\n";
		for (const auto& entry : section.entries)
		{
			file << entry.first << '=' << entry.second << '\n';
		}
		file << '\n';
	}
	return file.good();
}

void Win32SetProfileDirectory(const std::string& directory)
{
	state().directory = directory;
	state().files.clear();
}

UINT GetPrivateProfileIntA(LPCSTR section, LPCSTR key, INT defaultValue, LPCSTR fileName)
{
	const std::string* found = section != nullptr && key != nullptr ? profileFor(fileName).value(section, key) : nullptr;
	if (found == nullptr)
	{
		return static_cast<UINT>(defaultValue);
	}
	return static_cast<UINT>(strtol(unquoted(*found).c_str(), nullptr, 10));
}

DWORD GetPrivateProfileStringA(LPCSTR section, LPCSTR key, LPCSTR defaultValue, LPSTR buffer, DWORD bufferSize, LPCSTR fileName)
{
	if (buffer == nullptr || bufferSize == 0)
	{
		return 0;
	}
	Win32ProfileFile& file = profileFor(fileName);
	if (section == nullptr)
	{
		return copyList(file.sectionNames(), buffer, bufferSize);
	}
	if (key == nullptr)
	{
		return copyList(file.keyNames(section), buffer, bufferSize);
	}
	const std::string* found = file.value(section, key);
	const std::string text = found != nullptr ? unquoted(*found) : trim(defaultValue != nullptr ? defaultValue : "");
	return copyString(text, buffer, bufferSize);
}

BOOL WritePrivateProfileStringA(LPCSTR section, LPCSTR key, LPCSTR value, LPCSTR fileName)
{
	if (section == nullptr)
	{
		return TRUE;
	}
	Win32ProfileFile& file = profileFor(fileName);
	if (key == nullptr)
	{
		file.removeSection(section);
	}
	else if (value == nullptr)
	{
		file.removeKey(section, key);
	}
	else
	{
		file.setValue(section, key, value);
	}
	return file.save() ? TRUE : FALSE;
}
