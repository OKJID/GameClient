#pragma once

#include "Win32Gdi.h"

struct Win32ImageListData
{
	int width = 0;
	int height = 0;
	UINT flags = 0;
	COLORREF backgroundColor = CLR_NONE;
	std::vector<Win32ImagePtr> images;
};

namespace Win32ImageLists
{
Win32ImageListData* find(HIMAGELIST imageList);
Win32ImagePtr image(HIMAGELIST imageList, int index);
}
