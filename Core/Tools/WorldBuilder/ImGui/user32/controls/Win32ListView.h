#pragma once

#include "Win32Control.h"

#include "imgui.h"

#include <string>
#include <vector>

struct Win32ListViewColumn
{
	std::string text;
	int width = 50;
	int format = LVCFMT_LEFT;
};

struct Win32ListViewItem
{
	std::vector<std::string> texts;
	LPARAM lParam = 0;
	UINT state = 0;
	int image = 0;
};

class Win32ListView : public Win32Control
{
public:
	explicit Win32ListView(Win32Window& window) : Win32Control(window) {}

	LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
	void draw(const Win32DrawContext& context) override;

private:
	bool isValid(int index) const { return index >= 0 && index < static_cast<int>(m_items.size()); }
	int rowHeight() const;
	int headerHeight() const;

	int insertItem(const LVITEMA* item);
	BOOL deleteItem(int index);
	BOOL deleteAllItems();
	BOOL getItem(LVITEMA* item) const;
	BOOL setItem(const LVITEMA* item);
	int getItemText(int index, LVITEMA* item) const;
	BOOL setItemText(int index, const LVITEMA* item);
	int insertColumn(int index, const LVCOLUMNA* column);
	int nextItem(int start, UINT flags) const;
	BOOL setItemState(int index, UINT state, UINT mask);
	int findItem(int start, const LVFINDINFOA* info) const;
	BOOL sortItems(PFNLVCOMPARE compare, LPARAM sortParam);
	int hitTest(LVHITTESTINFO* info, bool withSubItem) const;
	BOOL itemRect(int index, RECT* rect) const;
	int selectedCount() const;

	LRESULT notifyItemChange(UINT code, int index, UINT newState, UINT oldState);
	LRESULT notifyActivation(UINT code, int index, POINT point);
	void selectByUser(int index, WPARAM keys);
	LRESULT handleMouse(UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT handleKey(WPARAM key);
	LRESULT handleItemMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);
	LRESULT handleViewMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);

	void drawHeader(ImDrawList* drawList, ImVec2 origin);
	void drawRows(const Win32DrawContext& context, ImDrawList* drawList, ImVec2 origin);

	std::vector<Win32ListViewColumn> m_columns;
	std::vector<Win32ListViewItem> m_items;
	DWORD m_extendedStyle = 0;
	HIMAGELIST m_imageLists[3] = {};
	int m_focusedItem = -1;
	int m_anchorItem = -1;
	float m_scrollY = 0.0f;
	float m_scrollX = 0.0f;
	int m_scrollTarget = -1;
	int m_pressedItem = -1;
};
