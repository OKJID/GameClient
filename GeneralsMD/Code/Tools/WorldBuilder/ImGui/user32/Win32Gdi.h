#pragma once

#include "Win32Window.h"

#include "imgui.h"

#include <memory>
#include <string>
#include <vector>

enum class Win32GdiObjectType
{
	Pen,
	Brush,
	Font,
	Bitmap,
	Region,
};

struct Win32Texture
{
	ImTextureID id = ImTextureID_Invalid;
	int width = 0;
	int height = 0;

	~Win32Texture();
};

struct Win32Image
{
	int width = 0;
	int height = 0;
	std::vector<uint32_t> pixels;

	ImTextureID textureForDrawing();
	void invalidateTexture() { m_texture.reset(); }

private:
	std::unique_ptr<Win32Texture> m_texture;
};

typedef std::shared_ptr<Win32Image> Win32ImagePtr;

struct Win32GdiObject
{
	explicit Win32GdiObject(Win32GdiObjectType objectType) : type(objectType) {}
	virtual ~Win32GdiObject() = default;

	Win32GdiObjectType type;
	bool isStock = false;
};

struct Win32Pen : Win32GdiObject
{
	Win32Pen() : Win32GdiObject(Win32GdiObjectType::Pen) {}

	int style = PS_SOLID;
	int width = 1;
	COLORREF color = 0;
};

struct Win32Brush : Win32GdiObject
{
	Win32Brush() : Win32GdiObject(Win32GdiObjectType::Brush) {}

	bool isHollow = false;
	COLORREF color = 0;
};

struct Win32Font : Win32GdiObject
{
	Win32Font() : Win32GdiObject(Win32GdiObjectType::Font) {}

	LOGFONTA logFont = {};

	float pixelHeight() const;
	bool isBold() const { return logFont.lfWeight >= FW_BOLD; }
};

struct Win32Bitmap : Win32GdiObject
{
	Win32Bitmap() : Win32GdiObject(Win32GdiObjectType::Bitmap), image(std::make_shared<Win32Image>()) {}

	Win32ImagePtr image;
};

struct Win32Region : Win32GdiObject
{
	Win32Region() : Win32GdiObject(Win32GdiObjectType::Region) {}

	std::vector<RECT> rects;

	RECT bounds() const;
	int complexity() const;
};

struct Win32Icon
{
	Win32ImagePtr image = std::make_shared<Win32Image>();
	POINT hotSpot = {};
	bool isCursor = false;
	HCURSOR platformCursor = nullptr;
};

enum class Win32DrawKind
{
	Polyline,
	FilledPolygon,
	FilledRect,
	Ellipse,
	Text,
	Image,
};

struct Win32DrawCommand
{
	Win32DrawKind kind = Win32DrawKind::Polyline;
	std::vector<ImVec2> points;
	ImVec2 min;
	ImVec2 max;
	ImU32 color = 0;
	ImU32 fillColor = 0;
	float thickness = 1.0f;
	bool closed = false;
	bool filled = false;
	bool outlined = false;
	std::string text;
	float fontSize = 0.0f;
	bool isBold = false;
	Win32ImagePtr image;
	ImVec2 uvMin;
	ImVec2 uvMax;
	RECT clip = {};
};

class Win32DisplayList
{
public:
	void clear() { m_commands.clear(); }
	void add(Win32DrawCommand command) { m_commands.push_back(std::move(command)); }
	bool isEmpty() const { return m_commands.empty(); }
	void replay(ImDrawList* drawList, ImVec2 origin) const;

private:
	std::vector<Win32DrawCommand> m_commands;
};

struct Win32DeviceContext
{
	Win32Window* window = nullptr;
	Win32Pen* pen = nullptr;
	Win32Brush* brush = nullptr;
	Win32Font* font = nullptr;
	Win32Bitmap* bitmap = nullptr;
	COLORREF textColor = 0;
	COLORREF backgroundColor = 0x00FFFFFF;
	int backgroundMode = OPAQUE;
	int rop2 = R2_COPYPEN;
	int stretchMode = COLORONCOLOR;
	POINT viewportOrigin = {};
	POINT position = {};
	RECT clip = {};
	bool isPaintDC = false;
	bool isWindowDC = false;
	std::vector<Win32DeviceContext> savedStates;
};

namespace Win32Gdi
{
void initialize();
Win32DeviceContext* deviceContext(HDC hdc);
Win32GdiObject* object(HGDIOBJ handle);
HGDIOBJ registerObject(std::unique_ptr<Win32GdiObject> object);
Win32Icon* icon(HICON handle);
HICON registerIcon(std::unique_ptr<Win32Icon> icon);
void destroyIcon(HICON handle);
Win32DisplayList& displayListOf(Win32Window* window);
Win32DisplayList& nonClientDisplayListOf(Win32Window* window);
Win32DisplayList& displayListFor(const Win32DeviceContext* dc);
RECT surfaceBounds(const Win32DeviceContext* dc);
void clearSurface(Win32Window* window);
void retireTexture(ImTextureID id);
void releaseRetiredTextures();
ImU32 toImColor(COLORREF color);
float fontSizeFor(const Win32Font* font);
Win32Brush* brushFromHandle(HBRUSH handle);
Win32ImagePtr imageFromDib(const BITMAPINFO* info, const void* bits);
}
