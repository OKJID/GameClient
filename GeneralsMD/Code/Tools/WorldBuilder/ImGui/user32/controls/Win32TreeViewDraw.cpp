#include "Win32TreeView.h"

#include "Win32ControlDrawing.h"
#include "Win32Gdi.h"
#include "Win32ImageList.h"

#include <algorithm>

namespace
{

const float kButtonSize = 9.0f;

RECT toClientRect(const Win32DrawContext& context, ImVec2 min, ImVec2 max)
{
	RECT rect;
	rect.left = static_cast<LONG>(min.x) - context.clientOrigin.x;
	rect.top = static_cast<LONG>(min.y) - context.clientOrigin.y;
	rect.right = static_cast<LONG>(max.x) - context.clientOrigin.x;
	rect.bottom = static_cast<LONG>(max.y) - context.clientOrigin.y;
	return rect;
}

ImVec2 toScreen(const Win32DrawContext& context, int x, int y)
{
	return ImVec2(static_cast<float>(context.clientOrigin.x + x), static_cast<float>(context.clientOrigin.y + y));
}

void drawImage(ImDrawList* drawList, HIMAGELIST imageList, int index, ImVec2 min)
{
	Win32ImagePtr image = Win32ImageLists::image(imageList, index);
	const ImTextureID texture = image != nullptr ? image->textureForDrawing() : ImTextureID_Invalid;
	if (texture == ImTextureID_Invalid)
	{
		return;
	}
	drawList->AddImage(ImTextureRef(texture), min, ImVec2(min.x + image->width, min.y + image->height));
}

int imageWidth(HIMAGELIST imageList)
{
	int width = 0;
	return imageList != nullptr && ImageList_GetIconSize(imageList, &width, nullptr) ? width : 0;
}

bool hasNextSibling(const Win32TreeNode* node)
{
	const std::vector<Win32TreeNode*>& siblings = node->parent->children;
	return !siblings.empty() && siblings.back() != node;
}

}

void Win32TreeView::layoutRow(const Win32DrawContext& context, Win32TreeNode* node, int depth, ImVec2 rowMin, float rowWidth)
{
	const float rowHeight = static_cast<float>(itemHeight());
	const int level = depth + ((style() & TVS_LINESATROOT) != 0 ? 1 : 0);
	const float indent = static_cast<float>(m_indent);
	const bool hasButton = (style() & TVS_HASBUTTONS) != 0 && node->hasChildren() && level > 0;
	const float buttonLeft = rowMin.x + (level - 1) * indent + (indent - kButtonSize) * 0.5f;
	const float buttonTop = rowMin.y + (rowHeight - kButtonSize) * 0.5f;
	float x = rowMin.x + level * indent;

	node->isLaidOut = true;
	node->rowRect = toClientRect(context, rowMin, ImVec2(rowMin.x + rowWidth, rowMin.y + rowHeight));
	node->buttonRect = hasButton ? toClientRect(context, ImVec2(buttonLeft, buttonTop), ImVec2(buttonLeft + kButtonSize, buttonTop + kButtonSize)) : RECT{};

	const UINT stateIndex = (node->state & TVIS_STATEIMAGEMASK) >> 12;
	const int stateWidth = m_stateImages != nullptr && stateIndex > 0 ? imageWidth(m_stateImages) : 0;
	node->stateRect = toClientRect(context, ImVec2(x, rowMin.y), ImVec2(x + stateWidth, rowMin.y + rowHeight));
	x += static_cast<float>(stateWidth);

	const int iconWidth = imageWidth(m_normalImages);
	node->iconRect = toClientRect(context, ImVec2(x, rowMin.y), ImVec2(x + iconWidth, rowMin.y + rowHeight));
	x += static_cast<float>(iconWidth) + (iconWidth > 0 ? 2.0f : 0.0f);

	const float textWidth = ImGui::CalcTextSize(node->text.c_str()).x + 4.0f;
	node->textRect = toClientRect(context, ImVec2(x, rowMin.y), ImVec2(x + textWidth, rowMin.y + rowHeight));
}

