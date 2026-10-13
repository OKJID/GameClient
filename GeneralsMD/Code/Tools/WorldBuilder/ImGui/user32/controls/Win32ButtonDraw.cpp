#include "Win32Button.h"

#include "Win32ControlDrawing.h"
#include "Win32Gdi.h"

#include "imgui_internal.h"

#include <algorithm>

namespace
{

Win32ImagePtr imageOf(HANDLE handle, UINT type)
{
	if (type == IMAGE_BITMAP)
	{
		Win32GdiObject* object = Win32Gdi::object(handle);
		return object != nullptr && object->type == Win32GdiObjectType::Bitmap ? static_cast<Win32Bitmap*>(object)->image : nullptr;
	}
	Win32Icon* icon = Win32Gdi::icon(static_cast<HICON>(handle));
	return icon != nullptr ? icon->image : nullptr;
}

ImU32 textColor(const Win32DrawContext& context)
{
	return Win32ControlDrawing::color(context.isEnabled ? ImGuiCol_Text : ImGuiCol_TextDisabled);
}

float markSize()
{
	return ImGui::GetFontSize();
}

}

bool Win32Button::drawImage(const Win32DrawContext& context, ImVec2 min, ImVec2 max)
{
	Win32ImagePtr image = imageOf(m_image, m_imageType);
	const ImTextureID texture = image != nullptr ? image->textureForDrawing() : ImTextureID_Invalid;
	if (texture == ImTextureID_Invalid)
	{
		return false;
	}

	const ImVec2 size(static_cast<float>(image->width), static_cast<float>(image->height));
	const ImVec2 imageMin(min.x + (max.x - min.x - size.x) * 0.5f, min.y + (max.y - min.y - size.y) * 0.5f);
	const ImU32 tint = context.isEnabled ? IM_COL32_WHITE : IM_COL32(255, 255, 255, 110);
	context.drawList->AddImage(ImTextureRef(texture), imageMin, ImVec2(imageMin.x + size.x, imageMin.y + size.y), ImVec2(0, 0), ImVec2(1, 1), tint);
	return true;
}

void Win32Button::drawPushButton(const Win32DrawContext& context, bool isDown)
{
	const ImVec2 min = Win32ControlDrawing::screenPoint(context, 0, 0);
	const ImVec2 size = Win32ControlDrawing::clientSize(context);
	const ImVec2 max(min.x + size.x, min.y + size.y);
	const bool isHovered = context.isEnabled && ImGui::IsMouseHoveringRect(min, max);
	const ImGuiCol background = isDown ? ImGuiCol_ButtonActive : (isHovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	const float rounding = ImGui::GetStyle().FrameRounding;
	context.drawList->AddRectFilled(min, max, Win32ControlDrawing::color(background), rounding);
	const float borderWidth = buttonType() == BS_DEFPUSHBUTTON ? 2.0f : 1.0f;
	context.drawList->AddRect(min, max, Win32ControlDrawing::color(ImGuiCol_Border), rounding, borderWidth);

	if ((style() & (BS_BITMAP | BS_ICON)) != 0 && drawImage(context, min, max))
	{
		return;
	}
	const std::string label = Win32ControlDrawing::withoutMnemonic(text());
	const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
	const float offset = isDown ? 1.0f : 0.0f;
	const ImVec2 position(min.x + (size.x - textSize.x) * 0.5f + offset, min.y + (size.y - textSize.y) * 0.5f + offset);
	context.drawList->PushClipRect(min, max, true);
	context.drawList->AddText(position, textColor(context), label.c_str());
	context.drawList->PopClipRect();
}

void Win32Button::drawLabel(const Win32DrawContext& context, float left)
{
	const std::string label = Win32ControlDrawing::withoutMnemonic(text());
	const ImVec2 origin = Win32ControlDrawing::screenPoint(context, 0, 0);
	const ImVec2 size = Win32ControlDrawing::clientSize(context);
	const float y = origin.y + (size.y - ImGui::GetFontSize()) * 0.5f;
	context.drawList->PushClipRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), true);
	if ((style() & BS_LEFTTEXT) != 0)
	{
		context.drawList->AddText(ImVec2(origin.x, y), textColor(context), label.c_str());
	}
	else
	{
		context.drawList->AddText(ImVec2(left, y), textColor(context), label.c_str());
	}
	context.drawList->PopClipRect();
}

