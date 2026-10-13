#include "Win32Edit.h"

#include "Win32ControlDrawing.h"

#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include <algorithm>
#include <cctype>

namespace
{

int countNewlines(const char* text, int length)
{
	return static_cast<int>(std::count(text, text + std::max(0, length), '\n'));
}

int countCarriageReturns(const std::string& text, int length)
{
	const int bounded = std::min(std::max(0, length), static_cast<int>(text.size()));
	return static_cast<int>(std::count(text.begin(), text.begin() + bounded, '\r'));
}

}

std::string Win32Edit::toEditorText(const std::string& text) const
{
	if (!isMultiline())
	{
		return text;
	}
	std::string result;
	result.reserve(text.size());
	for (char symbol : text)
	{
		if (symbol != '\r')
		{
			result += symbol;
		}
	}
	return result;
}

std::string Win32Edit::fromEditorText(const std::string& text) const
{
	if (!isMultiline())
	{
		return text;
	}
	std::string result;
	result.reserve(text.size() + 16);
	for (char symbol : text)
	{
		if (symbol == '\n')
		{
			result += '\r';
		}
		result += symbol;
	}
	return result;
}

int Win32Edit::editorToTextIndex(int index) const
{
	return isMultiline() ? index + countNewlines(m_editorText.c_str(), std::min(index, static_cast<int>(m_editorText.size()))) : index;
}

int Win32Edit::textToEditorIndex(int index) const
{
	return isMultiline() ? index - countCarriageReturns(text(), index) : index;
}

void Win32Edit::notifyChange()
{
	notifyCommand(EN_UPDATE);
	notifyCommand(EN_CHANGE);
}

void Win32Edit::setText(const std::string& newText, bool notify)
{
	m_window.text = newText;
	m_editorText = toEditorText(newText);
	m_selectionStart = 0;
	m_selectionEnd = 0;
	m_isModified = false;
	m_needsReload = true;
	if (notify)
	{
		notifyChange();
	}
}

void Win32Edit::replaceSelection(const std::string& replacement, bool notify)
{
	const std::string current = text();
	const int start = std::clamp(m_selectionStart, 0, static_cast<int>(current.size()));
	const int end = std::clamp(m_selectionEnd, start, static_cast<int>(current.size()));
	const int room = std::max(0, m_limit - (static_cast<int>(current.size()) - (end - start)));
	const std::string inserted = replacement.substr(0, static_cast<size_t>(room));

	m_undoText = current;
	m_window.text = current.substr(0, static_cast<size_t>(start)) + inserted + current.substr(static_cast<size_t>(end));
	m_editorText = toEditorText(m_window.text);
	m_selectionStart = start + static_cast<int>(inserted.size());
	m_selectionEnd = m_selectionStart;
	m_isModified = true;
	m_needsReload = true;
	m_hasPendingSelection = true;
	if (notify)
	{
		notifyChange();
	}
}

void Win32Edit::setSelection(int start, int end)
{
	const int length = static_cast<int>(text().size());
	if (start < 0)
	{
		start = m_selectionEnd;
		end = m_selectionEnd;
	}
	if (end < 0 || end > length)
	{
		end = length;
	}
	start = std::clamp(start, 0, length);
	m_selectionStart = std::min(start, end);
	m_selectionEnd = std::max(start, end);
	m_hasPendingSelection = true;
}

void Win32Edit::applyUserEdit(const std::string& editorText)
{
	const std::string newText = fromEditorText(editorText);
	if (newText == text())
	{
		return;
	}
	m_undoText = text();
	m_window.text = newText;
	m_isModified = true;
	notifyChange();
}

void Win32Edit::insertCharacter(char symbol)
{
	if (isReadOnly())
	{
		return;
	}
	if (symbol == 0x08)
	{
		if (m_selectionStart == m_selectionEnd && m_selectionStart > 0)
		{
			--m_selectionStart;
		}
		replaceSelection("", true);
		return;
	}
	if (static_cast<unsigned char>(symbol) < 0x20 && !(isMultiline() && (symbol == '\r' || symbol == '\t')))
	{
		return;
	}
	if ((style() & ES_NUMBER) != 0 && !std::isdigit(static_cast<unsigned char>(symbol)))
	{
		MessageBeep(0);
		return;
	}
	const char converted = (style() & ES_UPPERCASE) != 0 ? static_cast<char>(std::toupper(static_cast<unsigned char>(symbol)))
		: (style() & ES_LOWERCASE) != 0 ? static_cast<char>(std::tolower(static_cast<unsigned char>(symbol))) : symbol;
	replaceSelection(converted == '\r' ? std::string("\r\n") : std::string(1, converted), true);
}

void Win32Edit::deleteForward()
{
	if (isReadOnly())
	{
		return;
	}
	if (m_selectionStart == m_selectionEnd && m_selectionEnd < static_cast<int>(text().size()))
	{
		++m_selectionEnd;
	}
	replaceSelection("", true);
}

void Win32Edit::copySelection(bool remove)
{
	if (m_selectionStart == m_selectionEnd || (style() & ES_PASSWORD) != 0)
	{
		return;
	}
	const std::string selected = text().substr(static_cast<size_t>(m_selectionStart), static_cast<size_t>(m_selectionEnd - m_selectionStart));
	ImGui::SetClipboardText(selected.c_str());
	if (remove && !isReadOnly())
	{
		replaceSelection("", true);
	}
}

void Win32Edit::paste()
{
	const char* clipboard = ImGui::GetClipboardText();
	if (clipboard != nullptr && !isReadOnly())
	{
		replaceSelection(isMultiline() ? fromEditorText(toEditorText(clipboard)) : std::string(clipboard), true);
	}
}

void Win32Edit::undo()
{
	const std::string previous = m_undoText;
	m_undoText = text();
	m_window.text = previous;
	m_editorText = toEditorText(previous);
	m_needsReload = true;
	notifyChange();
}
