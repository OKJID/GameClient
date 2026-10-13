#pragma once

#include "Win32Control.h"
#include "Win32ItemList.h"

#include "imgui.h"

class Win32ComboBox : public Win32Control
{
public:
	explicit Win32ComboBox(Win32Window& window) : Win32Control(window) {}

	LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
	void draw(const Win32DrawContext& context) override;

private:
	UINT comboType() const { return style() & 0x3; }
	bool hasEdit() const { return comboType() != CBS_DROPDOWNLIST; }
	bool isSimple() const { return comboType() == CBS_SIMPLE; }
	int fieldHeight() const;

	void initialize(const CREATESTRUCTA* create);
	LRESULT setSelection(int index);
	LRESULT setText(LPCSTR text);
	LRESULT getEditSelection(WPARAM wParam, LPARAM lParam) const;
	LRESULT droppedRect(RECT* rect) const;
	LRESULT deleteItem(int index);
	LRESULT resetContent();
	LRESULT setItemData(int index, DWORD_PTR data);
	LRESULT handleItemMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);
	LRESULT handleStateMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);
	LRESULT handleKey(WPARAM key);
	LRESULT handleCharacter(char symbol);
	LRESULT handleFieldClick(int x);
	LRESULT dialogCode() const;

	void openList();
	void closeList(bool accept);
	void chooseByUser(int index);
	void moveSelection(int delta);
	void applyUserEdit(const std::string& editText);

	void drawField(const Win32DrawContext& context);
	void drawEditField(const Win32DrawContext& context, float width);
	void drawList(const Win32DrawContext& context, bool isPopup);
	void drawItems(float rowHeight);
	void closeAbandonedPopup() const;

	Win32ItemList m_items;
	int m_selection = -1;
	int m_dropHeight = 0;
	int m_droppedWidth = 0;
	int m_limit = 0x7FFFFFFE;
	int m_editSelectionStart = 0;
	int m_editSelectionEnd = 0;
	std::string m_editorText;
	bool m_isDropped = false;
	bool m_wantsPopupOpen = false;
	bool m_isEditing = false;
	bool m_wantsEditorFocus = false;
	bool m_needsReload = false;
	ImGuiID m_editorId = 0;
};
