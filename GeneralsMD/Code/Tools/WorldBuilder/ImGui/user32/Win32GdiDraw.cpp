#include "Win32Gdi.h"

#include "Win32Metrics.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>

namespace
{

struct DashPattern
{
	const float* lengths;
	int count;
};

const float kDash[] = { 18.0f, 6.0f };
const float kDot[] = { 3.0f, 3.0f };
const float kDashDot[] = { 9.0f, 6.0f, 3.0f, 6.0f };
const float kDashDotDot[] = { 9.0f, 3.0f, 3.0f, 3.0f, 3.0f, 3.0f };

DashPattern dashPattern(int style)
{
	switch (style)
	{
	case PS_DASH:
		return { kDash, 2 };
	case PS_DOT:
		return { kDot, 2 };
	case PS_DASHDOT:
		return { kDashDot, 4 };
	case PS_DASHDOTDOT:
		return { kDashDotDot, 6 };
	default:
		return { nullptr, 0 };
	}
}

ImVec2 toClient(const Win32DeviceContext* dc, int x, int y)
{
	return ImVec2(static_cast<float>(x + dc->viewportOrigin.x), static_cast<float>(y + dc->viewportOrigin.y));
}

bool hasStroke(const Win32DeviceContext* dc)
{
	return dc->pen != nullptr && dc->pen->style != PS_NULL;
}

bool hasFill(const Win32Brush* brush)
{
	return brush != nullptr && !brush->isHollow;
}

float strokeWidth(const Win32DeviceContext* dc)
{
	return static_cast<float>(std::max(1, dc->pen->width));
}

Win32DrawCommand newCommand(const Win32DeviceContext* dc, Win32DrawKind kind)
{
	Win32DrawCommand command;
	command.kind = kind;
	command.clip = dc->clip;
	return command;
}

void record(Win32DeviceContext* dc, Win32DrawCommand command)
{
	if (dc->window == nullptr)
	{
		return;
	}
	Win32Gdi::displayListFor(dc).add(std::move(command));
}

void recordSegment(Win32DeviceContext* dc, ImVec2 from, ImVec2 to)
{
	Win32DrawCommand command = newCommand(dc, Win32DrawKind::Polyline);
	command.points = { from, to };
	command.color = Win32Gdi::toImColor(dc->pen->color);
	command.thickness = strokeWidth(dc);
	record(dc, std::move(command));
}

void recordDashedSegment(Win32DeviceContext* dc, ImVec2 from, ImVec2 to, const DashPattern& pattern)
{
	const ImVec2 delta(to.x - from.x, to.y - from.y);
	const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
	if (length <= 0.0f)
	{
		return;
	}

	const ImVec2 direction(delta.x / length, delta.y / length);
	float travelled = 0.0f;
	for (int index = 0; travelled < length; index = (index + 1) % pattern.count)
	{
		const float end = std::min(length, travelled + pattern.lengths[index]);
		if (index % 2 == 0)
		{
			recordSegment(dc, ImVec2(from.x + direction.x * travelled, from.y + direction.y * travelled),
				ImVec2(from.x + direction.x * end, from.y + direction.y * end));
		}
		travelled = end;
	}
}

void recordStroke(Win32DeviceContext* dc, const std::vector<ImVec2>& points, bool closed)
{
	if (!hasStroke(dc) || points.size() < 2)
	{
		return;
	}

	const DashPattern pattern = dashPattern(dc->pen->style);
	if (pattern.count == 0)
	{
		Win32DrawCommand command = newCommand(dc, Win32DrawKind::Polyline);
		command.points = points;
		command.closed = closed;
		command.color = Win32Gdi::toImColor(dc->pen->color);
		command.thickness = strokeWidth(dc);
		record(dc, std::move(command));
		return;
	}

	for (size_t index = 0; index + 1 < points.size(); ++index)
	{
		recordDashedSegment(dc, points[index], points[index + 1], pattern);
	}
	if (closed)
	{
		recordDashedSegment(dc, points.back(), points.front(), pattern);
	}
}

void recordFilledRect(Win32DeviceContext* dc, ImVec2 min, ImVec2 max, COLORREF color)
{
	Win32DrawCommand command = newCommand(dc, Win32DrawKind::FilledRect);
	command.min = min;
	command.max = max;
	command.fillColor = Win32Gdi::toImColor(color);
	command.filled = true;
	record(dc, std::move(command));
}

std::vector<ImVec2> toClientPoints(const Win32DeviceContext* dc, const POINT* points, int count)
{
	std::vector<ImVec2> converted;
	converted.reserve(static_cast<size_t>(std::max(0, count)));
	for (int index = 0; index < count; ++index)
	{
		converted.push_back(toClient(dc, points[index].x, points[index].y));
	}
	return converted;
}

float measureTextWidth(float fontSize, const char* begin, const char* end)
{
	return Win32Metrics::systemFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, begin, end).x;
}