void Win32TreeView::drawConnectorLines(const Win32DrawContext& context, ImDrawList* drawList, const Win32TreeRow& row)
{
	const Win32TreeNode* node = row.node;
	const int level = row.depth + ((style() & TVS_LINESATROOT) != 0 ? 1 : 0);
	if ((style() & TVS_HASLINES) == 0 || level <= 0)
	{
		return;
	}

	const float rowHeight = static_cast<float>(itemHeight());
	const ImVec2 rowMin = toScreen(context, node->rowRect.left, node->rowRect.top);
	const float midY = rowMin.y + rowHeight * 0.5f;
	const float indent = static_cast<float>(m_indent);
	const ImU32 lineColor = Win32ControlDrawing::color(ImGuiCol_Border);
	const float lineX = rowMin.x + (level - 1) * indent + indent * 0.5f;
	drawList->AddLine(ImVec2(lineX, midY), ImVec2(rowMin.x + level * indent, midY), lineColor);
	drawList->AddLine(ImVec2(lineX, rowMin.y), ImVec2(lineX, hasNextSibling(node) ? rowMin.y + rowHeight : midY), lineColor);

	int ancestorLevel = level - 1;
	for (const Win32TreeNode* ancestor = node->parent; ancestor != &m_root && ancestorLevel > 0; ancestor = ancestor->parent, --ancestorLevel)
	{
		if (hasNextSibling(ancestor))
		{
			const float ancestorX = rowMin.x + (ancestorLevel - 1) * indent + indent * 0.5f;
			drawList->AddLine(ImVec2(ancestorX, rowMin.y), ImVec2(ancestorX, rowMin.y + rowHeight), lineColor);
		}
	}
}

void Win32TreeView::drawExpandButton(const Win32DrawContext& context, ImDrawList* drawList, const Win32TreeNode* node)
{
	if (node->buttonRect.right <= node->buttonRect.left)
	{
		return;
	}

	const ImVec2 buttonMin = toScreen(context, node->buttonRect.left, node->buttonRect.top);
	const ImVec2 buttonMax(buttonMin.x + kButtonSize, buttonMin.y + kButtonSize);
	const ImVec2 center((buttonMin.x + buttonMax.x) * 0.5f, (buttonMin.y + buttonMax.y) * 0.5f);
	const ImU32 markColor = Win32ControlDrawing::color(ImGuiCol_Text);
	drawList->AddRectFilled(buttonMin, buttonMax, Win32ControlDrawing::color(ImGuiCol_FrameBg));
	drawList->AddRect(buttonMin, buttonMax, Win32ControlDrawing::color(ImGuiCol_Border));
	drawList->AddLine(ImVec2(buttonMin.x + 2.0f, center.y), ImVec2(buttonMax.x - 2.0f, center.y), markColor);
	if (!node->isExpanded())
	{
		drawList->AddLine(ImVec2(center.x, buttonMin.y + 2.0f), ImVec2(center.x, buttonMax.y - 2.0f), markColor);
	}
}

