#include "Win32ResourceParser.h"

#include <cctype>
#include <cstdlib>
#include <set>

namespace
{

struct ControlKind
{
	const char* keyword;
	const char* className;
	DWORD defaultStyle;
	bool hasText;
	bool hasClassAndStyleFirst;
	DWORD typeMask = 0;
};

const DWORD kPushBoxStyle = 0x0000000AL;

const ControlKind kControlKinds[] = {
	{ "LTEXT", "Static", SS_LEFT | WS_GROUP, true, false },
	{ "RTEXT", "Static", SS_RIGHT | WS_GROUP, true, false },
	{ "CTEXT", "Static", SS_CENTER | WS_GROUP, true, false },
	{ "PUSHBUTTON", "Button", BS_PUSHBUTTON | WS_TABSTOP, true, false },
	{ "DEFPUSHBUTTON", "Button", BS_DEFPUSHBUTTON | WS_TABSTOP, true, false },
	{ "PUSHBOX", "Button", kPushBoxStyle | WS_TABSTOP, true, false },
	{ "CHECKBOX", "Button", BS_CHECKBOX | WS_TABSTOP, true, false },
	{ "AUTOCHECKBOX", "Button", BS_AUTOCHECKBOX | WS_TABSTOP, true, false },
	{ "RADIOBUTTON", "Button", BS_RADIOBUTTON, true, false },
	{ "AUTORADIOBUTTON", "Button", BS_AUTORADIOBUTTON, true, false },
	{ "STATE3", "Button", BS_3STATE | WS_TABSTOP, true, false },
	{ "AUTO3STATE", "Button", BS_AUTO3STATE | WS_TABSTOP, true, false },
	{ "GROUPBOX", "Button", BS_GROUPBOX, true, false },
	{ "EDITTEXT", "Edit", ES_LEFT | WS_BORDER | WS_TABSTOP, false, false },
	{ "COMBOBOX", "ComboBox", CBS_SIMPLE | WS_TABSTOP, false, false, CBS_DROPDOWNLIST },
	{ "LISTBOX", "ListBox", LBS_NOTIFY | WS_BORDER, false, false },
	{ "SCROLLBAR", "ScrollBar", SBS_HORZ, false, false },
	{ "ICON", "Static", SS_ICON, true, false },
	{ "CONTROL", "", 0, true, true },
};

const ControlKind* findControlKind(const std::string& keyword)
{
	for (const ControlKind& kind : kControlKinds)
	{
		if (keyword == kind.keyword)
		{
			return &kind;
		}
	}
	return nullptr;
}

const std::set<std::string> kMemoryOptions = { "DISCARDABLE", "MOVEABLE", "PURE", "IMPURE", "PRELOAD", "LOADONCALL", "FIXED", "SHARED", "NONSHARED" };

const DWORD kDefaultDialogStyle = WS_POPUP | WS_BORDER | WS_SYSMENU;
const DWORD kControlBaseStyle = WS_CHILD | WS_VISIBLE;

}

void Win32ResourceParser::parseDialog(UINT id, bool isExtended)
{
	Win32DialogTemplate& dialog = m_resources.m_dialogs[id];
	dialog = Win32DialogTemplate();
	parseDialogHeader(dialog, isExtended);

	bool hasStyle = false;
	while (!atEnd() && !nextIs("BEGIN"))
	{
		if (!parseDialogStatement(dialog, hasStyle))
		{
			++m_position;
		}
	}
	if (!hasStyle)
	{
		dialog.style = kDefaultDialogStyle | (dialog.caption.empty() ? 0 : WS_CAPTION);
	}
	if (!dialog.fontName.empty())
	{
		dialog.style |= DS_SETFONT;
	}

	finishBlock();
	parseDialogItems(dialog, isExtended);
}

void Win32ResourceParser::parseDialogHeader(Win32DialogTemplate& dialog, bool isExtended)
{
	while (!atEnd() && kMemoryOptions.count(peek().text) != 0)
	{
		++m_position;
	}
	dialog.x = nextNumber();
	dialog.y = nextNumber();
	dialog.cx = nextNumber();
	dialog.cy = nextNumber();
	if (isExtended && nextStartsArgument())
	{
		nextNumber();
	}
}

