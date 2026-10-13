#pragma once

#include "Win32Api.h"

struct ImFont;

namespace Win32Metrics
{
ImFont* systemFont();
float systemFontSize();
int captionHeight();
int frameBorder();
int menuHeight();
int scrollBarWidth();
int lineHeight();
SIZE dialogBaseUnits();
int dialogUnitsToPixelsX(int units);
int dialogUnitsToPixelsY(int units);
int systemMetric(int index);
}
