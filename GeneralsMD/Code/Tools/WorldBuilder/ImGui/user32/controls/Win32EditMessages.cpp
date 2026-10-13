#include "Win32Edit.h"

#include "Win32ControlDrawing.h"

#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace
{

int countNewlines(const char* text, int length)
{
	return static_cast<int>(std::count(text, text + std::max(0, length), '\n'));
}

struct LineSpan
{
	int start;
	int end;
};

std::vector<LineSpan> spansOf(const std::string& text)
{
	std::vector<LineSpan> spans;
	int start = 0;
	for (int index = 0; index <= static_cast<int>(text.size()); ++index)
	{
		const bool isEnd = index == static_cast<int>(text.size());
		if (!isEnd && text[static_cast<size_t>(index)] != '\n')
		{
			continue;
		}
		const int end = index > start && text[static_cast<size_t>(index - 1)] == '\r' ? index - 1 : index;
		spans.push_back({ start, end });
		start = index + 1;
	}
	return spans;
}

}

int Win32Edit::filterCharacter(ImGuiInputTextCallbackData* data)
{
	const Win32Edit* edit = static_cast<const Win32Edit*>(data->UserData);
	if ((edit->style() & ES_NUMBER) != 0 && (data->EventChar < '0' || data->EventChar > '9'))
	{
		return 1;
	}
	if ((edit->style() & ES_LOWERCASE) != 0 && data->EventChar < 0x80)
	{
		data->EventChar = static_cast<ImWchar>(std::tolower(static_cast<int>(data->EventChar)));
	}
	return 0;
}

int Win32Edit::trackInput(ImGuiInputTextCallbackData* data)
{
	Win32Edit* edit = static_cast<Win32Edit*>(data->UserData);
	if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter)
	{
		return filterCharacter(data);
	}
	if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit && data->BufTextLen > edit->m_limit)
	{
		data->DeleteChars(edit->m_limit, data->BufTextLen - edit->m_limit);
		return 0;
	}
	if (data->EventFlag != ImGuiInputTextFlags_CallbackAlways)
	{
		return 0;
	}

	const bool isMultiline = edit->isMultiline();
	if (edit->m_hasPendingSelection)
	{
		data->SelectionStart = edit->textToEditorIndex(edit->m_selectionStart);
		data->SelectionEnd = edit->textToEditorIndex(edit->m_selectionEnd);
		data->CursorPos = data->SelectionEnd;
		edit->m_hasPendingSelection = false;
		return 0;
	}

	const int start = std::min(data->SelectionStart, data->SelectionEnd);
	const int end = std::max(data->SelectionStart, data->SelectionEnd);
	edit->m_selectionStart = isMultiline ? start + countNewlines(data->Buf, start) : start;
	edit->m_selectionEnd = isMultiline ? end + countNewlines(data->Buf, end) : end;
	return 0;
}

LRESULT Win32Edit::getSelection(WPARAM wParam, LPARAM lParam) const
{
	DWORD* start = reinterpret_cast<DWORD*>(wParam);
	DWORD* end = reinterpret_cast<DWORD*>(lParam);
	if (start != nullptr)
	{
		*start = static_cast<DWORD>(m_selectionStart);
	}
	if (end != nullptr)
	{
		*end = static_cast<DWORD>(m_selectionEnd);
	}
	return MAKELRESULT(std::min(m_selectionStart, 0xFFFF), std::min(m_selectionEnd, 0xFFFF));
}

LRESULT Win32Edit::lineCount() const
{
	return static_cast<LRESULT>(spansOf(text()).size());
}

LRESULT Win32Edit::lineIndex(int line) const
{
	const std::vector<LineSpan> spans = spansOf(text());
	const int target = line < 0 ? static_cast<int>(lineFromChar(-1)) : line;
	return target < static_cast<int>(spans.size()) ? spans[static_cast<size_t>(target)].start : -1;
}

