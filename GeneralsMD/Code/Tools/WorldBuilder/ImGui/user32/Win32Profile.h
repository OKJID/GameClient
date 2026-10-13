#pragma once

#include <string>
#include <utility>
#include <vector>

struct Win32ProfileSection
{
	std::string name;
	std::vector<std::pair<std::string, std::string>> entries;
};

class Win32ProfileFile
{
public:
	explicit Win32ProfileFile(std::string path);

	const std::string* value(const std::string& section, const std::string& key) const;
	std::vector<std::string> sectionNames() const;
	std::vector<std::string> keyNames(const std::string& section) const;

	void setValue(const std::string& section, const std::string& key, const std::string& value);
	void removeKey(const std::string& section, const std::string& key);
	void removeSection(const std::string& section);
	bool save() const;

private:
	void load();
	Win32ProfileSection* findSection(const std::string& section);
	const Win32ProfileSection* findSection(const std::string& section) const;

	std::string m_path;
	std::vector<Win32ProfileSection> m_sections;
};

void Win32SetProfileDirectory(const std::string& directory);
