#pragma once

#include "Win32Control.h"

#include "imgui.h"

#include <vector>

struct Win32RichCharFormat
{
	DWORD effects = 0;
	COLORREF color = 0;
	bool usesAutoColor = true;

	bool operator==(const Win32RichCharFormat& other) const
	{
		return effects == other.effects && color == other.color && usesAutoColor == other.usesAutoColor;
	}
};

struct Win32RichGlyph
{
	ImVec2 min;
	ImVec2 max;
};

class Win32RichEdit : public Win32Control
{
public:
	explicit Win32RichEdit(Win32Window& window) : Win32Control(window) {}

	LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
	void draw(const Win32DrawContext& context) override;

private:
	int textLength() const { return static_cast<int>(m_window.text.size()); }
	bool isReadOnly() const { return (style() & ES_READONLY) != 0; }
	bool wants(DWORD eventFlag) const { return (m_eventMask & eventFlag) != 0; }

	void setText(const std::string& text);
	void setSelection(int start, int end);
	void replaceSelection(const std::string& replacement);
	LRESULT getSelection(DWORD* start, DWORD* end) const;
	LRESULT getSelectionRange(CHARRANGE* range) const;
	LRESULT copySelectedText(LPSTR buffer) const;
	LRESULT copyTextRange(TEXTRANGEA* range) const;
	void applyFormat(Win32RichCharFormat& format, const CHARFORMATA& source) const;
	LRESULT setCharFormat(WPARAM scope, const CHARFORMATA* format);
	LRESULT getCharFormat(WPARAM scope, CHARFORMATA* format) const;
	CHARRANGE linkRangeAt(int index) const;
	bool isLink(int index) const;

	LRESULT notifySelectionChange();
	LRESULT notifyLink(UINT message, WPARAM wParam, LPARAM lParam, CHARRANGE range);
	LRESULT notifyMessageFilter(UINT message, WPARAM wParam, LPARAM lParam);
	void notifyChange();

	int charFromPoint(LPARAM point) const;
	LRESULT handleMouse(UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT handleKey(WPARAM key);
	LRESULT handleCharacter(char symbol);
	LRESULT handleTextMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);
	LRESULT handleFormatMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);

	void layoutGlyphs(const Win32DrawContext& context);
	void drawGlyphs(const Win32DrawContext& context);

	std::vector<Win32RichCharFormat> m_formats;
	Win32RichCharFormat m_defaultFormat;
	std::vector<Win32RichGlyph> m_glyphs;
	int m_selectionStart = 0;
	int m_selectionEnd = 0;
	int m_anchor = 0;
	DWORD m_eventMask = ENM_NONE;
	COLORREF m_backgroundColor = CLR_DEFAULT;
	int m_limit = 32767;
	bool m_hidesSelection = false;
	bool m_isSelecting = false;
	POINT m_clientOrigin = {};
};
