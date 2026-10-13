#include "Win32Trackbar.h"

#include "Win32ControlDrawing.h"
#include "Win32Gdi.h"

#include <algorithm>
#include <utility>

int Win32Trackbar::clampPosition(int position) const
{
	return std::clamp(position, m_minimum, std::max(m_minimum, m_maximum));
}

void Win32Trackbar::setRange(int minimum, int maximum, bool redraw)
{
	m_minimum = minimum;
	m_maximum = maximum;
	m_position = clampPosition(m_position);
	if (redraw)
	{
		InvalidateRect(hwnd(), nullptr, TRUE);
	}
}

void Win32Trackbar::notifyScroll(WORD code)
{
	const HWND parent = parentHwnd();
	if (parent == nullptr)
	{
		return;
	}
	const UINT message = isVertical() ? WM_VSCROLL : WM_HSCROLL;
	const WORD position = static_cast<WORD>(code == TB_THUMBTRACK || code == TB_THUMBPOSITION ? m_position : 0);
	SendMessageA(parent, message, MAKEWPARAM(code, position), reinterpret_cast<LPARAM>(hwnd()));
}

void Win32Trackbar::moveByUser(int position, WORD code)
{
	takeFocus();
	m_position = clampPosition(position);
	notifyScroll(code);
}

LRESULT Win32Trackbar::handleKey(WPARAM key)
{
	switch (key)
	{
	case VK_LEFT:
	case VK_UP:
		moveByUser(m_position - m_lineSize, TB_LINEUP);
		break;
	case VK_RIGHT:
	case VK_DOWN:
		moveByUser(m_position + m_lineSize, TB_LINEDOWN);
		break;
	case VK_PRIOR:
		moveByUser(m_position - m_pageSize, TB_PAGEUP);
		break;
	case VK_NEXT:
		moveByUser(m_position + m_pageSize, TB_PAGEDOWN);
		break;
	case VK_HOME:
		moveByUser(m_minimum, TB_TOP);
		break;
	case VK_END:
		moveByUser(m_maximum, TB_BOTTOM);
		break;
	default:
		return 0;
	}
	notifyScroll(TB_ENDTRACK);
	return 0;
}

LRESULT Win32Trackbar::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case TBM_GETPOS:
		return m_position;
	case TBM_GETRANGEMIN:
		return m_minimum;
	case TBM_GETRANGEMAX:
		return m_maximum;
	case TBM_SETPOS:
		m_position = clampPosition(static_cast<int>(lParam));
		return 0;
	case TBM_SETRANGE:
		setRange(static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)), wParam != 0);
		return 0;
	case TBM_SETRANGEMIN:
		setRange(static_cast<int>(lParam), m_maximum, wParam != 0);
		return 0;
	case TBM_SETRANGEMAX:
		setRange(m_minimum, static_cast<int>(lParam), wParam != 0);
		return 0;
	case TBM_SETTICFREQ:
		m_tickFrequency = std::max(1, static_cast<int>(wParam));
		return 0;
	case TBM_SETPAGESIZE:
		return std::exchange(m_pageSize, static_cast<int>(lParam));
	case TBM_GETPAGESIZE:
		return m_pageSize;
	case TBM_SETLINESIZE:
		return std::exchange(m_lineSize, static_cast<int>(lParam));
	case TBM_GETLINESIZE:
		return m_lineSize;
	case WM_GETDLGCODE:
		return DLGC_WANTARROWS;
	case WM_KEYDOWN:
		return handleKey(wParam);
	case WM_LBUTTONDOWN:
	case WM_MOUSEMOVE:
	case WM_LBUTTONUP:
	case WM_CAPTURECHANGED:
		return handleMouse(message, lParam);
	default:
		return handleSelectionMessage(message, wParam, lParam);
	}
}