bool Win32ResourceParser::parseDialogStatement(Win32DialogTemplate& dialog, bool& hasStyle)
{
	const std::string keyword = peek().text;
	if (keyword == "STYLE")
	{
		++m_position;
		dialog.style = parseStyleExpression(0);
		hasStyle = true;
		return true;
	}
	if (keyword == "EXSTYLE")
	{
		++m_position;
		dialog.exStyle = parseStyleExpression(0);
		return true;
	}
	if (keyword == "CAPTION")
	{
		++m_position;
		dialog.caption = Win32UnescapeResourceString(next().text);
		dialog.style |= WS_CAPTION;
		return true;
	}
	if (keyword == "FONT")
	{
		++m_position;
		dialog.fontSize = nextNumber();
		dialog.fontName = Win32UnescapeResourceString(next().text);
		while (nextStartsArgument())
		{
			++m_position;
		}
		return true;
	}
	if (keyword == "MENU" || keyword == "CLASS")
	{
		++m_position;
		(keyword == "MENU" ? dialog.menuName : dialog.className) = Win32UnescapeResourceString(next().text);
		return true;
	}
	return false;
}

void Win32ResourceParser::parseDialogItems(Win32DialogTemplate& dialog, bool isExtended)
{
	while (!atEnd() && !nextIs("END"))
	{
		const std::string keyword = next().text;
		if (findControlKind(keyword) != nullptr)
		{
			parseDialogItem(dialog, keyword, isExtended);
		}
	}
	finishBlock();
}

void Win32ResourceParser::parseItemText(Win32DialogItemTemplate& item)
{
	const Win32ResourceToken& token = next();
	if (token.isQuoted)
	{
		item.text = Win32UnescapeResourceString(token.text);
		return;
	}
	item.textOrdinal = resolveId(token.text);
}

void Win32ResourceParser::parseItemTail(Win32DialogItemTemplate& item, bool isExtended)
{
	if (nextStartsArgument())
	{
		item.style = parseStyleExpression(item.style);
	}
	if (nextStartsArgument())
	{
		item.exStyle = parseStyleExpression(0);
	}
	if (isExtended && nextStartsArgument())
	{
		nextNumber();
	}
}

void Win32ResourceParser::parseDialogItem(Win32DialogTemplate& dialog, const std::string& keyword, bool isExtended)
{
	const ControlKind* kind = findControlKind(keyword);
	Win32DialogItemTemplate item;
	item.className = kind->className;
	item.style = kControlBaseStyle | kind->defaultStyle;
	if (kind->hasText)
	{
		parseItemText(item);
	}
	item.id = resolveId(next().text);

	if (kind->hasClassAndStyleFirst)
	{
		item.className = Win32UnescapeResourceString(next().text);
		item.style = parseStyleExpression(kControlBaseStyle);
	}

	item.x = nextNumber();
	item.y = nextNumber();
	const bool hasSize = keyword != "ICON" || nextStartsArgument();
	item.cx = hasSize ? nextNumber() : 0;
	item.cy = hasSize ? nextNumber() : 0;

	if (kind->hasClassAndStyleFirst)
	{
		item.exStyle = nextStartsArgument() ? parseStyleExpression(0) : 0;
		if (isExtended && nextStartsArgument())
		{
			nextNumber();
		}
	}
	else
	{
		item.style &= ~kind->typeMask;
		parseItemTail(item, isExtended);
		if ((item.style & kind->typeMask) == 0)
		{
			item.style |= kind->defaultStyle & kind->typeMask;
		}
	}
	dialog.items.push_back(item);
}

void Win32ResourceParser::parseDialogInitData(size_t byteCount, std::string& data)
{
	std::string bytes;
	while (!atEnd() && bytes.size() < byteCount)
	{
		const Win32ResourceToken& token = next();
		if (token.isQuoted)
		{
			bytes += Win32UnescapeResourceString(token.text);
			continue;
		}
		const UINT word = static_cast<UINT>(std::strtoul(token.text.c_str(), nullptr, 0));
		bytes += static_cast<char>(word & 0xFF);
		bytes += static_cast<char>((word >> 8) & 0xFF);
	}
	bytes.resize(byteCount);
	const size_t terminator = bytes.find('\0');
	data = terminator == std::string::npos ? bytes : bytes.substr(0, terminator);
}

void Win32ResourceParser::parseDialogInit(UINT id)
{
	skipUntilBegin();
	std::vector<Win32DialogInitEntry>& entries = m_resources.m_dialogInits[id];
	while (!atEnd() && !nextIs("END"))
	{
		Win32DialogInitEntry entry;
		entry.controlId = resolveId(next().text);
		if (entry.controlId == 0)
		{
			continue;
		}
		entry.message = static_cast<UINT>(nextNumber());
		const size_t byteCount = static_cast<size_t>(nextNumber());
		nextNumber();
		parseDialogInitData(byteCount, entry.text);
		entries.push_back(entry);
	}
	finishBlock();
}
