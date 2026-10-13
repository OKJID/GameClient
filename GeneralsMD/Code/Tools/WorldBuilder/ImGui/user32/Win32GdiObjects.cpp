#include "Win32Gdi.h"

#include "Win32Metrics.h"
#include "imgui_impl_dx8wrapper.h"

#include "MacOSGdiBackend.h"

#include <algorithm>
#include <cstring>
#include <set>
#include <utility>

namespace
{

const size_t kSystemFontSlot = NULL_PEN + 1;

struct GdiState
{
	std::set<Win32GdiObject*> objects;
	std::set<Win32DeviceContext*> deviceContexts;
	std::map<HICON, std::unique_ptr<Win32Icon>> icons;
	std::vector<std::unique_ptr<Win32GdiObject>> stockObjects;
	std::map<int, HBRUSH> systemColorBrushes;
	std::vector<ImTextureID> retiredTextures;
	std::vector<ImTextureID> texturesToRelease;
	uintptr_t nextIconHandle = 0x20010;
};

GdiState& state()
{
	static GdiState gdiState;
	return gdiState;
}

template<class Object>
HGDIOBJ registerObject(std::unique_ptr<Object> object)
{
	return Win32Gdi::registerObject(std::unique_ptr<Win32GdiObject>(object.release()));
}

Win32GdiObject* addStockObject(std::unique_ptr<Win32GdiObject> object)
{
	object->isStock = true;
	Win32GdiObject* raw = object.get();
	state().objects.insert(raw);
	state().stockObjects.push_back(std::move(object));
	return raw;
}

std::unique_ptr<Win32Brush> makeBrush(COLORREF color, bool isHollow)
{
	auto brush = std::make_unique<Win32Brush>();
	brush->color = color;
	brush->isHollow = isHollow;
	return brush;
}

std::unique_ptr<Win32Pen> makePen(int style, int width, COLORREF color)
{
	auto pen = std::make_unique<Win32Pen>();
	pen->style = style;
	pen->width = width;
	pen->color = color;
	return pen;
}

std::unique_ptr<Win32Font> makeFont(LONG height, const char* faceName)
{
	auto font = std::make_unique<Win32Font>();
	font->logFont.lfHeight = height;
	font->logFont.lfWeight = FW_NORMAL;
	strlcpy(font->logFont.lfFaceName, faceName, sizeof(font->logFont.lfFaceName));
	return font;
}

Win32GdiObject* stockObject(int index)
{
	if (state().stockObjects.empty())
	{
		addStockObject(makeBrush(RGB(255, 255, 255), false));
		addStockObject(makeBrush(RGB(192, 192, 192), false));
		addStockObject(makeBrush(RGB(128, 128, 128), false));
		addStockObject(makeBrush(RGB(64, 64, 64), false));
		addStockObject(makeBrush(RGB(0, 0, 0), false));
		addStockObject(makeBrush(0, true));
		addStockObject(makePen(PS_SOLID, 1, RGB(255, 255, 255)));
		addStockObject(makePen(PS_SOLID, 1, RGB(0, 0, 0)));
		addStockObject(makePen(PS_NULL, 1, 0));
		addStockObject(makeFont(0, "System"));
	}

	if (index >= WHITE_BRUSH && index <= NULL_PEN)
	{
		return state().stockObjects[static_cast<size_t>(index)].get();
	}
	return state().stockObjects[kSystemFontSlot].get();
}

void resetDeviceContext(Win32DeviceContext& dc, Win32Window* window, bool isWindowDC)
{
	dc = Win32DeviceContext();
	dc.window = window;
	dc.isWindowDC = isWindowDC;
	dc.pen = static_cast<Win32Pen*>(stockObject(BLACK_PEN));
	dc.brush = static_cast<Win32Brush*>(stockObject(WHITE_BRUSH));
	dc.font = static_cast<Win32Font*>(stockObject(SYSTEM_FONT));
	dc.clip = Win32Gdi::surfaceBounds(&dc);
}

HDC newDeviceContext(Win32Window* window, bool isWindowDC = false)
{
	auto* dc = new Win32DeviceContext();
	resetDeviceContext(*dc, window, isWindowDC);
	state().deviceContexts.insert(dc);
	return reinterpret_cast<HDC>(dc);
}

void releaseDeviceContext(HDC hdc)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr)
	{
		return;
	}
	state().deviceContexts.erase(dc);
	delete dc;
}