LRESULT Win32Trackbar::handleSelectionMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case TBM_SETSEL:
		m_selectionStart = static_cast<short>(LOWORD(lParam));
		m_selectionEnd = static_cast<short>(HIWORD(lParam));
		return 0;
	case TBM_SETSELSTART:
		m_selectionStart = static_cast<int>(lParam);
		return 0;
	case TBM_SETSELEND:
		m_selectionEnd = static_cast<int>(lParam);
		return 0;
	case TBM_GETSELSTART:
		return m_selectionStart;
	case TBM_GETSELEND:
		return m_selectionEnd;
	case TBM_CLEARSEL:
		m_selectionStart = 0;
		m_selectionEnd = 0;
		return 0;
	default:
		return Win32Control::handleMessage(message, wParam, lParam);
	}
}

namespace
{

const int kThumbBreadth = 10;
const int kChannelMargin = kThumbBreadth / 2 + 2;

}

int Win32Trackbar::length() const
{
	const int extent = isVertical() ? m_window.height() : m_window.width();
	return std::max(1, extent - kChannelMargin * 2);
}

int Win32Trackbar::thumbOffset() const
{
	const int span = std::max(1, m_maximum - m_minimum);
	return kChannelMargin + MulDiv(m_position - m_minimum, length(), span);
}

int Win32Trackbar::positionAt(int offset) const
{
	const int span = std::max(1, m_maximum - m_minimum);
	return clampPosition(m_minimum + MulDiv(offset - kChannelMargin, span, length()));
}

int Win32Trackbar::offsetAlong(LPARAM point) const
{
	return isVertical() ? GET_Y_LPARAM(point) : GET_X_LPARAM(point);
}

RECT Win32Trackbar::thumbRect() const
{
	RECT rect;
	const int center = thumbOffset();
	if (isVertical())
	{
		SetRect(&rect, 0, center - kThumbBreadth / 2, m_window.width(), center + kThumbBreadth / 2);
		return rect;
	}
	SetRect(&rect, center - kThumbBreadth / 2, 0, center + kThumbBreadth / 2, m_window.height());
	return rect;
}

LRESULT Win32Trackbar::handleMouse(UINT message, LPARAM lParam)
{
	switch (message)
	{
	case WM_LBUTTONDOWN:
	{
		takeFocus();
		const POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		const RECT thumb = thumbRect();
		if (PtInRect(&thumb, point))
		{
			m_isTracking = true;
			SetCapture(hwnd());
			return 0;
		}
		const bool before = offsetAlong(lParam) < thumbOffset();
		moveByUser(m_position + (before ? -m_pageSize : m_pageSize), before ? TB_PAGEUP : TB_PAGEDOWN);
		notifyScroll(TB_ENDTRACK);
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		const int position = positionAt(offsetAlong(lParam));
		if (m_isTracking && position != m_position)
		{
			moveByUser(position, TB_THUMBTRACK);
		}
		return 0;
	}
	case WM_LBUTTONUP:
		if (!m_isTracking)
		{
			return 0;
		}
		m_isTracking = false;
		ReleaseCapture();
		notifyScroll(TB_THUMBPOSITION);
		notifyScroll(TB_ENDTRACK);
		return 0;
	default:
		m_isTracking = false;
		return 0;
	}
}