void recordText(Win32DeviceContext* dc, ImVec2 position, const char* text, int length)
{
	const float fontSize = Win32Gdi::fontSizeFor(dc->font);
	const std::string content(text, static_cast<size_t>(std::max(0, length)));
	if (dc->backgroundMode == OPAQUE)
	{
		const float width = measureTextWidth(fontSize, content.c_str(), content.c_str() + content.size());
		recordFilledRect(dc, position, ImVec2(position.x + width, position.y + fontSize), dc->backgroundColor);
	}

	Win32DrawCommand command = newCommand(dc, Win32DrawKind::Text);
	command.min = position;
	command.text = content;
	command.fontSize = fontSize;
	command.isBold = dc->font != nullptr && dc->font->isBold();
	command.color = Win32Gdi::toImColor(dc->textColor);
	record(dc, std::move(command));
}

std::vector<std::string> wrapLines(const std::string& text, float fontSize, float width, bool wordBreak)
{
	std::vector<std::string> lines;
	size_t start = 0;
	while (start <= text.size())
	{
		const size_t newline = text.find('\n', start);
		const std::string paragraph = text.substr(start, newline == std::string::npos ? std::string::npos : newline - start);
		const char* cursor = paragraph.c_str();
		const char* end = cursor + paragraph.size();
		do
		{
			const char* wrap = wordBreak ? Win32Metrics::systemFont()->CalcWordWrapPosition(fontSize, cursor, end, width) : end;
			wrap = wrap == cursor && cursor < end ? cursor + 1 : wrap;
			lines.emplace_back(cursor, wrap);
			cursor = wrap;
			while (cursor < end && *cursor == ' ')
			{
				++cursor;
			}
		} while (cursor < end);

		if (newline == std::string::npos)
		{
			break;
		}
		start = newline + 1;
	}
	return lines;
}

float alignedX(UINT format, const RECT& rect, float lineWidth)
{
	if ((format & DT_CENTER) != 0)
	{
		return rect.left + (rect.right - rect.left - lineWidth) * 0.5f;
	}
	if ((format & DT_RIGHT) != 0)
	{
		return rect.right - lineWidth;
	}
	return static_cast<float>(rect.left);
}

float alignedY(UINT format, const RECT& rect, float blockHeight)
{
	if ((format & DT_SINGLELINE) == 0)
	{
		return static_cast<float>(rect.top);
	}
	if ((format & DT_VCENTER) != 0)
	{
		return rect.top + (rect.bottom - rect.top - blockHeight) * 0.5f;
	}
	if ((format & DT_BOTTOM) != 0)
	{
		return rect.bottom - blockHeight;
	}
	return static_cast<float>(rect.top);
}

void replayPolyline(ImDrawList* drawList, const Win32DrawCommand& command, ImVec2 origin)
{
	std::vector<ImVec2> points;
	points.reserve(command.points.size());
	for (const ImVec2& point : command.points)
	{
		points.push_back(ImVec2(origin.x + point.x + 0.5f, origin.y + point.y + 0.5f));
	}
	drawList->AddPolyline(points.data(), static_cast<int>(points.size()), command.color, command.thickness, command.closed ? ImDrawFlags_Closed : ImDrawFlags_None);
}

void replayPolygon(ImDrawList* drawList, const Win32DrawCommand& command, ImVec2 origin)
{
	std::vector<ImVec2> points;
	points.reserve(command.points.size());
	for (const ImVec2& point : command.points)
	{
		points.push_back(ImVec2(origin.x + point.x, origin.y + point.y));
	}
	if (command.filled)
	{
		drawList->AddConcavePolyFilled(points.data(), static_cast<int>(points.size()), command.fillColor);
	}
	if (command.outlined)
	{
		drawList->AddPolyline(points.data(), static_cast<int>(points.size()), command.color, command.thickness, ImDrawFlags_Closed);
	}
}

