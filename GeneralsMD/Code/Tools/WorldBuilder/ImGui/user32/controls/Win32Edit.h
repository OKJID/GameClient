#pragma once

#include "Win32Control.h"

#include "imgui.h"

class Win32Edit : public Win32Control
{
public:
	explicit Win32Edit(Win32Window& window) : Win32Control(window) {}

	LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
	void draw(const Win32DrawContext& context) override;

	static int filterCharacter(ImGuiInputTextCallbackData* data);
	static int trackInput(ImGuiInputTextCallbackData* data);

private:
	bool isMultiline() const { return (style() & ES_MULTILINE) != 0; }
	bool isReadOnly() const { return (style() & ES_READONLY) != 0; }

	std::string toEditorText(const std::string& text) const;
	std::string fromEditorText(const std::string& text) const;
	int editorToTextIndex(int index) const;
	int textToEditorIndex(int index) const;

	void setText(const std::string& text, bool notify);
	void replaceSelection(const std::string& replacement, bool notify);
	void setSelection(int start, int end);
	void notifyChange();
	void applyUserEdit(const std::string& editorText);
	void insertCharacter(char symbol);
	void deleteForward();
	void copySelection(bool remove);
	void paste();
	void undo();

	LRESULT getSelection(WPARAM wParam, LPARAM lParam) const;
	LRESULT lineCount() const;
	LRESULT lineIndex(int line) const;
	LRESULT lineLength(int index) const;
	LRESULT getLine(int line, LPSTR buffer) const;
	LRESULT lineFromChar(int index) const;
	LRESULT handleEditMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);
	LRESULT handleClipboardMessage(UINT message, bool& handled);
	LRESULT dialogCode() const;

	ImGuiInputTextFlags inputFlags() const;
	void synchronizeEditorState();

	std::string m_editorText;
	std::string m_undoText;
	int m_selectionStart = 0;
	int m_selectionEnd = 0;
	int m_limit = 0x7FFFFFFE;
	bool m_isModified = false;
	bool m_isEditing = false;
	bool m_needsReload = false;
	bool m_hasPendingSelection = false;
	bool m_wantsEditorFocus = false;
	bool m_wantsEditorBlur = false;
	bool m_wantsCaretScroll = false;
	ImGuiID m_editorId = 0;
};