HGDIOBJ selectIntoContext(Win32DeviceContext* dc, Win32GdiObject* object)
{
	switch (object->type)
	{
	case Win32GdiObjectType::Pen:
		return static_cast<HGDIOBJ>(static_cast<Win32GdiObject*>(std::exchange(dc->pen, static_cast<Win32Pen*>(object))));
	case Win32GdiObjectType::Brush:
		return static_cast<HGDIOBJ>(static_cast<Win32GdiObject*>(std::exchange(dc->brush, static_cast<Win32Brush*>(object))));
	case Win32GdiObjectType::Font:
		return static_cast<HGDIOBJ>(static_cast<Win32GdiObject*>(std::exchange(dc->font, static_cast<Win32Font*>(object))));
	case Win32GdiObjectType::Bitmap:
		return static_cast<HGDIOBJ>(static_cast<Win32GdiObject*>(std::exchange(dc->bitmap, static_cast<Win32Bitmap*>(object))));
	case Win32GdiObjectType::Region:
		SelectClipRgn(reinterpret_cast<HDC>(dc), static_cast<HRGN>(static_cast<HGDIOBJ>(object)));
		return reinterpret_cast<HGDIOBJ>(static_cast<uintptr_t>(SIMPLEREGION));
	}
	return nullptr;
}

bool isSelectedAnywhere(const Win32GdiObject* object)
{
	for (const Win32DeviceContext* dc : state().deviceContexts)
	{
		if (dc->pen == object || dc->brush == object || dc->font == object || dc->bitmap == object)
		{
			return true;
		}
	}
	return false;
}

class Win32GdiBackend : public MacOSGdiBackend
{
public:
	HDC getDC(HWND hwnd) override
	{
		Win32Window* window = Win32FindWindow(hwnd);
		return window != nullptr ? newDeviceContext(window) : nullptr;
	}

	int releaseDC(HWND, HDC hdc) override
	{
		releaseDeviceContext(hdc);
		return 1;
	}

	HGDIOBJ selectObject(HDC hdc, HGDIOBJ handle) override
	{
		Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
		Win32GdiObject* object = Win32Gdi::object(handle);
		return dc != nullptr && object != nullptr ? selectIntoContext(dc, object) : nullptr;
	}

	BOOL deleteObject(HGDIOBJ handle) override
	{
		Win32GdiObject* object = Win32Gdi::object(handle);
		if (object == nullptr || object->isStock || isSelectedAnywhere(object))
		{
			return FALSE;
		}
		state().objects.erase(object);
		delete object;
		return TRUE;
	}

	COLORREF setBkColor(HDC hdc, COLORREF color) override
	{
		Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
		return dc != nullptr ? std::exchange(dc->backgroundColor, color) : CLR_INVALID;
	}

	COLORREF setTextColor(HDC hdc, COLORREF color) override
	{
		Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
		return dc != nullptr ? std::exchange(dc->textColor, color) : CLR_INVALID;
	}

	int setBkMode(HDC hdc, int mode) override
	{
		Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
		return dc != nullptr ? std::exchange(dc->backgroundMode, mode) : 0;
	}
};

}

Win32Texture::~Win32Texture()
{
	Win32Gdi::retireTexture(id);
}

ImTextureID Win32Image::textureForDrawing()
{
	if (m_texture != nullptr)
	{
		return m_texture->id;
	}
	if (width <= 0 || height <= 0)
	{
		return ImTextureID_Invalid;
	}

	m_texture = std::make_unique<Win32Texture>();
	m_texture->width = width;
	m_texture->height = height;
	m_texture->id = ImGui_ImplDX8Wrapper_CreateTexture(width, height, pixels.data(), width * 4);
	return m_texture->id;
}

float Win32Font::pixelHeight() const
{
	return static_cast<float>(logFont.lfHeight < 0 ? -logFont.lfHeight : logFont.lfHeight);
}

RECT Win32Region::bounds() const
{
	RECT result = {};
	for (const RECT& rect : rects)
	{
		UnionRect(&result, &result, &rect);
	}
	return result;
}

int Win32Region::complexity() const
{
	if (rects.empty())
	{
		return NULLREGION;
	}
	return rects.size() == 1 ? SIMPLEREGION : COMPLEXREGION;
}