void replayRect(ImDrawList* drawList, const Win32DrawCommand& command, ImVec2 origin)
{
	const ImVec2 min(origin.x + command.min.x, origin.y + command.min.y);
	const ImVec2 max(origin.x + command.max.x, origin.y + command.max.y);
	if (command.filled)
	{
		drawList->AddRectFilled(min, max, command.fillColor);
	}
	if (command.outlined)
	{
		drawList->AddRect(min, max, command.color, 0.0f, command.thickness);
	}
}

void replayEllipse(ImDrawList* drawList, const Win32DrawCommand& command, ImVec2 origin)
{
	const ImVec2 center(origin.x + (command.min.x + command.max.x) * 0.5f, origin.y + (command.min.y + command.max.y) * 0.5f);
	const ImVec2 radius((command.max.x - command.min.x) * 0.5f, (command.max.y - command.min.y) * 0.5f);
	if (command.filled)
	{
		drawList->AddEllipseFilled(center, radius, command.fillColor);
	}
	if (command.outlined)
	{
		drawList->AddEllipse(center, radius, command.color, 0.0f, 0, command.thickness);
	}
}

void replayText(ImDrawList* drawList, const Win32DrawCommand& command, ImVec2 origin)
{
	const ImVec2 position(origin.x + command.min.x, origin.y + command.min.y);
	ImFont* font = Win32Metrics::systemFont();
	drawList->AddText(font, command.fontSize, position, command.color, command.text.c_str());
	if (command.isBold)
	{
		drawList->AddText(font, command.fontSize, ImVec2(position.x + 1.0f, position.y), command.color, command.text.c_str());
	}
}

void replayImage(ImDrawList* drawList, const Win32DrawCommand& command, ImVec2 origin)
{
	const ImTextureID texture = command.image != nullptr ? command.image->textureForDrawing() : ImTextureID_Invalid;
	if (texture == ImTextureID_Invalid)
	{
		return;
	}
	drawList->AddImage(ImTextureRef(texture), ImVec2(origin.x + command.min.x, origin.y + command.min.y),
		ImVec2(origin.x + command.max.x, origin.y + command.max.y), command.uvMin, command.uvMax);
}

void replayCommand(ImDrawList* drawList, const Win32DrawCommand& command, ImVec2 origin)
{
	switch (command.kind)
	{
	case Win32DrawKind::Polyline:
		replayPolyline(drawList, command, origin);
		return;
	case Win32DrawKind::FilledPolygon:
		replayPolygon(drawList, command, origin);
		return;
	case Win32DrawKind::FilledRect:
		replayRect(drawList, command, origin);
		return;
	case Win32DrawKind::Ellipse:
		replayEllipse(drawList, command, origin);
		return;
	case Win32DrawKind::Text:
		replayText(drawList, command, origin);
		return;
	case Win32DrawKind::Image:
		replayImage(drawList, command, origin);
		return;
	}
}

uint32_t dibPixel(const unsigned char* row, int x, int bitCount, const RGBQUAD* palette)
{
	if (bitCount == 32)
	{
		const unsigned char* pixel = row + x * 4;
		return 0xFF000000u | (pixel[2] << 16) | (pixel[1] << 8) | pixel[0];
	}
	if (bitCount == 24)
	{
		const unsigned char* pixel = row + x * 3;
		return 0xFF000000u | (pixel[2] << 16) | (pixel[1] << 8) | pixel[0];
	}
	if (bitCount == 16)
	{
		const uint16_t value = static_cast<uint16_t>(row[x * 2] | (row[x * 2 + 1] << 8));
		const uint32_t red = ((value >> 10) & 0x1F) * 255 / 31;
		const uint32_t green = ((value >> 5) & 0x1F) * 255 / 31;
		const uint32_t blue = (value & 0x1F) * 255 / 31;
		return 0xFF000000u | (red << 16) | (green << 8) | blue;
	}

	const int perByte = 8 / bitCount;
	const int shift = (perByte - 1 - x % perByte) * bitCount;
	const int index = (row[x / perByte] >> shift) & ((1 << bitCount) - 1);
	const RGBQUAD& color = palette[index];
	return 0xFF000000u | (color.rgbRed << 16) | (color.rgbGreen << 8) | color.rgbBlue;
}

}

