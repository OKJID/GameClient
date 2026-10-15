#pragma once

#include "Win32Api.h"

#include <string>
#include <vector>

struct Win32ListItem
{
	std::string text;
	DWORD_PTR data = 0;
	bool isSelected = false;
};

class Win32ItemList
{
public:
	int count() const { return static_cast<int>(m_items.size()); }
	bool isValid(int index) const { return index >= 0 && index < count(); }
	Win32ListItem& at(int index) { return m_items[static_cast<size_t>(index)]; }
	const Win32ListItem& at(int index) const { return m_items[static_cast<size_t>(index)]; }

	int add(const std::string& text, bool sorted);
	int insert(int index, const std::string& text);
	int remove(int index);
	void clear() { m_items.clear(); }
	int find(int startAfter, const std::string& prefix, bool exact) const;
	int copyText(int index, LPSTR buffer) const;
	int textLength(int index) const;

private:
	std::vector<Win32ListItem> m_items;
};

int Win32CompareNoCase(const std::string& first, const std::string& second);
