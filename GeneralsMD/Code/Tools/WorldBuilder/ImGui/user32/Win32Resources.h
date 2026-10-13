#pragma once

#include "Win32Api.h"
#include "Win32Paths.h"

#include <map>
#include <string>
#include <vector>

struct Win32MenuTemplateItem
{
	std::string text;
	UINT id = 0;
	bool isPopup = false;
	bool isSeparator = false;
	UINT flags = 0;
	std::vector<Win32MenuTemplateItem> children;
};

struct Win32AcceleratorTemplate
{
	UINT virtualKey = 0;
	bool isVirtualKey = false;
	bool control = false;
	bool shift = false;
	bool alt = false;
	UINT id = 0;
};

struct Win32ToolbarTemplate
{
	int buttonWidth = 0;
	int buttonHeight = 0;
	std::vector<UINT> buttons;
};

struct Win32DialogItemTemplate
{
	std::string className;
	std::string text;
	UINT textOrdinal = 0;
	UINT id = 0;
	DWORD style = 0;
	DWORD exStyle = 0;
	int x = 0;
	int y = 0;
	int cx = 0;
	int cy = 0;
};

struct Win32DialogTemplate
{
	DWORD style = 0;
	DWORD exStyle = 0;
	int x = 0;
	int y = 0;
	int cx = 0;
	int cy = 0;
	std::string caption;
	std::string menuName;
	std::string className;
	std::string fontName;
	int fontSize = 8;
	std::vector<Win32DialogItemTemplate> items;
};

struct Win32DialogInitEntry
{
	UINT controlId = 0;
	UINT message = 0;
	std::string text;
};

class Win32Resources
{
public:
	static Win32Resources& instance();

	void registerIds(const struct Win32NamedValue* values, size_t count);
	void registerString(UINT id, const char* text);
	bool load(const std::string& directory);

	const std::string& directory() const { return m_directory; }
	std::string filePath(const std::string& fileName) const { return Win32JoinPath(m_directory, fileName); }

	bool resolveName(LPCSTR name, UINT& id) const;
	bool findId(const std::string& name, UINT& id) const;

	const char* string(UINT id) const;
	const std::string* cursorFile(UINT id) const;
	const std::string* bitmapFile(UINT id) const;
	const std::string* iconFile(UINT id) const;
	const Win32MenuTemplateItem* menu(UINT id) const;
	const std::vector<Win32AcceleratorTemplate>* accelerators(UINT id) const;
	const Win32ToolbarTemplate* toolbar(UINT id) const;
	const Win32DialogTemplate* dialog(UINT id) const;
	const std::vector<Win32DialogInitEntry>* dialogInit(UINT id) const;

private:
	friend class Win32ResourceParser;

	std::string m_directory;
	std::map<std::string, UINT> m_ids;
	std::map<UINT, std::string> m_strings;
	std::map<UINT, std::string> m_cursors;
	std::map<UINT, std::string> m_bitmaps;
	std::map<UINT, std::string> m_icons;
	std::map<UINT, Win32MenuTemplateItem> m_menus;
	std::map<UINT, std::vector<Win32AcceleratorTemplate>> m_accelerators;
	std::map<UINT, Win32ToolbarTemplate> m_toolbars;
	std::map<UINT, Win32DialogTemplate> m_dialogs;
	std::map<UINT, std::vector<Win32DialogInitEntry>> m_dialogInits;
};

struct Win32NamedValue
{
	const char* name;
	UINT value;
};

#define WIN32_NAMED_VALUE(name) { #name, static_cast<UINT>(name) }