void Win32Trackbar::draw(const Win32DrawContext& context)
{
	const ImVec2 origin = Win32ControlDrawing::screenPoint(context, 0, 0);
	const ImVec2 size = Win32ControlDrawing::clientSize(context);
	const float channelStart = static_cast<float>(kChannelMargin);
	const float channelEnd = static_cast<float>(kChannelMargin + length());
	const ImU32 channelColor = Win32ControlDrawing::color(ImGuiCol_FrameBg);
	const ImU32 tickColor = Win32ControlDrawing::color(ImGuiCol_Border);
	if (isVertical())
	{
		const float x = origin.x + size.x * 0.5f;
		context.drawList->AddRectFilled(ImVec2(x - 2.0f, origin.y + channelStart), ImVec2(x + 2.0f, origin.y + channelEnd), channelColor);
	}
	else
	{
		const float y = origin.y + size.y * 0.5f;
		context.drawList->AddRectFilled(ImVec2(origin.x + channelStart, y - 2.0f), ImVec2(origin.x + channelEnd, y + 2.0f), channelColor);
	}

	if ((style() & TBS_AUTOTICKS) != 0)
	{
		const int span = std::max(1, m_maximum - m_minimum);
		for (int value = m_minimum; value <= m_maximum; value += m_tickFrequency)
		{
			const float offset = channelStart + static_cast<float>(MulDiv(value - m_minimum, length(), span));
			const ImVec2 tickMin = isVertical() ? ImVec2(origin.x + size.x - 4.0f, origin.y + offset) : ImVec2(origin.x + offset, origin.y + size.y - 4.0f);
			const ImVec2 tickMax = isVertical() ? ImVec2(origin.x + size.x, origin.y + offset) : ImVec2(origin.x + offset, origin.y + size.y);
			context.drawList->AddLine(tickMin, tickMax, tickColor);
		}
	}

	const RECT thumb = thumbRect();
	const ImVec2 thumbMin(origin.x + thumb.left, origin.y + thumb.top);
	const ImVec2 thumbMax(origin.x + thumb.right, origin.y + thumb.bottom - ((style() & TBS_AUTOTICKS) != 0 && !isVertical() ? 5.0f : 0.0f));
	const ImGuiCol thumbColor = m_isTracking ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab;
	context.drawList->AddRectFilled(thumbMin, thumbMax, Win32ControlDrawing::color(thumbColor), ImGui::GetStyle().GrabRounding);
	if (context.hasFocus)
	{
		context.drawList->AddRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), Win32ControlDrawing::color(ImGuiCol_NavCursor));
	}
}

int Win32Progress::setPosition(int position)
{
	const int previous = m_position;
	m_position = std::clamp(position, m_minimum, std::max(m_minimum, m_maximum));
	return previous;
}

LRESULT Win32Progress::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case PBM_SETRANGE:
	{
		const LRESULT previous = MAKELRESULT(m_minimum, m_maximum);
		m_minimum = LOWORD(lParam);
		m_maximum = HIWORD(lParam);
		return previous;
	}
	case PBM_SETRANGE32:
	{
		const LRESULT previous = MAKELRESULT(m_minimum, m_maximum);
		m_minimum = static_cast<int>(wParam);
		m_maximum = static_cast<int>(lParam);
		return previous;
	}
	case PBM_GETRANGE:
	{
		PBRANGE* range = reinterpret_cast<PBRANGE*>(lParam);
		if (range != nullptr)
		{
			range->iLow = m_minimum;
			range->iHigh = m_maximum;
		}
		return wParam != 0 ? m_minimum : m_maximum;
	}
	case PBM_SETPOS:
		return setPosition(static_cast<int>(wParam));
	case PBM_DELTAPOS:
		return setPosition(m_position + static_cast<int>(wParam));
	case PBM_GETPOS:
		return m_position;
	case PBM_SETSTEP:
		return std::exchange(m_step, static_cast<int>(wParam));
	case PBM_STEPIT:
	{
		const int next = m_position + m_step > m_maximum ? m_minimum + (m_position + m_step - m_maximum) : m_position + m_step;
		return setPosition(next);
	}
	case PBM_SETBARCOLOR:
		return static_cast<LRESULT>(std::exchange(m_barColor, static_cast<COLORREF>(lParam)));
	default:
		return Win32Control::handleMessage(message, wParam, lParam);
	}
}

void Win32Progress::draw(const Win32DrawContext& context)
{
	Win32ControlDrawing::placeCursor(context);
	const float span = static_cast<float>(std::max(1, m_maximum - m_minimum));
	const float fraction = static_cast<float>(m_position - m_minimum) / span;
	const bool hasCustomColor = m_barColor != CLR_DEFAULT;
	if (hasCustomColor)
	{
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Win32Gdi::toImColor(m_barColor));
	}
	ImGui::ProgressBar(fraction, Win32ControlDrawing::clientSize(context), "");
	if (hasCustomColor)
	{
		ImGui::PopStyleColor();
	}
}
