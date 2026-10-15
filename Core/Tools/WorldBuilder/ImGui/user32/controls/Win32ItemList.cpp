#include "Win32ItemList.h"

#include <cctype>
#include <cstring>
#include <strings.h>

int Win32CompareNoCase(const std::string& first, const std::string& second)
{
	return strcasecmp(first.c_str(), second.c_str());
}

int Win32ItemList::add(const std::string& text, bool sorted)
{
	if (!sorted)
	{
		m_items.push_back({ text, 0, false });
		return count() - 1;
	}

	size_t position = 0;
	while (position < m_items.size() && Win32CompareNoCase(m_items[position].text, text) <= 0)
	{
		++position;
	}
	m_items.insert(m_items.begin() + static_cast<long>(position), { text, 0, false });
	return static_cast<int>(position);
}

int Win32ItemList::insert(int index, const std::string& text)
{
	if (index < 0 || index > count())
	{
		m_items.push_back({ text, 0, false });
		return count() - 1;
	}
	m_items.insert(m_items.begin() + index, { text, 0, false });
	return index;
}

int Win32ItemList::remove(int index)
{
	if (!isValid(index))
	{
		return -1;
	}
	m_items.erase(m_items.begin() + index);
	return count();
}

int Win32ItemList::find(int startAfter, const std::string& prefix, bool exact) const
{
	const int total = count();
	const int first = startAfter < 0 || startAfter >= total ? 0 : startAfter + 1;
	for (int step = 0; step < total; ++step)
	{
		const int index = (first + step) % total;
		const std::string& text = at(index).text;
		const bool matches = exact ? Win32CompareNoCase(text, prefix) == 0 : strncasecmp(text.c_str(), prefix.c_str(), prefix.size()) == 0;
		if (matches)
		{
			return index;
		}
	}
	return -1;
}

int Win32ItemList::copyText(int index, LPSTR buffer) const
{
	if (!isValid(index) || buffer == nullptr)
	{
		return -1;
	}
	const std::string& text = at(index).text;
	memcpy(buffer, text.c_str(), text.size() + 1);
	return static_cast<int>(text.size());
}

int Win32ItemList::textLength(int index) const
{
	return isValid(index) ? static_cast<int>(at(index).text.size()) : -1;
}
