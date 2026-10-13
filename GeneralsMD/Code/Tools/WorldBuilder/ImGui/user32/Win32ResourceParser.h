#pragma once

#include "Win32ResourceScript.h"
#include "Win32Resources.h"

class Win32ResourceParser
{
public:
	Win32ResourceParser(Win32Resources& resources, std::vector<Win32ResourceToken> tokens);

	void parse();

private:
	bool atEnd() const { return m_position >= m_tokens.size(); }
	const Win32ResourceToken& peek() const { return m_tokens[m_position]; }
	const Win32ResourceToken& next() { return m_tokens[m_position++]; }
	bool nextIs(const char* text) const;
	bool nextStartsArgument() const;

	UINT resolveId(const std::string& text) const;
	int nextNumber();
	DWORD parseStyleExpression(DWORD style);
	void skipUntilBegin();
	void skipBlock();
	void finishBlock();

	void parseResource(const std::string& name, const std::string& type);
	void parseFileResource(std::map<UINT, std::string>& files, UINT id);
	void parseStringTable();
	UINT parseMenuOptions();
	void parseMenu(UINT id);
	void parseMenuItems(std::vector<Win32MenuTemplateItem>& items);
	void parseMenuItem(std::vector<Win32MenuTemplateItem>& items);
	void parseAcceleratorKey(const Win32ResourceToken& token, Win32AcceleratorTemplate& accelerator) const;
	void parseAccelerators(UINT id);
	void parseToolbar(UINT id);

	void parseDialog(UINT id, bool isExtended);
	void parseDialogHeader(Win32DialogTemplate& dialog, bool isExtended);
	bool parseDialogStatement(Win32DialogTemplate& dialog, bool& hasStyle);
	void parseDialogItems(Win32DialogTemplate& dialog, bool isExtended);
	void parseDialogItem(Win32DialogTemplate& dialog, const std::string& keyword, bool isExtended);
	void parseItemText(Win32DialogItemTemplate& item);
	void parseItemTail(Win32DialogItemTemplate& item, bool isExtended);
	void parseDialogInit(UINT id);
	void parseDialogInitData(size_t byteCount, std::string& data);

	Win32Resources& m_resources;
	std::vector<Win32ResourceToken> m_tokens;
	size_t m_position = 0;
};
