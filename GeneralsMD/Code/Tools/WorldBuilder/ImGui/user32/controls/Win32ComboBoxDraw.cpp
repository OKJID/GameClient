#include "Win32ComboBox.h"

#include "Win32ControlDrawing.h"
#include "Win32Metrics.h"

#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include <algorithm>

namespace
{

const char kListPopupId[] = "##comboList";

}

void Win32ComboBox::drawEditField(const Win32DrawContext&, float width)
{
	if (m_wantsEditorFocus)
	{
		ImGui::SetKeyboardFocusHere();
		m_wantsEditorFocus = false;
	}
	if (m_needsReload)
	{
		m_needsReload = false;
		ImGuiInputTextState* state = ImGui::GetInputTextState(m_editorId);
		if (state != nullptr)
		{
			state->ReloadUserBufAndKeepSelection();
		}
	}

	ImGui::SetNextItemWidth(width);
	const bool isEdited = ImGui::InputText("##comboEdit", &m_editorText);
	m_editorId = ImGui::GetItemID();
	m_isEditing = ImGui::IsItemActive();
	if (ImGui::IsItemActivated())
	{
		queueInput([this]() { takeFocus(); });
	}
	if (isEdited)
	{
		const std::string snapshot = m_editorText.substr(0, static_cast<size_t>(m_limit));
		queueInput([this, snapshot]() { applyUserEdit(snapshot); });
	}
}

void Win32ComboBox::drawField(const Win32DrawContext& context)
{
	const float width = static_cast<float>(context.clientSize.cx);
	const float height = static_cast<float>(fieldHeight());
	const float arrowWidth = height;
	Win32ControlDrawing::placeCursor(context);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, std::max(0.0f, (height - ImGui::GetFontSize()) * 0.5f)));
	if (hasEdit())
	{
		drawEditField(context, isSimple() ? width : width - arrowWidth);
	}
	if (isSimple())
	{
		ImGui::PopStyleVar();
		return;
	}

	if (hasEdit())
	{
		ImGui::SameLine(0.0f, 0.0f);
	}
	const ImVec2 min = hasEdit() ? Win32ControlDrawing::screenPoint(context, static_cast<int>(width - arrowWidth), 0) : Win32ControlDrawing::screenPoint(context, 0, 0);
	const ImVec2 max = Win32ControlDrawing::screenPoint(context, static_cast<int>(width), static_cast<int>(height));
	const ImGuiCol background = ImGui::IsMouseHoveringRect(min, max) ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg;
	context.drawList->AddRectFilled(min, max, Win32ControlDrawing::color(background));
	context.drawList->AddRect(min, max, Win32ControlDrawing::color(ImGuiCol_Border));
	if (!hasEdit())
	{
		const float textY = min.y + (height - ImGui::GetFontSize()) * 0.5f;
		context.drawList->PushClipRect(min, ImVec2(max.x - arrowWidth, max.y), true);
		context.drawList->AddText(ImVec2(min.x + ImGui::GetStyle().FramePadding.x, textY), Win32ControlDrawing::color(ImGuiCol_Text), m_window.text.c_str());
		context.drawList->PopClipRect();
	}
	ImGui::RenderArrow(context.drawList, ImVec2(max.x - arrowWidth + (arrowWidth - ImGui::GetFontSize()) * 0.5f, min.y + (height - ImGui::GetFontSize()) * 0.5f),
		Win32ControlDrawing::color(ImGuiCol_Text), ImGuiDir_Down);
	ImGui::PopStyleVar();
}

void Win32ComboBox::drawItems(float rowHeight)
{
	for (int index = 0; index < m_items.count(); ++index)
	{
		ImGui::PushID(index);
		const bool isSelected = index == m_selection;
		if (ImGui::Selectable(m_items.at(index).text.c_str(), isSelected, ImGuiSelectableFlags_None, ImVec2(0.0f, rowHeight - ImGui::GetStyle().ItemSpacing.y)))
		{
			queueInput([this, index]() { chooseByUser(index); });
			if (!isSimple())
			{
				ImGui::CloseCurrentPopup();
			}
		}
		if (isSelected && ImGui::IsWindowAppearing())
		{
			ImGui::SetScrollHereY();
		}
		ImGui::PopID();
	}
}

void Win32ComboBox::drawList(const Win32DrawContext& context, bool isPopup)
{
	const float rowHeight = static_cast<float>(Win32Metrics::lineHeight());
	const float width = static_cast<float>(std::max(m_droppedWidth, context.clientSize.cx));
	if (!isPopup)
	{
		const float listTop = static_cast<float>(fieldHeight());
		ImGui::SetCursorScreenPos(Win32ControlDrawing::screenPoint(context, 0, fieldHeight()));
		ImGui::BeginChild("##comboSimpleList", ImVec2(width, std::max(rowHeight, context.clientSize.cy - listTop)), ImGuiChildFlags_Borders);
		drawItems(rowHeight);
		ImGui::EndChild();
		return;
	}

	const float itemsHeight = rowHeight * static_cast<float>(std::max(1, m_items.count())) + ImGui::GetStyle().WindowPadding.y * 2.0f;
	const float availableHeight = static_cast<float>(std::max(m_dropHeight - fieldHeight(), Win32Metrics::lineHeight()));
	ImGui::SetNextWindowPos(Win32ControlDrawing::screenPoint(context, 0, fieldHeight()));
	ImGui::SetNextWindowSize(ImVec2(width, std::min(itemsHeight, std::max(availableHeight, rowHeight * 3.0f))));
	if (!ImGui::BeginPopup(kListPopupId))
	{
		queueInput([this]() { closeList(false); });
		return;
	}
	drawItems(rowHeight);
	ImGui::EndPopup();
}

void Win32ComboBox::draw(const Win32DrawContext& context)
{
	drawField(context);
	if (isSimple())
	{
		drawList(context, false);
		return;
	}
	if (m_wantsPopupOpen)
	{
		ImGui::OpenPopup(kListPopupId);
		m_wantsPopupOpen = false;
	}
	if (m_isDropped)
	{
		drawList(context, true);
		return;
	}
	closeAbandonedPopup();
}

void Win32ComboBox::closeAbandonedPopup() const
{
	const ImGuiID popupId = ImGui::GetID(kListPopupId);
	const ImGuiContext& imguiContext = *ImGui::GetCurrentContext();
	for (int level = 0; level < imguiContext.OpenPopupStack.Size; ++level)
	{
		if (imguiContext.OpenPopupStack[level].PopupId == popupId)
		{
			ImGui::ClosePopupToLevel(level, false);
			return;
		}
	}
}