void Win32DisplayList::replay(ImDrawList* drawList, ImVec2 origin) const
{
	for (const Win32DrawCommand& command : m_commands)
	{
		drawList->PushClipRect(ImVec2(origin.x + command.clip.left, origin.y + command.clip.top),
			ImVec2(origin.x + command.clip.right, origin.y + command.clip.bottom), true);
		replayCommand(drawList, command, origin);
		drawList->PopClipRect();
	}
}

namespace Win32Gdi
{

float fontSizeFor(const Win32Font* font)
{
	const float height = font != nullptr ? font->pixelHeight() : 0.0f;
	return height > 0.0f ? height : Win32Metrics::systemFontSize();
}

Win32Brush* brushFromHandle(HBRUSH handle)
{
	const uintptr_t value = reinterpret_cast<uintptr_t>(handle);
	if (value != 0 && value <= COLOR_INFOBK + 1)
	{
		handle = GetSysColorBrush(static_cast<int>(value) - 1);
	}
	Win32GdiObject* object = Win32Gdi::object(handle);
	return object != nullptr && object->type == Win32GdiObjectType::Brush ? static_cast<Win32Brush*>(object) : nullptr;
}

Win32ImagePtr imageFromDib(const BITMAPINFO* info, const void* bits)
{
	const BITMAPINFOHEADER& header = info->bmiHeader;
	const int width = header.biWidth;
	const int height = std::abs(header.biHeight);
	const int bitCount = header.biBitCount;
	const int stride = ((width * bitCount + 31) / 32) * 4;
	const bool isBottomUp = header.biHeight > 0;

	auto image = std::make_shared<Win32Image>();
	image->width = width;
	image->height = height;
	image->pixels.resize(static_cast<size_t>(width * height));
	const unsigned char* source = static_cast<const unsigned char*>(bits);
	for (int y = 0; y < height; ++y)
	{
		const unsigned char* row = source + stride * (isBottomUp ? height - 1 - y : y);
		for (int x = 0; x < width; ++x)
		{
			image->pixels[static_cast<size_t>(y * width + x)] = dibPixel(row, x, bitCount, info->bmiColors);
		}
	}
	return image;
}

}

BOOL MoveToEx(HDC hdc, int x, int y, LPPOINT previous)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr)
	{
		return FALSE;
	}
	if (previous != nullptr)
	{
		*previous = dc->position;
	}
	dc->position.x = x;
	dc->position.y = y;
	return TRUE;
}

BOOL LineTo(HDC hdc, int x, int y)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr)
	{
		return FALSE;
	}
	recordStroke(dc, { toClient(dc, dc->position.x, dc->position.y), toClient(dc, x, y) }, false);
	dc->position.x = x;
	dc->position.y = y;
	return TRUE;
}

BOOL Polyline(HDC hdc, const POINT* points, int count)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr || points == nullptr || count < 2)
	{
		return FALSE;
	}
	recordStroke(dc, toClientPoints(dc, points, count), false);
	return TRUE;
}

BOOL Polygon(HDC hdc, const POINT* points, int count)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr || points == nullptr || count < 2)
	{
		return FALSE;
	}

	const std::vector<ImVec2> converted = toClientPoints(dc, points, count);
	if (hasFill(dc->brush) && count >= 3)
	{
		Win32DrawCommand command = newCommand(dc, Win32DrawKind::FilledPolygon);
		command.points = converted;
		command.fillColor = Win32Gdi::toImColor(dc->brush->color);
		command.filled = true;
		record(dc, std::move(command));
	}
	recordStroke(dc, converted, true);
	return TRUE;
}

BOOL Rectangle(HDC hdc, int left, int top, int right, int bottom)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr)
	{
		return FALSE;
	}

	if (hasFill(dc->brush))
	{
		recordFilledRect(dc, toClient(dc, left, top), toClient(dc, right, bottom), dc->brush->color);
	}
	const POINT corners[] = { { left, top }, { right - 1, top }, { right - 1, bottom - 1 }, { left, bottom - 1 } };
	recordStroke(dc, toClientPoints(dc, corners, 4), true);
	return TRUE;
}

