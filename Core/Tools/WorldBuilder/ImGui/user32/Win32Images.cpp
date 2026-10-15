#include "Win32Gdi.h"
#include "Win32Platform.h"
#include "Win32Resources.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>

namespace
{

#pragma pack(push, 1)
struct IconDirectory
{
	WORD reserved;
	WORD type;
	WORD count;
};

struct IconDirectoryEntry
{
	BYTE width;
	BYTE height;
	BYTE colorCount;
	BYTE reserved;
	WORD planesOrHotSpotX;
	WORD bitCountOrHotSpotY;
	DWORD bytesInResource;
	DWORD imageOffset;
};
#pragma pack(pop)

const WORD kIconType = 1;
const WORD kCursorType = 2;
const uint32_t kTransparent = 0x00000000u;

struct CursorState
{
	HCURSOR current = nullptr;
	int showCount = 0;
	std::map<UINT, HCURSOR> systemCursors;
};

CursorState& cursorState()
{
	static CursorState state;
	return state;
}

bool readBytes(const std::string& path, std::vector<unsigned char>& bytes)
{
	std::ifstream file(Win32NativePath(path), std::ios::binary);
	if (!file)
	{
		return false;
	}
	bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	return true;
}

int paletteSize(const BITMAPINFOHEADER& header)
{
	if (header.biBitCount > 8)
	{
		return 0;
	}
	return header.biClrUsed != 0 ? static_cast<int>(header.biClrUsed) : 1 << header.biBitCount;
}

Win32ImagePtr decodeBitmapFile(const std::vector<unsigned char>& bytes)
{
	if (bytes.size() < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) || bytes[0] != 'B' || bytes[1] != 'M')
	{
		return nullptr;
	}

	BITMAPFILEHEADER fileHeader;
	memcpy(&fileHeader, bytes.data(), sizeof(fileHeader));
	const BITMAPINFO* info = reinterpret_cast<const BITMAPINFO*>(bytes.data() + sizeof(BITMAPFILEHEADER));
	if (fileHeader.bfOffBits >= bytes.size())
	{
		return nullptr;
	}
	return Win32Gdi::imageFromDib(info, bytes.data() + fileHeader.bfOffBits);
}

void applyAndMask(Win32Image& image, const unsigned char* mask)
{
	const int stride = ((image.width + 31) / 32) * 4;
	for (int y = 0; y < image.height; ++y)
	{
		const unsigned char* row = mask + stride * (image.height - 1 - y);
		for (int x = 0; x < image.width; ++x)
		{
			const bool isTransparent = (row[x / 8] & (0x80 >> (x % 8))) != 0;
			uint32_t& pixel = image.pixels[static_cast<size_t>(y * image.width + x)];
			const bool isInverted = isTransparent && (pixel & 0x00FFFFFFu) != 0;
			pixel = isInverted ? 0xFF000000u : (isTransparent ? kTransparent : pixel);
		}
	}
}

void applyAlphaChannel(Win32Image& image, const unsigned char* bits)
{
	bool hasAlpha = false;
	for (int y = 0; y < image.height && !hasAlpha; ++y)
	{
		for (int x = 0; x < image.width && !hasAlpha; ++x)
		{
			hasAlpha = bits[(y * image.width + x) * 4 + 3] != 0;
		}
	}
	if (!hasAlpha)
	{
		return;
	}

	for (int y = 0; y < image.height; ++y)
	{
		for (int x = 0; x < image.width; ++x)
		{
			const uint32_t alpha = bits[((image.height - 1 - y) * image.width + x) * 4 + 3];
			uint32_t& pixel = image.pixels[static_cast<size_t>(y * image.width + x)];
			pixel = (pixel & 0x00FFFFFFu) | (alpha << 24);
		}
	}
}

Win32ImagePtr decodeIconImage(const unsigned char* data, size_t size)
{
	if (size < sizeof(BITMAPINFOHEADER))
	{
		return nullptr;
	}

	const size_t headerCapacity = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256;
	std::vector<unsigned char> header(headerCapacity, 0);
	memcpy(header.data(), data, std::min(size, headerCapacity));
	BITMAPINFO* info = reinterpret_cast<BITMAPINFO*>(header.data());
	info->bmiHeader.biHeight /= 2;
	const BITMAPINFOHEADER& bitmapHeader = info->bmiHeader;
	const size_t colorOffset = bitmapHeader.biSize + sizeof(RGBQUAD) * static_cast<size_t>(paletteSize(bitmapHeader));
	const unsigned char* colorBits = data + colorOffset;
	const size_t colorStride = static_cast<size_t>(((bitmapHeader.biWidth * bitmapHeader.biBitCount + 31) / 32) * 4);
	const unsigned char* maskBits = colorBits + colorStride * static_cast<size_t>(bitmapHeader.biHeight);

	Win32ImagePtr image = Win32Gdi::imageFromDib(info, colorBits);
	applyAndMask(*image, maskBits);
	if (bitmapHeader.biBitCount == 32)
	{
		applyAlphaChannel(*image, colorBits);
	}
	return image;
}