LRESULT Win32Edit::lineFromChar(int index) const
{
	const int position = index < 0 ? m_selectionStart : index;
	const std::vector<LineSpan> spans = spansOf(text());
	for (size_t line = 0; line < spans.size(); ++line)
	{
		if (position <= spans[line].end)
		{
			return static_cast<LRESULT>(line);
		}
	}
	return static_cast<LRESULT>(spans.size()) - 1;
}

LRESULT Win32Edit::lineLength(int index) const
{
	const std::vector<LineSpan> spans = spansOf(text());
	const LineSpan& span = spans[static_cast<size_t>(lineFromChar(index))];
	return span.end - span.start;
}

LRESULT Win32Edit::getLine(int line, LPSTR buffer) const
{
	const std::vector<LineSpan> spans = spansOf(text());
	if (buffer == nullptr || line < 0 || line >= static_cast<int>(spans.size()))
	{
		return 0;
	}
	WORD capacity = 0;
	memcpy(&capacity, buffer, sizeof(capacity));
	const LineSpan& span = spans[static_cast<size_t>(line)];
	const int count = std::min<int>(capacity, span.end - span.start);
	memcpy(buffer, text().c_str() + span.start, static_cast<size_t>(count));
	return count;
}

LRESULT Win32Edit::dialogCode() const
{
	const LRESULT code = DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS;
	return isMultiline() && (style() & ES_WANTRETURN) != 0 ? code | DLGC_WANTALLKEYS : code;
}

LRESULT Win32Edit::handleEditMessage(UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
	handled = true;
	switch (message)
	{
	case EM_GETSEL:
		return getSelection(wParam, lParam);
	case EM_SETSEL:
		setSelection(static_cast<int>(static_cast<INT_PTR>(wParam)), static_cast<int>(lParam));
		return 0;
	case EM_REPLACESEL:
		replaceSelection(reinterpret_cast<LPCSTR>(lParam) != nullptr ? reinterpret_cast<LPCSTR>(lParam) : "", true);
		return 0;
	case EM_LIMITTEXT:
		m_limit = wParam != 0 ? static_cast<int>(wParam) : 0x7FFFFFFE;
		return 0;
	case EM_GETLIMITTEXT:
		return m_limit;
	case EM_SETREADONLY:
		m_window.style = wParam != 0 ? (m_window.style | ES_READONLY) : (m_window.style & ~ES_READONLY);
		return TRUE;
	case EM_GETMODIFY:
		return m_isModified ? TRUE : FALSE;
	case EM_SETMODIFY:
		m_isModified = wParam != 0;
		return 0;
	case EM_CANUNDO:
		return m_undoText != text() ? TRUE : FALSE;
	case EM_GETLINECOUNT:
		return lineCount();
	case EM_LINEINDEX:
		return lineIndex(static_cast<int>(static_cast<INT_PTR>(wParam)));
	case EM_LINELENGTH:
		return lineLength(static_cast<int>(static_cast<INT_PTR>(wParam)));
	case EM_GETLINE:
		return getLine(static_cast<int>(wParam), reinterpret_cast<LPSTR>(lParam));
	case EM_LINEFROMCHAR:
		return lineFromChar(static_cast<int>(static_cast<INT_PTR>(wParam)));
	case EM_SCROLLCARET:
		m_wantsCaretScroll = true;
		return TRUE;
	default:
		handled = false;
		return 0;
	}
}

LRESULT Win32Edit::handleClipboardMessage(UINT message, bool& handled)
{
	handled = true;
	switch (message)
	{
	case WM_CUT:
		copySelection(true);
		return 0;
	case WM_COPY:
		copySelection(false);
		return 0;
	case WM_PASTE:
		paste();
		return 0;
	case WM_CLEAR:
		replaceSelection("", true);
		return 0;
	case EM_UNDO:
	case WM_UNDO:
		undo();
		return TRUE;
	default:
		handled = false;
		return 0;
	}
}