BOOL Ellipse(HDC hdc, int left, int top, int right, int bottom)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr)
	{
		return FALSE;
	}

	Win32DrawCommand command = newCommand(dc, Win32DrawKind::Ellipse);
	command.min = toClient(dc, left, top);
	command.max = toClient(dc, right, bottom);
	command.filled = hasFill(dc->brush);
	command.fillColor = command.filled ? Win32Gdi::toImColor(dc->brush->color) : 0;
	command.outlined = hasStroke(dc);
	command.color = command.outlined ? Win32Gdi::toImColor(dc->pen->color) : 0;
	command.thickness = command.outlined ? strokeWidth(dc) : 1.0f;
	record(dc, std::move(command));
	return TRUE;
}

int FillRect(HDC hdc, const RECT* rect, HBRUSH brushHandle)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	Win32Brush* brush = Win32Gdi::brushFromHandle(brushHandle);
	if (dc == nullptr || rect == nullptr || !hasFill(brush))
	{
		return 0;
	}
	recordFilledRect(dc, toClient(dc, rect->left, rect->top), toClient(dc, rect->right, rect->bottom), brush->color);
	return 1;
}

int FrameRect(HDC hdc, const RECT* rect, HBRUSH brushHandle)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	Win32Brush* brush = Win32Gdi::brushFromHandle(brushHandle);
	if (dc == nullptr || rect == nullptr || !hasFill(brush))
	{
		return 0;
	}

	Win32DrawCommand command = newCommand(dc, Win32DrawKind::FilledRect);
	command.min = toClient(dc, rect->left, rect->top);
	command.max = toClient(dc, rect->right, rect->bottom);
	command.color = Win32Gdi::toImColor(brush->color);
	command.outlined = true;
	record(dc, std::move(command));
	return 1;
}

COLORREF SetPixel(HDC hdc, int x, int y, COLORREF color)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr)
	{
		return CLR_INVALID;
	}
	recordFilledRect(dc, toClient(dc, x, y), toClient(dc, x + 1, y + 1), color);
	return color;
}

BOOL TextOutA(HDC hdc, int x, int y, LPCSTR text, int length)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr || text == nullptr)
	{
		return FALSE;
	}
	recordText(dc, toClient(dc, x, y), text, length);
	return TRUE;
}

BOOL ExtTextOutA(HDC hdc, int x, int y, UINT options, const RECT* rect, LPCSTR text, UINT length, const int*)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr)
	{
		return FALSE;
	}

	if ((options & ETO_OPAQUE) != 0 && rect != nullptr)
	{
		recordFilledRect(dc, toClient(dc, rect->left, rect->top), toClient(dc, rect->right, rect->bottom), dc->backgroundColor);
	}
	if (text != nullptr && length > 0)
	{
		recordText(dc, toClient(dc, x, y), text, static_cast<int>(length));
	}
	return TRUE;
}

int DrawTextA(HDC hdc, LPCSTR text, int length, LPRECT rect, UINT format)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr || text == nullptr || rect == nullptr)
	{
		return 0;
	}

	const std::string content = length < 0 ? std::string(text) : std::string(text, static_cast<size_t>(length));
	const float fontSize = Win32Gdi::fontSizeFor(dc->font);
	const bool singleLine = (format & DT_SINGLELINE) != 0;
	const std::vector<std::string> lines = singleLine ? std::vector<std::string>{ content } :
		wrapLines(content, fontSize, static_cast<float>(rect->right - rect->left), (format & DT_WORDBREAK) != 0);

	float widest = 0.0f;
	for (const std::string& line : lines)
	{
		widest = std::max(widest, measureTextWidth(fontSize, line.c_str(), line.c_str() + line.size()));
	}
	const float blockHeight = fontSize * static_cast<float>(lines.size());
	if ((format & DT_CALCRECT) != 0)
	{
		rect->right = rect->left + static_cast<LONG>(std::ceil(widest));
		rect->bottom = rect->top + static_cast<LONG>(std::ceil(blockHeight));
		return static_cast<int>(std::ceil(blockHeight));
	}

	const RECT savedClip = dc->clip;
	if ((format & DT_NOCLIP) == 0)
	{
		RECT textClip = *rect;
		OffsetRect(&textClip, dc->viewportOrigin.x, dc->viewportOrigin.y);
		IntersectRect(&dc->clip, &dc->clip, &textClip);
	}

	float y = alignedY(format, *rect, blockHeight);
	for (const std::string& line : lines)
	{
		const float width = measureTextWidth(fontSize, line.c_str(), line.c_str() + line.size());
		recordText(dc, toClient(dc, static_cast<int>(alignedX(format, *rect, width)), static_cast<int>(y)), line.c_str(), static_cast<int>(line.size()));
		y += fontSize;
	}
	dc->clip = savedClip;
	return static_cast<int>(std::ceil(blockHeight));
}