namespace Win32Gdi
{

void initialize()
{
	static Win32GdiBackend backend;
	MacOS_SetGdiBackend(&backend);
}

Win32DeviceContext* deviceContext(HDC hdc)
{
	Win32DeviceContext* dc = reinterpret_cast<Win32DeviceContext*>(hdc);
	return state().deviceContexts.count(dc) != 0 ? dc : nullptr;
}

Win32GdiObject* object(HGDIOBJ handle)
{
	Win32GdiObject* object = static_cast<Win32GdiObject*>(handle);
	return state().objects.count(object) != 0 ? object : nullptr;
}

HGDIOBJ registerObject(std::unique_ptr<Win32GdiObject> object)
{
	Win32GdiObject* raw = object.release();
	state().objects.insert(raw);
	return static_cast<HGDIOBJ>(raw);
}

Win32Icon* icon(HICON handle)
{
	const auto it = state().icons.find(handle);
	return it == state().icons.end() ? nullptr : it->second.get();
}

HICON registerIcon(std::unique_ptr<Win32Icon> icon)
{
	const HICON handle = reinterpret_cast<HICON>(state().nextIconHandle);
	state().nextIconHandle += 0x10;
	state().icons[handle] = std::move(icon);
	return handle;
}

void destroyIcon(HICON handle)
{
	state().icons.erase(handle);
}

Win32DisplayList& displayListOf(Win32Window* window)
{
	if (window->displayList == nullptr)
	{
		window->displayList = std::make_unique<Win32DisplayList>();
	}
	return *window->displayList;
}

Win32DisplayList& nonClientDisplayListOf(Win32Window* window)
{
	if (window->nonClientDisplayList == nullptr)
	{
		window->nonClientDisplayList = std::make_unique<Win32DisplayList>();
	}
	return *window->nonClientDisplayList;
}

Win32DisplayList& displayListFor(const Win32DeviceContext* dc)
{
	return dc->isWindowDC ? nonClientDisplayListOf(dc->window) : displayListOf(dc->window);
}

RECT surfaceBounds(const Win32DeviceContext* dc)
{
	RECT bounds = {};
	if (dc->window == nullptr)
	{
		return bounds;
	}
	if (dc->isWindowDC)
	{
		SetRect(&bounds, 0, 0, dc->window->width(), dc->window->height());
		return bounds;
	}
	GetClientRect(dc->window->hwnd, &bounds);
	return bounds;
}

void clearSurface(Win32Window* window)
{
	displayListOf(window).clear();
}

void retireTexture(ImTextureID id)
{
	if (id != ImTextureID_Invalid)
	{
		state().retiredTextures.push_back(id);
	}
}

void releaseRetiredTextures()
{
	for (ImTextureID id : state().texturesToRelease)
	{
		ImGui_ImplDX8Wrapper_DestroyTexture(id);
	}
	state().texturesToRelease.swap(state().retiredTextures);
	state().retiredTextures.clear();
}

ImU32 toImColor(COLORREF color)
{
	return IM_COL32(GetRValue(color), GetGValue(color), GetBValue(color), 255);
}

}

HPEN CreatePen(int style, int width, COLORREF color)
{
	return registerObject(makePen(style, width, color));
}

HPEN CreatePenIndirect(const LOGPEN* pen)
{
	return pen != nullptr ? CreatePen(static_cast<int>(pen->lopnStyle), pen->lopnWidth.x, pen->lopnColor) : nullptr;
}

HBRUSH CreateSolidBrush(COLORREF color)
{
	return registerObject(makeBrush(color, false));
}

HBRUSH CreateBrushIndirect(const LOGBRUSH* brush)
{
	if (brush == nullptr)
	{
		return nullptr;
	}
	return registerObject(makeBrush(brush->lbColor, brush->lbStyle == BS_NULL));
}

HFONT CreateFontIndirectA(const LOGFONTA* logFont)
{
	if (logFont == nullptr)
	{
		return nullptr;
	}
	auto font = std::make_unique<Win32Font>();
	font->logFont = *logFont;
	return registerObject(std::move(font));
}

HGDIOBJ GetStockObject(int index)
{
	return static_cast<HGDIOBJ>(stockObject(index));
}

HBRUSH GetSysColorBrush(int index)
{
	HBRUSH& brush = state().systemColorBrushes[index];
	if (brush == nullptr)
	{
		brush = static_cast<HBRUSH>(static_cast<HGDIOBJ>(addStockObject(makeBrush(GetSysColor(index), false))));
	}
	return brush;
}

HBITMAP CreateBitmap(int width, int height, UINT, UINT bitsPerPixel, const void* bits)
{
	auto bitmap = std::make_unique<Win32Bitmap>();
	Win32Image& image = *bitmap->image;
	image.width = width;
	image.height = height;
	image.pixels.assign(static_cast<size_t>(std::max(0, width * height)), 0xFF000000u);
	if (bits != nullptr && bitsPerPixel == 32)
	{
		memcpy(image.pixels.data(), bits, image.pixels.size() * sizeof(uint32_t));
	}
	return registerObject(std::move(bitmap));
}

