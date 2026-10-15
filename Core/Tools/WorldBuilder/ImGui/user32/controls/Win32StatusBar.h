#pragma once

#include "Win32Control.h"

#include <string>
#include <vector>

class Win32StatusBar : public Win32Control
{
public:
	explicit Win32StatusBar(Win32Window& window) : Win32Control(window) {}

	LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
	void draw(const Win32DrawContext& context) override;
	bool drawsOwnFocus() const override { return false; }

private:
	struct Part
	{
		int rightEdge = -1;
		std::string text;
		UINT type = 0;
	};

	Part* partAt(WPARAM index);
	const Part* partAt(WPARAM index) const;
	LRESULT setParts(int count, const int* rightEdges);
	LRESULT getParts(int count, int* rightEdges) const;
	LRESULT setText(WPARAM indexAndType, LPCSTR text);
	LRESULT getText(WPARAM index, LPSTR buffer) const;
	LRESULT textLength(WPARAM index) const;
	LRESULT getRect(WPARAM index, RECT* rect) const;
	LRESULT handleTextMessage(UINT message, WPARAM wParam, LPARAM lParam);
	RECT partRect(size_t index) const;
	void drawPart(const Win32DrawContext& context, const Part& part, const RECT& rect);

	std::vector<Part> m_parts = std::vector<Part>(1);
	Part m_simplePart;
	bool m_isSimple = false;
	int m_minHeight = 0;
};