BOOL GetTextExtentPoint32A(HDC hdc, LPCSTR text, int length, LPSIZE size)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr || text == nullptr || size == nullptr)
	{
		return FALSE;
	}

	const float fontSize = Win32Gdi::fontSizeFor(dc->font);
	size->cx = static_cast<LONG>(std::ceil(measureTextWidth(fontSize, text, text + std::max(0, length))));
	size->cy = static_cast<LONG>(std::ceil(fontSize));
	return TRUE;
}

BOOL GetTextMetricsA(HDC hdc, LPTEXTMETRICA metrics)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr || metrics == nullptr)
	{
		return FALSE;
	}

	const float fontSize = Win32Gdi::fontSizeFor(dc->font);
	const float scale = fontSize / Win32Metrics::systemFontSize();
	*metrics = {};
	metrics->tmHeight = static_cast<long>(std::ceil(fontSize));
	metrics->tmAscent = static_cast<long>(std::ceil(fontSize * 0.8f));
	metrics->tmDescent = metrics->tmHeight - metrics->tmAscent;
	metrics->tmAveCharWidth = static_cast<long>(std::lround(Win32Metrics::dialogBaseUnits().cx * scale));
	metrics->tmMaxCharWidth = static_cast<long>(std::ceil(measureTextWidth(fontSize, "W", nullptr)));
	metrics->tmWeight = dc->font != nullptr ? dc->font->logFont.lfWeight : FW_NORMAL;
	return TRUE;
}

int StretchDIBits(HDC hdc, int destX, int destY, int destWidth, int destHeight, int sourceX, int sourceY, int sourceWidth, int sourceHeight,
	const void* bits, const BITMAPINFO* info, UINT, DWORD)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr || bits == nullptr || info == nullptr || sourceWidth == 0 || sourceHeight == 0)
	{
		return 0;
	}

	Win32ImagePtr image = Win32Gdi::imageFromDib(info, bits);
	const bool isBottomUp = info->bmiHeader.biHeight > 0;
	const int top = isBottomUp ? image->height - (sourceY + sourceHeight) : sourceY;

	Win32DrawCommand command = newCommand(dc, Win32DrawKind::Image);
	command.min = toClient(dc, destX, destY);
	command.max = toClient(dc, destX + destWidth, destY + destHeight);
	command.uvMin = ImVec2(static_cast<float>(sourceX) / image->width, static_cast<float>(top) / image->height);
	command.uvMax = ImVec2(static_cast<float>(sourceX + sourceWidth) / image->width, static_cast<float>(top + sourceHeight) / image->height);
	command.image = image;
	record(dc, std::move(command));
	return sourceHeight;
}

BOOL DrawIconEx(HDC hdc, int x, int y, HICON iconHandle, int width, int height, UINT, HBRUSH, UINT)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	Win32Icon* icon = Win32Gdi::icon(iconHandle);
	if (dc == nullptr || icon == nullptr)
	{
		return FALSE;
	}

	const int drawWidth = width > 0 ? width : icon->image->width;
	const int drawHeight = height > 0 ? height : icon->image->height;
	Win32DrawCommand command = newCommand(dc, Win32DrawKind::Image);
	command.min = toClient(dc, x, y);
	command.max = toClient(dc, x + drawWidth, y + drawHeight);
	command.uvMin = ImVec2(0.0f, 0.0f);
	command.uvMax = ImVec2(1.0f, 1.0f);
	command.image = icon->image;
	record(dc, std::move(command));
	return TRUE;
}
