#pragma once

#include "Win32Control.h"

#include "imgui.h"

#include <map>
#include <memory>
#include <vector>

struct Win32TreeNode
{
	HTREEITEM handle = nullptr;
	Win32TreeNode* parent = nullptr;
	std::vector<Win32TreeNode*> children;
	std::string text;
	int image = 0;
	int selectedImage = 0;
	UINT state = 0;
	LPARAM lParam = 0;
	int childrenHint = 0;
	bool isLaidOut = false;
	RECT rowRect = {};
	RECT buttonRect = {};
	RECT stateRect = {};
	RECT iconRect = {};
	RECT textRect = {};

	bool isExpanded() const { return (state & TVIS_EXPANDED) != 0; }
	bool hasChildren() const { return !children.empty() || childrenHint > 0; }
};

struct Win32TreeRow
{
	Win32TreeNode* node;
	int depth;
};

class Win32TreeView : public Win32Control
{
public:
	explicit Win32TreeView(Win32Window& window);

	LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
	void draw(const Win32DrawContext& context) override;

	void endLabelEdit(bool accept);

private:
	Win32TreeNode* find(HTREEITEM handle);
	Win32TreeNode* parentFor(HTREEITEM handle);
	std::vector<Win32TreeRow> visibleRows();
	int rowIndexOf(const Win32TreeNode* node);
	int itemHeight() const;

	HTREEITEM insertItem(const TVINSERTSTRUCTA* insert);
	void placeNode(Win32TreeNode* parent, Win32TreeNode* node, HTREEITEM insertAfter);
	BOOL deleteItem(HTREEITEM handle);
	void deleteSubtree(Win32TreeNode* node);
	void moveSelectionAwayFrom(Win32TreeNode* node);
	BOOL getItem(TVITEMA* item);
	BOOL setItem(const TVITEMA* item);
	HTREEITEM nextItem(UINT relation, HTREEITEM handle);
	HTREEITEM visibleNeighbour(HTREEITEM handle, int delta);
	HTREEITEM visibleRelation(UINT relation, HTREEITEM handle);
	BOOL sortChildren(HTREEITEM handle, bool recurse, PFNTVCOMPARE compare, LPARAM sortParam);

	void fillNotifyItem(TVITEMA& item, const Win32TreeNode* node) const;
	LRESULT notifyTree(UINT code, UINT action, const Win32TreeNode* oldNode, const Win32TreeNode* newNode, POINT dragPoint);
	BOOL expandNode(Win32TreeNode* node, UINT code, bool isUserAction);
	BOOL selectNode(Win32TreeNode* node, UINT action);
	BOOL selectItem(UINT relation, HTREEITEM handle);
	void ensureVisible(Win32TreeNode* node);
	void setDropHighlight(Win32TreeNode* node);
	HTREEITEM hitTest(TVHITTESTINFO* info);
	BOOL itemRect(BOOL textOnly, RECT* rect);

	LRESULT handleItemMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);
	LRESULT handleViewMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled);
	LRESULT handleMouse(UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT handleLeftButtonUp(LPARAM lParam);
	LRESULT handleRightButtonUp(LPARAM lParam);
	LRESULT handleDoubleClick(LPARAM lParam);
	LRESULT handleKey(WPARAM key);
	LRESULT selectRelative(int delta);
	LRESULT collapseOrSelectParent();
	LRESULT expandOrSelectChild();
	LRESULT handleCharacter(char symbol);
	void toggleStateImage(Win32TreeNode* node);

	HWND beginLabelEdit(HTREEITEM handle);

	void layoutRow(const Win32DrawContext& context, Win32TreeNode* node, int depth, ImVec2 rowMin, float rowWidth);
	void drawConnectorLines(const Win32DrawContext& context, ImDrawList* drawList, const Win32TreeRow& row);
	void drawExpandButton(const Win32DrawContext& context, ImDrawList* drawList, const Win32TreeNode* node);
	void drawRow(const Win32DrawContext& context, ImDrawList* drawList, const Win32TreeRow& row);
	void applyScrollRequest(const std::vector<Win32TreeRow>& rows, float rowHeight);

	std::map<HTREEITEM, std::unique_ptr<Win32TreeNode>> m_nodes;
	Win32TreeNode m_root;
	uintptr_t m_nextHandle = 0x100010;
	Win32TreeNode* m_selected = nullptr;
	Win32TreeNode* m_dropHighlight = nullptr;
	HIMAGELIST m_normalImages = nullptr;
	HIMAGELIST m_stateImages = nullptr;
	int m_indent = 19;
	int m_customItemHeight = 0;
	COLORREF m_backgroundColor = CLR_DEFAULT;
	COLORREF m_textColor = CLR_DEFAULT;
	float m_scrollY = 0.0f;
	Win32TreeNode* m_scrollTarget = nullptr;
	bool m_scrollTargetToTop = false;
	Win32TreeNode* m_pressedNode = nullptr;
	POINT m_pressPoint = {};
	bool m_isLeftPressed = false;
	bool m_isRightPressed = false;
	bool m_wasSelectedOnPress = false;
	HWND m_labelEdit = nullptr;
	Win32TreeNode* m_labelEditNode = nullptr;
	bool m_isEndingLabelEdit = false;
};
