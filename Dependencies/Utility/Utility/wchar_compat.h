/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 TheSuperHackers
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// This file contains WCHAR and related macros for compatibility with non-windows platforms.
#pragma once

// WCHAR
typedef wchar_t WCHAR;
typedef const WCHAR* LPCWSTR;
typedef WCHAR* LPWSTR;

#define _wcsicmp wcscasecmp
#define wcsicmp wcscasecmp

// MultiByteToWideChar
#define CP_ACP 0
#ifdef __APPLE__
// TheSuperHackers @bugfix Okladnoj 15/10/2026 Follow the Win32 contract: a length of -1 converts and counts the null terminator,
// a zero output size returns the required size, and a short buffer fails instead of being left unterminated.
#include <climits>
#include <cstring>
#include <cwchar>

inline int MultiByteToWideChar(unsigned int, unsigned int, const char* mbstr, int cb, wchar_t* wcstr, int cch)
{
	const size_t byteCount = cb < 0 ? strlen(mbstr) + 1 : (size_t)cb;
	mbstate_t state = {};
	size_t written = 0;
	size_t offset = 0;
	while (offset < byteCount)
	{
		wchar_t character;
		const size_t length = mbrtowc(&character, mbstr + offset, byteCount - offset, &state);
		if (length == (size_t)-1 || length == (size_t)-2)
			return 0;
		if (cch != 0)
		{
			if (written >= (size_t)cch)
				return 0;
			wcstr[written] = character;
		}
		++written;
		offset += length == 0 ? 1 : length;
	}
	return (int)written;
}

inline int WideCharToMultiByte(unsigned int, unsigned int, const wchar_t* wcstr, int cch, char* mbstr, int cb, const char*, int*)
{
	const size_t wideCount = cch < 0 ? wcslen(wcstr) + 1 : (size_t)cch;
	mbstate_t state = {};
	size_t written = 0;
	char bytes[MB_LEN_MAX];
	for (size_t index = 0; index < wideCount; ++index)
	{
		const size_t length = wcrtomb(bytes, wcstr[index], &state);
		if (length == (size_t)-1)
			return 0;
		if (cb != 0)
		{
			if (written + length > (size_t)cb)
				return 0;
			memcpy(mbstr + written, bytes, length);
		}
		written += length;
	}
	return (int)written;
}
#else
#define MultiByteToWideChar(cp, flags, mbstr, cb, wcstr, cch) mbstowcs(wcstr, mbstr, cch)
#define WideCharToMultiByte(cp, flags, wcstr, cch, mbstr, cb, defchar, used) wcstombs(mbstr, wcstr, cb)
#endif