const IconDirectoryEntry* chooseEntry(const std::vector<unsigned char>& bytes, const IconDirectory& directory, int desiredSize)
{
	const IconDirectoryEntry* entries = reinterpret_cast<const IconDirectoryEntry*>(bytes.data() + sizeof(IconDirectory));
	const IconDirectoryEntry* best = nullptr;
	for (int index = 0; index < directory.count; ++index)
	{
		const IconDirectoryEntry& entry = entries[index];
		const int size = entry.width == 0 ? 256 : entry.width;
		const int bestSize = best == nullptr ? -1 : (best->width == 0 ? 256 : best->width);
		const bool closer = best == nullptr || std::abs(size - desiredSize) < std::abs(bestSize - desiredSize);
		const bool sameSizeRicher = best != nullptr && size == bestSize && entry.bitCountOrHotSpotY > best->bitCountOrHotSpotY;
		if (closer || sameSizeRicher)
		{
			best = &entry;
		}
	}
	return best;
}

std::unique_ptr<Win32Icon> decodeIconFile(const std::vector<unsigned char>& bytes, int desiredSize)
{
	if (bytes.size() < sizeof(IconDirectory))
	{
		return nullptr;
	}
	IconDirectory directory;
	memcpy(&directory, bytes.data(), sizeof(directory));
	if ((directory.type != kIconType && directory.type != kCursorType) || directory.count == 0)
	{
		return nullptr;
	}

	const IconDirectoryEntry* entry = chooseEntry(bytes, directory, desiredSize);
	if (entry == nullptr || entry->imageOffset + entry->bytesInResource > bytes.size())
	{
		return nullptr;
	}

	auto icon = std::make_unique<Win32Icon>();
	icon->isCursor = directory.type == kCursorType;
	icon->hotSpot.x = icon->isCursor ? entry->planesOrHotSpotX : 0;
	icon->hotSpot.y = icon->isCursor ? entry->bitCountOrHotSpotY : 0;
	icon->image = decodeIconImage(bytes.data() + entry->imageOffset, entry->bytesInResource);
	return icon->image != nullptr ? std::move(icon) : nullptr;
}

void mapColor(Win32Image& image, uint32_t from, uint32_t to)
{
	for (uint32_t& pixel : image.pixels)
	{
		pixel = (pixel & 0x00FFFFFFu) == (from & 0x00FFFFFFu) ? to : pixel;
	}
}

uint32_t systemColorPixel(int index)
{
	const COLORREF color = GetSysColor(index);
	return 0xFF000000u | (GetRValue(color) << 16) | (GetGValue(color) << 8) | GetBValue(color);
}

void applyLoadFlags(Win32Image& image, UINT flags)
{
	if ((flags & LR_LOADTRANSPARENT) != 0 && !image.pixels.empty())
	{
		mapColor(image, image.pixels.front(), systemColorPixel(COLOR_WINDOW));
	}
	if ((flags & LR_LOADMAP3DCOLORS) != 0)
	{
		mapColor(image, 0xFF808080u, systemColorPixel(COLOR_3DSHADOW));
		mapColor(image, 0xFFC0C0C0u, systemColorPixel(COLOR_3DFACE));
		mapColor(image, 0xFFDFDFDFu, systemColorPixel(COLOR_3DLIGHT));
	}
}

bool resolveImagePath(LPCSTR name, UINT type, UINT flags, std::string& path)
{
	if ((flags & LR_LOADFROMFILE) != 0 && name != nullptr && !IS_INTRESOURCE(name))
	{
		path = name;
		return true;
	}

	UINT id = 0;
	if (!Win32Resources::instance().resolveName(name, id))
	{
		return false;
	}
	const std::string* file = type == IMAGE_BITMAP ? Win32Resources::instance().bitmapFile(id)
		: type == IMAGE_ICON ? Win32Resources::instance().iconFile(id) : Win32Resources::instance().cursorFile(id);
	if (file == nullptr)
	{
		return false;
	}
	path = Win32Resources::instance().filePath(*file);
	return true;
}