LRESULT Win32Edit::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	bool handled = false;
	LRESULT result = handleEditMessage(message, wParam, lParam, handled);
	if (handled)
	{
		return result;
	}
	result = handleClipboardMessage(message, handled);
	if (handled)
	{
		return result;
	}

	switch (message)
	{
	case WM_CREATE:
		m_editorText = toEditorText(text());
		return 0;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		takeFocus();
		return 0;
	case WM_SETTEXT:
		setText(reinterpret_cast<LPCSTR>(lParam) != nullptr ? reinterpret_cast<LPCSTR>(lParam) : "", true);
		return TRUE;
	case WM_CHAR:
		if (!m_isEditing)
		{
			insertCharacter(static_cast<char>(wParam));
		}
		return 0;
	case WM_KEYDOWN:
		if (!m_isEditing && wParam == VK_DELETE)
		{
			deleteForward();
		}
		return 0;
	case WM_GETDLGCODE:
		return dialogCode();
	case WM_SETFOCUS:
		m_wantsEditorFocus = !m_isEditing;
		notifyCommand(EN_SETFOCUS);
		return 0;
	case WM_KILLFOCUS:
		m_wantsEditorBlur = true;
		notifyCommand(EN_KILLFOCUS);
		return 0;
	default:
		return Win32Control::handleMessage(message, wParam, lParam);
	}
}

ImGuiInputTextFlags Win32Edit::inputFlags() const
{
	ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_CallbackEdit;
	flags |= isReadOnly() ? ImGuiInputTextFlags_ReadOnly : 0;
	flags |= (style() & ES_PASSWORD) != 0 ? ImGuiInputTextFlags_Password : 0;
	flags |= (style() & ES_UPPERCASE) != 0 ? ImGuiInputTextFlags_CharsUppercase : 0;
	flags |= (style() & (ES_NUMBER | ES_LOWERCASE)) != 0 ? ImGuiInputTextFlags_CallbackCharFilter : 0;
	flags |= isMultiline() && (style() & ES_WANTRETURN) == 0 ? ImGuiInputTextFlags_CtrlEnterForNewLine : 0;
	return flags;
}

void Win32Edit::synchronizeEditorState()
{
	ImGuiInputTextState* state = ImGui::GetInputTextState(m_editorId);
	if (state != nullptr && m_needsReload)
	{
		state->ReloadUserBufAndKeepSelection();
	}
	if (state != nullptr && m_wantsCaretScroll)
	{
		state->CursorFollow = true;
	}
	m_needsReload = false;
	m_wantsCaretScroll = false;
}

void Win32Edit::draw(const Win32DrawContext& context)
{
	const ImVec2 size = Win32ControlDrawing::clientSize(context);
	const float verticalPadding = std::max(0.0f, (size.y - ImGui::GetFontSize()) * 0.5f);
	Win32ControlDrawing::placeCursor(context);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, isMultiline() ? ImGui::GetStyle().FramePadding.y : verticalPadding));
	if (m_wantsEditorFocus)
	{
		ImGui::SetKeyboardFocusHere();
		m_wantsEditorFocus = false;
	}
	synchronizeEditorState();

	ImGui::SetNextItemWidth(size.x);
	const bool isEdited = isMultiline() ? ImGui::InputTextMultiline("##edit", &m_editorText, size, inputFlags(), trackInput, this)
		: ImGui::InputText("##edit", &m_editorText, inputFlags(), trackInput, this);
	ImGui::PopStyleVar();

	m_editorId = ImGui::GetItemID();
	if (ImGui::IsItemActivated())
	{
		queueInput([this]() { takeFocus(); });
	}
	if (isEdited)
	{
		const std::string snapshot = m_editorText;
		queueInput([this, snapshot]() { applyUserEdit(snapshot); });
	}

	m_isEditing = ImGui::IsItemActive();
	if (m_wantsEditorBlur)
	{
		if (m_isEditing)
		{
			ImGui::ClearActiveID();
		}
		m_wantsEditorBlur = false;
	}
}
