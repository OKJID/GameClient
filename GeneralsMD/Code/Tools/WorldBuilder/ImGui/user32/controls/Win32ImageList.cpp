#include "Win32ImageList.h"

#include <map>

namespace
{

struct ImageListState
{
	std::map<HIMAGELIST, std::unique_ptr<Win32ImageListData>> lists;
	uintptr_t nextHandle = 0x50010;
};

ImageListState& state()
{
	static ImageListState imageListState;
	return imageListState;
}

Win32ImagePtr bitmapImage(HBITMAP bitmap)
{
	Win32GdiObject* object = Win32Gdi::object(bitmap);
	return object != nullptr && object->type == Win32GdiObjectType::Bitmap ? static_cast<Win32Bitmap*>(object)->image : nullptr;
}

Win32ImagePtr cutImage(const Win32Image& source, int left, int width, int height)
{
	auto image = std::make_shared<Win32Image>();
	image->width = width;
	image->height = height;
	image->pixels.assign(static_cast<size_t>(width * height), 0);
	for (int y = 0; y < std::min(height, source.height); ++y)
	{
		for (int x = 0; x < width && left + x < source.width; ++x)
		{
			image->pixels[static_cast<size_t>(y * width + x)] = source.pixels[static_cast<size_t>(y * source.width + left + x)];
		}
	}
	return image;
}

void applyMaskImage(Win32Image& image, const Win32Image& mask, int left)
{
	for (int y = 0; y < std::min(image.height, mask.height); ++y)
	{
		for (int x = 0; x < image.width && left + x < mask.width; ++x)
		{
			const bool isTransparent = (mask.pixels[static_cast<size_t>(y * mask.width + left + x)] & 0x00FFFFFFu) != 0;
			uint32_t& pixel = image.pixels[static_cast<size_t>(y * image.width + x)];
			pixel = isTransparent ? 0 : pixel;
		}
	}
}

void applyMaskColor(Win32Image& image, COLORREF maskColor)
{
	const uint32_t mask = (GetRValue(maskColor) << 16) | (GetGValue(maskColor) << 8) | GetBValue(maskColor);
	for (uint32_t& pixel : image.pixels)
	{
		pixel = (pixel & 0x00FFFFFFu) == mask ? 0 : pixel;
	}
}

int addStrip(Win32ImageListData& list, const Win32Image& strip, const Win32Image* mask, COLORREF maskColor)
{
	const int first = static_cast<int>(list.images.size());
	for (int left = 0; left + list.width <= strip.width; left += list.width)
	{
		Win32ImagePtr image = cutImage(strip, left, list.width, list.height);
		if (mask != nullptr)
		{
			applyMaskImage(*image, *mask, left);
		}
		if (maskColor != CLR_NONE)
		{
			applyMaskColor(*image, maskColor);
		}
		list.images.push_back(image);
	}
	return static_cast<int>(list.images.size()) > first ? first : -1;
}

}

namespace Win32ImageLists
{

Win32ImageListData* find(HIMAGELIST imageList)
{
	const auto it = state().lists.find(imageList);
	return it == state().lists.end() ? nullptr : it->second.get();
}

Win32ImagePtr image(HIMAGELIST imageList, int index)
{
	Win32ImageListData* list = find(imageList);
	if (list == nullptr || index < 0 || index >= static_cast<int>(list->images.size()))
	{
		return nullptr;
	}
	return list->images[static_cast<size_t>(index)];
}

}

HIMAGELIST ImageList_Create(int width, int height, UINT flags, int, int)
{
	const HIMAGELIST handle = reinterpret_cast<HIMAGELIST>(state().nextHandle);
	state().nextHandle += 0x10;
	auto list = std::make_unique<Win32ImageListData>();
	list->width = width;
	list->height = height;
	list->flags = flags;
	state().lists[handle] = std::move(list);
	return handle;
}

BOOL ImageList_Destroy(HIMAGELIST imageList)
{
	return state().lists.erase(imageList) != 0 ? TRUE : FALSE;
}

int ImageList_GetImageCount(HIMAGELIST imageList)
{
	Win32ImageListData* list = Win32ImageLists::find(imageList);
	return list != nullptr ? static_cast<int>(list->images.size()) : 0;
}

BOOL ImageList_SetImageCount(HIMAGELIST imageList, UINT count)
{
	Win32ImageListData* list = Win32ImageLists::find(imageList);
	if (list == nullptr)
	{
		return FALSE;
	}
	list->images.resize(count);
	return TRUE;
}