int GetObjectA(HGDIOBJ handle, int size, void* buffer)
{
	Win32GdiObject* object = Win32Gdi::object(handle);
	if (object == nullptr || object->type != Win32GdiObjectType::Bitmap || buffer == nullptr || size < static_cast<int>(sizeof(BITMAP)))
	{
		return 0;
	}

	const Win32Image& image = *static_cast<Win32Bitmap*>(object)->image;
	BITMAP* bitmap = static_cast<BITMAP*>(buffer);
	*bitmap = {};
	bitmap->bmWidth = image.width;
	bitmap->bmHeight = image.height;
	bitmap->bmWidthBytes = image.width * 4;
	bitmap->bmPlanes = 1;
	bitmap->bmBitsPixel = 32;
	return sizeof(BITMAP);
}

HDC GetWindowDC(HWND hwnd)
{
	Win32Window* window = Win32FindWindow(hwnd);
	return window != nullptr ? newDeviceContext(window, true) : nullptr;
}

static void paintNonClientArea(Win32Window* window)
{
	if (!window->needsNonClientPaint)
	{
		return;
	}
	window->needsNonClientPaint = false;
	Win32Gdi::nonClientDisplayListOf(window).clear();
	SendMessageA(window->hwnd, WM_NCPAINT, 1, 0);
}

HDC BeginPaint(HWND hwnd, LPPAINTSTRUCT paint)
{
	Win32Window* window = Win32FindWindow(hwnd);
	if (window == nullptr || paint == nullptr)
	{
		return nullptr;
	}

	paintNonClientArea(window);
	const HDC hdc = newDeviceContext(window);
	Win32Gdi::deviceContext(hdc)->isPaintDC = true;
	Win32Gdi::clearSurface(window);
	*paint = {};
	paint->hdc = hdc;
	GetClientRect(hwnd, &paint->rcPaint);
	if (window->needsErase)
	{
		window->needsErase = false;
		paint->fErase = SendMessageA(hwnd, WM_ERASEBKGND, reinterpret_cast<WPARAM>(hdc), 0) == 0 ? TRUE : FALSE;
	}
	ValidateRect(hwnd, nullptr);
	return hdc;
}

BOOL EndPaint(HWND, const PAINTSTRUCT* paint)
{
	if (paint == nullptr)
	{
		return FALSE;
	}
	releaseDeviceContext(paint->hdc);
	return TRUE;
}

int SaveDC(HDC hdc)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr)
	{
		return 0;
	}
	Win32DeviceContext snapshot = *dc;
	snapshot.savedStates.clear();
	dc->savedStates.push_back(snapshot);
	return static_cast<int>(dc->savedStates.size());
}

BOOL RestoreDC(HDC hdc, int savedDC)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr || dc->savedStates.empty())
	{
		return FALSE;
	}

	const size_t index = savedDC < 0 ? dc->savedStates.size() + static_cast<size_t>(savedDC) : static_cast<size_t>(savedDC) - 1;
	if (index >= dc->savedStates.size())
	{
		return FALSE;
	}

	std::vector<Win32DeviceContext> remaining(dc->savedStates.begin(), dc->savedStates.begin() + static_cast<long>(index));
	*dc = dc->savedStates[index];
	dc->savedStates = std::move(remaining);
	return TRUE;
}

int SetROP2(HDC hdc, int mode)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	return dc != nullptr ? std::exchange(dc->rop2, mode) : 0;
}

int SetStretchBltMode(HDC hdc, int mode)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	return dc != nullptr ? std::exchange(dc->stretchMode, mode) : 0;
}

BOOL SetViewportOrgEx(HDC hdc, int x, int y, LPPOINT previous)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr)
	{
		return FALSE;
	}
	if (previous != nullptr)
	{
		*previous = dc->viewportOrigin;
	}
	dc->viewportOrigin.x = x;
	dc->viewportOrigin.y = y;
	return TRUE;
}

BOOL GetViewportOrgEx(HDC hdc, LPPOINT origin)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr || origin == nullptr)
	{
		return FALSE;
	}
	*origin = dc->viewportOrigin;
	return TRUE;
}

BOOL OffsetViewportOrgEx(HDC hdc, int dx, int dy, LPPOINT previous)
{
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (dc == nullptr)
	{
		return FALSE;
	}
	return SetViewportOrgEx(hdc, dc->viewportOrigin.x + dx, dc->viewportOrigin.y + dy, previous);
}
