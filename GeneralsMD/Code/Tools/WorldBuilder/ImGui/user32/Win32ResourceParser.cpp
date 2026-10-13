#include "Win32ResourceParser.h"

#include <cctype>
#include <cstdlib>
#include <set>

Win32ResourceParser::Win32ResourceParser(Win32Resources& resources, std::vector<Win32ResourceToken> tokens)
	: m_resources(resources), m_tokens(std::move(tokens))
{
}

bool Win32ResourceParser::nextIs(const char* text) const
{
	return !atEnd() && !peek().isQuoted && peek().text == text;
}

bool Win32ResourceParser::nextStartsArgument() const
{
	return !atEnd() && peek().separator == ',';
}

UINT Win32ResourceParser::resolveId(const std::string& text) const
{
	if (text.empty())
	{
		return 0;
	}
	const char first = text[0];
	if (std::isdigit(static_cast<unsigned char>(first)) || first == '-')
	{
		return static_cast<UINT>(std::strtol(text.c_str(), nullptr, 0));
	}

	UINT id = 0;
	m_resources.findId(text, id);
	return id;
}

int Win32ResourceParser::nextNumber()
{
	return atEnd() ? 0 : static_cast<int>(resolveId(next().text));
}

DWORD Win32ResourceParser::parseStyleExpression(DWORD style)
{
	bool isFirst = true;
	while (!atEnd())
	{
		if (!isFirst && peek().separator != '|')
		{
			break;
		}
		const bool negate = nextIs("NOT");
		if (negate)
		{
			++m_position;
		}
		const DWORD value = resolveId(next().text);
		style = negate ? (style & ~value) : (style | value);
		isFirst = false;
	}
	return style;
}

void Win32ResourceParser::skipUntilBegin()
{
	while (!atEnd() && !nextIs("BEGIN"))
	{
		++m_position;
	}
	if (!atEnd())
	{
		++m_position;
	}
}

void Win32ResourceParser::skipBlock()
{
	int depth = 1;
	while (!atEnd() && depth > 0)
	{
		const Win32ResourceToken& token = next();
		if (token.isQuoted)
		{
			continue;
		}
		depth += token.text == "BEGIN" ? 1 : token.text == "END" ? -1 : 0;
	}
}

void Win32ResourceParser::finishBlock()
{
	if (!atEnd())
	{
		++m_position;
	}
}

void Win32ResourceParser::parse()
{
	while (!atEnd())
	{
		const Win32ResourceToken first = next();
		if (first.text == "LANGUAGE")
		{
			m_position += 2;
			continue;
		}
		if (first.text == "STRINGTABLE")
		{
			parseStringTable();
			continue;
		}
		if (atEnd())
		{
			return;
		}
		parseResource(first.text, next().text);
	}
}

void Win32ResourceParser::parseResource(const std::string& name, const std::string& type)
{
	const UINT id = resolveId(name);
	if (type == "CURSOR") { parseFileResource(m_resources.m_cursors, id); return; }
	if (type == "BITMAP") { parseFileResource(m_resources.m_bitmaps, id); return; }
	if (type == "ICON") { parseFileResource(m_resources.m_icons, id); return; }
	if (type == "MENU") { parseMenu(id); return; }
	if (type == "ACCELERATORS") { parseAccelerators(id); return; }
	if (type == "TOOLBAR") { parseToolbar(id); return; }
	if (type == "DIALOG" || type == "DIALOGEX") { parseDialog(id, type == "DIALOGEX"); return; }
	if (type == "DLGINIT") { parseDialogInit(id); return; }
	skipUntilBegin();
	skipBlock();
}

void Win32ResourceParser::parseFileResource(std::map<UINT, std::string>& files, UINT id)
{
	while (!atEnd() && !peek().isQuoted)
	{
		++m_position;
	}
	if (atEnd())
	{
		return;
	}
	files[id] = Win32UnescapeResourceString(next().text);
}

void Win32ResourceParser::parseStringTable()
{
	skipUntilBegin();
	while (!atEnd() && !nextIs("END"))
	{
		const UINT id = resolveId(next().text);
		if (atEnd() || !peek().isQuoted)
		{
			continue;
		}
		m_resources.m_strings[id] = Win32UnescapeResourceString(next().text);
	}
	finishBlock();
}