int ImageList_Add(HIMAGELIST imageList, HBITMAP image, HBITMAP mask)
{
	Win32ImageListData* list = Win32ImageLists::find(imageList);
	Win32ImagePtr strip = bitmapImage(image);
	if (list == nullptr || strip == nullptr)
	{
		return -1;
	}
	Win32ImagePtr maskImage = (list->flags & ILC_MASK) != 0 ? bitmapImage(mask) : nullptr;
	return addStrip(*list, *strip, maskImage.get(), CLR_NONE);
}

int ImageList_AddMasked(HIMAGELIST imageList, HBITMAP image, COLORREF maskColor)
{
	Win32ImageListData* list = Win32ImageLists::find(imageList);
	Win32ImagePtr strip = bitmapImage(image);
	if (list == nullptr || strip == nullptr)
	{
		return -1;
	}
	return addStrip(*list, *strip, nullptr, maskColor);
}

int ImageList_ReplaceIcon(HIMAGELIST imageList, int index, HICON iconHandle)
{
	Win32ImageListData* list = Win32ImageLists::find(imageList);
	Win32Icon* icon = Win32Gdi::icon(iconHandle);
	if (list == nullptr || icon == nullptr)
	{
		return -1;
	}
	if (index < 0 || index >= static_cast<int>(list->images.size()))
	{
		list->images.push_back(icon->image);
		return static_cast<int>(list->images.size()) - 1;
	}
	list->images[static_cast<size_t>(index)] = icon->image;
	return index;
}

BOOL ImageList_Remove(HIMAGELIST imageList, int index)
{
	Win32ImageListData* list = Win32ImageLists::find(imageList);
	if (list == nullptr)
	{
		return FALSE;
	}
	if (index == -1)
	{
		list->images.clear();
		return TRUE;
	}
	if (index < 0 || index >= static_cast<int>(list->images.size()))
	{
		return FALSE;
	}
	list->images.erase(list->images.begin() + index);
	return TRUE;
}

BOOL ImageList_GetIconSize(HIMAGELIST imageList, int* width, int* height)
{
	Win32ImageListData* list = Win32ImageLists::find(imageList);
	if (list == nullptr)
	{
		return FALSE;
	}
	if (width != nullptr)
	{
		*width = list->width;
	}
	if (height != nullptr)
	{
		*height = list->height;
	}
	return TRUE;
}

COLORREF ImageList_SetBkColor(HIMAGELIST imageList, COLORREF color)
{
	Win32ImageListData* list = Win32ImageLists::find(imageList);
	return list != nullptr ? std::exchange(list->backgroundColor, color) : CLR_NONE;
}

BOOL ImageList_Draw(HIMAGELIST imageList, int index, HDC hdc, int x, int y, UINT)
{
	Win32ImagePtr image = Win32ImageLists::image(imageList, index);
	Win32DeviceContext* dc = Win32Gdi::deviceContext(hdc);
	if (image == nullptr || dc == nullptr || dc->window == nullptr)
	{
		return FALSE;
	}

	Win32DrawCommand command;
	command.kind = Win32DrawKind::Image;
	command.clip = dc->clip;
	command.min = ImVec2(static_cast<float>(x + dc->viewportOrigin.x), static_cast<float>(y + dc->viewportOrigin.y));
	command.max = ImVec2(command.min.x + image->width, command.min.y + image->height);
	command.uvMin = ImVec2(0.0f, 0.0f);
	command.uvMax = ImVec2(1.0f, 1.0f);
	command.image = image;
	Win32Gdi::displayListFor(dc).add(std::move(command));
	return TRUE;
}

HIMAGELIST ImageList_LoadImageA(HINSTANCE instance, LPCSTR name, int width, int grow, COLORREF maskColor, UINT type, UINT flags)
{
	if (type != IMAGE_BITMAP)
	{
		return nullptr;
	}
	const HBITMAP bitmap = static_cast<HBITMAP>(LoadImageA(instance, name, IMAGE_BITMAP, 0, 0, flags));
	Win32ImagePtr strip = bitmapImage(bitmap);
	if (strip == nullptr)
	{
		return nullptr;
	}

	const HIMAGELIST imageList = ImageList_Create(width, strip->height, maskColor != CLR_NONE ? ILC_MASK : ILC_COLOR, 0, grow);
	ImageList_AddMasked(imageList, bitmap, maskColor);
	DeleteObject(bitmap);
	return imageList;
}
