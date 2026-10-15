#include "Win32Metrics.h"

#include "Win32SystemColors.h"
#include "Win32Window.h"

#include "imgui.h"

#include <cfloat>
#include <cmath>

namespace
{

const char kAverageWidthSample[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

}

namespace Win32Metrics
{

ImFont* systemFont()
{
	const ImGuiIO& io = ImGui::GetIO();
	return io.FontDefault != nullptr ? io.FontDefault : io.Fonts->Fonts[0];
}

float systemFontSize()
{
	const ImGuiStyle& style = ImGui::GetStyle();
	return style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi;
}

int captionHeight()
{
	return static_cast<int>(std::ceil(systemFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f));
}

int frameBorder()
{
	return static_cast<int>(std::ceil(ImGui::GetStyle().WindowBorderSize)) + 2;
}

int menuHeight()
{
	return captionHeight();
}

int scrollBarWidth()
{
	return static_cast<int>(std::ceil(ImGui::GetStyle().ScrollbarSize));
}

int lineHeight()
{
	return static_cast<int>(std::ceil(systemFontSize() + ImGui::GetStyle().ItemSpacing.y));
}

SIZE dialogBaseUnits()
{
	const float fontSize = systemFontSize();
	const ImVec2 sample = systemFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, kAverageWidthSample);
	SIZE units;
	units.cx = static_cast<LONG>((sample.x / 26.0f + 1.0f) / 2.0f);
	units.cy = static_cast<LONG>(std::ceil(fontSize));
	return units;
}

int dialogUnitsToPixelsX(int units)
{
	return MulDiv(units, dialogBaseUnits().cx, 4);
}

int dialogUnitsToPixelsY(int units)
{
	return MulDiv(units, dialogBaseUnits().cy, 8);
}

int systemMetric(int index)
{
	const SIZE host = Win32WindowManager::instance().hostSize();
	switch (index)
	{
	case SM_CXSCREEN:
	case SM_CXFULLSCREEN:
		return host.cx;
	case SM_CYSCREEN:
	case SM_CYFULLSCREEN:
		return host.cy;
	case SM_CXVSCROLL:
	case SM_CYHSCROLL:
		return scrollBarWidth();
	case SM_CYCAPTION:
	case SM_CYSMCAPTION:
		return captionHeight();
	case SM_CYMENU:
		return menuHeight();
	case SM_CXBORDER:
	case SM_CYBORDER:
		return 1;
	case SM_CXEDGE:
	case SM_CYEDGE:
		return 2;
	case SM_CXDLGFRAME:
	case SM_CYDLGFRAME:
	case SM_CXFRAME:
	case SM_CYFRAME:
		return frameBorder();
	case SM_CXICON:
	case SM_CYICON:
	case SM_CXCURSOR:
	case SM_CYCURSOR:
		return 32;
	case SM_CXSMICON:
	case SM_CYSMICON:
		return 16;
	case SM_CXDOUBLECLK:
	case SM_CYDOUBLECLK:
		return 4;
	default:
		return 0;
	}
}

}

int GetSystemMetrics(int index)
{
	return Win32Metrics::systemMetric(index);
}

DWORD GetSysColor(int index)
{
	return Win32SystemColors::color(index);
}

LONG GetDialogBaseUnits()
{
	const SIZE units = Win32Metrics::dialogBaseUnits();
	return MAKELONG(units.cx, units.cy);
}
