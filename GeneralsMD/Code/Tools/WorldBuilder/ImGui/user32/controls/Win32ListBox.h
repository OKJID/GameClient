#pragma once

#include "Win32Control.h"
#include "Win32ItemList.h"

class Win32ListBox : public Win32Control
{
public:
	explicit Win32ListBox(Win32Window& window) : Win32Control(window) {}

	LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
	void draw(const Win32DrawContext& context) override;

private:
	bool isMultiSelect() const { return (style() & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) != 0; }
	bool notifies() const { return (style() & LBS_NOTIFY) != 0; }
	int itemHeight() const;

	LRESULT setCurrentSelection(int index);
	LRESULT setSelected(bool isSelected, int index);
	LRESULT selectRange(bool isSelected, int first, int last);
	int selectedCount() const;
	LRESULT copySelectedIndexes(int capacity, int* indexes) const;
	LRESULT itemRect(int index, RECT* rect) const;
	LRESULT itemFromPoint(LPARAM point) const;
	LRESULT deleteItem(int index);
	LRESULT resetContent();
	LRESULT setItemData(int index, DWORD_PTR data);
	LRESULT handleKey(WPARAM key);
	LRESULT handleItemMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);
	LRESULT handleSelectionMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);

	void selectByUser(int index, bool toggle, bool extend);
	void moveCaret(int delta);
	void typeAhead(char symbol);
	void notifySelectionChange();
	int indexAtClientY(int y) const;
	void drawRows(const Win32DrawContext& context, float rowHeight);

	Win32ItemList m_items;
	int m_caret = -1;
	int m_anchor = -1;
	int m_topIndex = 0;
	int m_customItemHeight = 0;
	int m_horizontalExtent = 0;
	float m_scrollY = 0.0f;
	float m_contentTop = 0.0f;
	bool m_wantsScroll = false;
};