void Win32TreeView::drawRow(const Win32DrawContext& context, ImDrawList* drawList, const Win32TreeRow& row)
{
	const Win32TreeNode* node = row.node;
	const float rowHeight = static_cast<float>(itemHeight());
	drawConnectorLines(context, drawList, row);
	drawExpandButton(context, drawList, node);

	const UINT stateIndex = (node->state & TVIS_STATEIMAGEMASK) >> 12;
	if (stateIndex > 0)
	{
		drawImage(drawList, m_stateImages, static_cast<int>(stateIndex), toScreen(context, node->stateRect.left, node->stateRect.top + 1));
	}
	const bool isSelected = (node->state & TVIS_SELECTED) != 0;
	drawImage(drawList, m_normalImages, isSelected ? node->selectedImage : node->image, toScreen(context, node->iconRect.left, node->iconRect.top + 1));

	const ImVec2 textMin = toScreen(context, node->textRect.left, node->textRect.top);
	const ImVec2 textMax = toScreen(context, node->textRect.right, node->textRect.bottom);
	const bool showsSelection = isSelected && (context.hasFocus || (style() & TVS_SHOWSELALWAYS) != 0);
	if (showsSelection || (node->state & TVIS_DROPHILITED) != 0)
	{
		drawList->AddRectFilled(textMin, textMax, Win32ControlDrawing::color((node->state & TVIS_DROPHILITED) != 0 ? ImGuiCol_HeaderHovered : ImGuiCol_Header));
	}
	const ImU32 textColor = m_textColor != CLR_DEFAULT ? Win32Gdi::toImColor(m_textColor) : Win32ControlDrawing::color(context.isEnabled ? ImGuiCol_Text : ImGuiCol_TextDisabled);
	drawList->AddText(ImVec2(textMin.x + 2.0f, textMin.y + (rowHeight - ImGui::GetFontSize()) * 0.5f), textColor, node->text.c_str());
}

void Win32TreeView::applyScrollRequest(const std::vector<Win32TreeRow>& rows, float rowHeight)
{
	if (m_scrollTarget == nullptr)
	{
		return;
	}
	for (size_t index = 0; index < rows.size(); ++index)
	{
		if (rows[index].node != m_scrollTarget)
		{
			continue;
		}
		const float top = static_cast<float>(index) * rowHeight;
		const float visibleHeight = ImGui::GetWindowHeight() - rowHeight;
		if (m_scrollTargetToTop || top < ImGui::GetScrollY())
		{
			ImGui::SetScrollY(top);
		}
		else if (top > ImGui::GetScrollY() + visibleHeight)
		{
			ImGui::SetScrollY(top - visibleHeight);
		}
		break;
	}
	m_scrollTarget = nullptr;
	m_scrollTargetToTop = false;
}

void Win32TreeView::draw(const Win32DrawContext& context)
{
	Win32ControlDrawing::placeCursor(context);
	const bool hasBorder = (style() & WS_BORDER) != 0 || (m_window.exStyle & WS_EX_CLIENTEDGE) != 0;
	const ImU32 background = m_backgroundColor != CLR_DEFAULT ? Win32Gdi::toImColor(m_backgroundColor) : Win32ControlDrawing::color(ImGuiCol_FrameBg);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, background);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1.0f, 1.0f));
	ImGui::BeginChild("##tree", Win32ControlDrawing::clientSize(context), hasBorder ? ImGuiChildFlags_Borders : ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

	const std::vector<Win32TreeRow> rows = visibleRows();
	const float rowHeight = static_cast<float>(itemHeight());
	applyScrollRequest(rows, rowHeight);
	m_scrollY = ImGui::GetScrollY();
	for (auto& entry : m_nodes)
	{
		entry.second->isLaidOut = false;
	}

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const float rowWidth = std::max(ImGui::GetContentRegionAvail().x, 1.0f);
	float contentWidth = rowWidth;
	for (size_t index = 0; index < rows.size(); ++index)
	{
		layoutRow(context, rows[index].node, rows[index].depth, ImVec2(origin.x, origin.y + static_cast<float>(index) * rowHeight), rowWidth);
		contentWidth = std::max(contentWidth, static_cast<float>(rows[index].node->textRect.right + context.clientOrigin.x) - origin.x);
	}
	const size_t first = static_cast<size_t>(std::max(0.0f, m_scrollY / rowHeight));
	const size_t last = std::min(rows.size(), first + static_cast<size_t>(ImGui::GetWindowHeight() / rowHeight) + 2);
	for (size_t index = first; index < last; ++index)
	{
		drawRow(context, drawList, rows[index]);
	}

	ImGui::Dummy(ImVec2(contentWidth, rowHeight * static_cast<float>(rows.size())));
	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
}