void Win32Button::drawCheckMark(const Win32DrawContext& context)
{
	const ImVec2 origin = Win32ControlDrawing::screenPoint(context, 0, 0);
	const ImVec2 size = Win32ControlDrawing::clientSize(context);
	const float box = markSize();
	const float left = (style() & BS_LEFTTEXT) != 0 ? origin.x + size.x - box : origin.x;
	const ImVec2 min(left, origin.y + (size.y - box) * 0.5f);
	const ImVec2 max(min.x + box, min.y + box);
	const ImGuiCol background = m_isPushed ? ImGuiCol_FrameBgActive : ImGuiCol_FrameBg;
	context.drawList->AddRectFilled(min, max, Win32ControlDrawing::color(background), ImGui::GetStyle().FrameRounding);
	context.drawList->AddRect(min, max, Win32ControlDrawing::color(ImGuiCol_Border), ImGui::GetStyle().FrameRounding);
	if (m_check == BST_CHECKED)
	{
		ImGui::RenderCheckMark(context.drawList, ImVec2(min.x + box * 0.15f, min.y + box * 0.15f), Win32ControlDrawing::color(ImGuiCol_CheckMark), box * 0.7f);
	}
	if (m_check == BST_INDETERMINATE)
	{
		context.drawList->AddRectFilled(ImVec2(min.x + box * 0.25f, min.y + box * 0.25f), ImVec2(max.x - box * 0.25f, max.y - box * 0.25f), Win32ControlDrawing::color(ImGuiCol_CheckMark));
	}
	drawLabel(context, max.x + ImGui::GetStyle().ItemInnerSpacing.x);
}

void Win32Button::drawRadioMark(const Win32DrawContext& context)
{
	const ImVec2 origin = Win32ControlDrawing::screenPoint(context, 0, 0);
	const ImVec2 size = Win32ControlDrawing::clientSize(context);
	const float diameter = markSize();
	const float left = (style() & BS_LEFTTEXT) != 0 ? origin.x + size.x - diameter : origin.x;
	const ImVec2 center(left + diameter * 0.5f, origin.y + size.y * 0.5f);
	const ImGuiCol background = m_isPushed ? ImGuiCol_FrameBgActive : ImGuiCol_FrameBg;
	context.drawList->AddCircleFilled(center, diameter * 0.5f, Win32ControlDrawing::color(background));
	context.drawList->AddCircle(center, diameter * 0.5f, Win32ControlDrawing::color(ImGuiCol_Border));
	if (m_check == BST_CHECKED)
	{
		context.drawList->AddCircleFilled(center, diameter * 0.25f, Win32ControlDrawing::color(ImGuiCol_CheckMark));
	}
	drawLabel(context, left + diameter + ImGui::GetStyle().ItemInnerSpacing.x);
}

void Win32Button::drawGroupBox(const Win32DrawContext& context)
{
	const std::string label = Win32ControlDrawing::withoutMnemonic(text());
	const float fontSize = ImGui::GetFontSize();
	const ImVec2 min = Win32ControlDrawing::screenPoint(context, 0, 0);
	const ImVec2 size = Win32ControlDrawing::clientSize(context);
	const ImVec2 frameMin(min.x, min.y + fontSize * 0.5f);
	const ImVec2 frameMax(min.x + size.x - 1.0f, min.y + size.y - 1.0f);
	const ImU32 border = Win32ControlDrawing::color(ImGuiCol_Border);
	const float textWidth = label.empty() ? 0.0f : ImGui::CalcTextSize(label.c_str()).x + 4.0f;
	const float textStart = frameMin.x + 6.0f;

	context.drawList->AddLine(frameMin, ImVec2(textStart, frameMin.y), border);
	context.drawList->AddLine(ImVec2(textStart + textWidth, frameMin.y), ImVec2(frameMax.x, frameMin.y), border);
	context.drawList->AddLine(ImVec2(frameMax.x, frameMin.y), frameMax, border);
	context.drawList->AddLine(frameMax, ImVec2(frameMin.x, frameMax.y), border);
	context.drawList->AddLine(ImVec2(frameMin.x, frameMax.y), frameMin, border);
	context.drawList->AddText(ImVec2(textStart + 2.0f, min.y), textColor(context), label.c_str());
}

void Win32Button::drawFocus(const Win32DrawContext& context)
{
	if (!context.hasFocus || buttonType() == BS_GROUPBOX)
	{
		return;
	}
	const ImVec2 min = Win32ControlDrawing::screenPoint(context, 0, 0);
	const ImVec2 size = Win32ControlDrawing::clientSize(context);
	context.drawList->AddRect(ImVec2(min.x + 1.0f, min.y + 1.0f), ImVec2(min.x + size.x - 1.0f, min.y + size.y - 1.0f), Win32ControlDrawing::color(ImGuiCol_NavCursor));
}

void Win32Button::draw(const Win32DrawContext& context)
{
	if (buttonType() == BS_GROUPBOX)
	{
		drawGroupBox(context);
		return;
	}

	if (isPushLike())
	{
		drawPushButton(context, m_isPushed || m_check != BST_UNCHECKED);
	}
	else if (isCheckBox())
	{
		drawCheckMark(context);
	}
	else if (isRadioButton())
	{
		drawRadioMark(context);
	}
	else
	{
		drawPushButton(context, m_isPushed);
	}
	drawFocus(context);
}