HANDLE loadBitmap(const std::string& path, UINT flags)
{
	std::vector<unsigned char> bytes;
	Win32ImagePtr image = readBytes(path, bytes) ? decodeBitmapFile(bytes) : nullptr;
	if (image == nullptr)
	{
		return nullptr;
	}
	applyLoadFlags(*image, flags);
	auto bitmap = std::make_unique<Win32Bitmap>();
	bitmap->image = image;
	return Win32Gdi::registerObject(std::move(bitmap));
}

HANDLE loadIconOrCursor(const std::string& path, int desiredSize)
{
	std::vector<unsigned char> bytes;
	std::unique_ptr<Win32Icon> icon = readBytes(path, bytes) ? decodeIconFile(bytes, desiredSize) : nullptr;
	if (icon == nullptr)
	{
		return nullptr;
	}
	Win32Platform* platform = Win32GetPlatform();
	if (icon->isCursor && platform != nullptr)
	{
		icon->platformCursor = platform->createCursor(*icon->image, icon->hotSpot);
	}
	return Win32Gdi::registerIcon(std::move(icon));
}

HCURSOR systemCursor(UINT id)
{
	HCURSOR& cursor = cursorState().systemCursors[id];
	Win32Platform* platform = Win32GetPlatform();
	if (cursor != nullptr || platform == nullptr)
	{
		return cursor;
	}

	auto icon = std::make_unique<Win32Icon>();
	icon->isCursor = true;
	icon->platformCursor = platform->systemCursor(id);
	cursor = Win32Gdi::registerIcon(std::move(icon));
	return cursor;
}

}

HANDLE LoadImageA(HINSTANCE, LPCSTR name, UINT type, int width, int, UINT flags)
{
	std::string path;
	if (!resolveImagePath(name, type, flags, path))
	{
		return nullptr;
	}
	if (type == IMAGE_BITMAP)
	{
		return loadBitmap(path, flags);
	}
	return loadIconOrCursor(path, width > 0 ? width : 32);
}

HICON LoadIconA(HINSTANCE instance, LPCSTR iconName)
{
	if (instance == nullptr)
	{
		return nullptr;
	}
	return static_cast<HICON>(LoadImageA(instance, iconName, IMAGE_ICON, 32, 32, LR_DEFAULTSIZE));
}

HCURSOR LoadCursorA(HINSTANCE instance, LPCSTR cursorName)
{
	if (instance == nullptr && IS_INTRESOURCE(cursorName))
	{
		return systemCursor(static_cast<UINT>(reinterpret_cast<uintptr_t>(cursorName)));
	}
	return static_cast<HCURSOR>(LoadImageA(instance, cursorName, IMAGE_CURSOR, 32, 32, LR_DEFAULTSIZE));
}

HCURSOR SetCursor(HCURSOR cursor)
{
	const HCURSOR previous = cursorState().current;
	cursorState().current = cursor;
	Win32Icon* icon = Win32Gdi::icon(cursor);
	Win32Platform* platform = Win32GetPlatform();
	if (platform != nullptr && icon != nullptr && icon->platformCursor != nullptr)
	{
		platform->setCursor(icon->platformCursor);
	}
	return previous;
}

HCURSOR GetCursor()
{
	return cursorState().current;
}

int ShowCursor(BOOL show)
{
	CursorState& state = cursorState();
	state.showCount += show ? 1 : -1;
	Win32Platform* platform = Win32GetPlatform();
	if (platform != nullptr)
	{
		platform->setCursorVisible(state.showCount >= 0);
	}
	return state.showCount;
}

BOOL DestroyCursor(HCURSOR cursor)
{
	Win32Icon* icon = Win32Gdi::icon(cursor);
	if (icon == nullptr)
	{
		return FALSE;
	}
	for (const auto& entry : cursorState().systemCursors)
	{
		if (entry.second == cursor)
		{
			return TRUE;
		}
	}

	Win32Platform* platform = Win32GetPlatform();
	if (platform != nullptr && icon->platformCursor != nullptr)
	{
		platform->destroyCursor(icon->platformCursor);
	}
	if (cursorState().current == cursor)
	{
		cursorState().current = nullptr;
	}
	Win32Gdi::destroyIcon(cursor);
	return TRUE;
}

BOOL DestroyIcon(HICON icon)
{
	if (Win32Gdi::icon(icon) == nullptr)
	{
		return FALSE;
	}
	Win32Gdi::destroyIcon(icon);
	return TRUE;
}