UINT Win32ResourceParser::parseMenuOptions()
{
	UINT flags = 0;
	while (!atEnd() && !peek().isQuoted)
	{
		const std::string& option = peek().text;
		const UINT flag = option == "GRAYED" ? MF_GRAYED : option == "CHECKED" ? MF_CHECKED : option == "INACTIVE" ? MF_DISABLED
			: option == "MENUBARBREAK" ? MF_MENUBARBREAK : option == "MENUBREAK" ? MF_MENUBREAK : option == "HELP" ? 0x4000 : 0xFFFFFFFF;
		if (flag == 0xFFFFFFFF)
		{
			break;
		}
		flags |= flag;
		++m_position;
	}
	return flags;
}

void Win32ResourceParser::parseMenu(UINT id)
{
	skipUntilBegin();
	Win32MenuTemplateItem& root = m_resources.m_menus[id];
	root.isPopup = true;
	parseMenuItems(root.children);
}

void Win32ResourceParser::parseMenuItems(std::vector<Win32MenuTemplateItem>& items)
{
	while (!atEnd() && !nextIs("END"))
	{
		parseMenuItem(items);
	}
	finishBlock();
}

void Win32ResourceParser::parseMenuItem(std::vector<Win32MenuTemplateItem>& items)
{
	const Win32ResourceToken keyword = next();
	Win32MenuTemplateItem item;
	if (keyword.text == "POPUP")
	{
		item.isPopup = true;
		item.text = Win32UnescapeResourceString(next().text);
		item.flags = parseMenuOptions();
		skipUntilBegin();
		parseMenuItems(item.children);
		items.push_back(item);
		return;
	}
	if (keyword.text != "MENUITEM")
	{
		return;
	}
	if (nextIs("SEPARATOR"))
	{
		++m_position;
		item.isSeparator = true;
		items.push_back(item);
		return;
	}

	item.text = Win32UnescapeResourceString(next().text);
	item.id = resolveId(next().text);
	item.flags = parseMenuOptions();
	items.push_back(item);
}

void Win32ResourceParser::parseAcceleratorKey(const Win32ResourceToken& token, Win32AcceleratorTemplate& accelerator) const
{
	if (!token.isQuoted)
	{
		accelerator.virtualKey = resolveId(token.text);
		return;
	}
	if (token.text.size() >= 2 && token.text[0] == '^')
	{
		accelerator.control = true;
		accelerator.virtualKey = static_cast<UINT>(std::toupper(static_cast<unsigned char>(token.text[1])));
		return;
	}
	accelerator.virtualKey = token.text.empty() ? 0 : static_cast<unsigned char>(token.text[0]);
}

void Win32ResourceParser::parseAccelerators(UINT id)
{
	static const std::set<std::string> kFlags = { "VIRTKEY", "ASCII", "CONTROL", "SHIFT", "ALT", "NOINVERT" };

	skipUntilBegin();
	std::vector<Win32AcceleratorTemplate>& table = m_resources.m_accelerators[id];
	while (!atEnd() && !nextIs("END"))
	{
		Win32AcceleratorTemplate accelerator;
		parseAcceleratorKey(next(), accelerator);
		accelerator.id = resolveId(next().text);
		while (!atEnd() && !peek().isQuoted && kFlags.count(peek().text) != 0)
		{
			const std::string& flag = next().text;
			accelerator.isVirtualKey = accelerator.isVirtualKey || flag == "VIRTKEY";
			accelerator.control = accelerator.control || flag == "CONTROL";
			accelerator.shift = accelerator.shift || flag == "SHIFT";
			accelerator.alt = accelerator.alt || flag == "ALT";
		}
		table.push_back(accelerator);
	}
	finishBlock();
}

void Win32ResourceParser::parseToolbar(UINT id)
{
	Win32ToolbarTemplate& toolbar = m_resources.m_toolbars[id];
	while (!atEnd() && !nextIs("BEGIN") && !std::isdigit(static_cast<unsigned char>(peek().text[0])))
	{
		++m_position;
	}
	toolbar.buttonWidth = nextNumber();
	toolbar.buttonHeight = nextNumber();
	skipUntilBegin();
	while (!atEnd() && !nextIs("END"))
	{
		const std::string keyword = next().text;
		toolbar.buttons.push_back(keyword == "BUTTON" ? resolveId(next().text) : 0);
	}
	finishBlock();
}
