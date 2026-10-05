/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
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

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: QuotedPrintable.cpp /////////////////////////////////////////////////////////
// Author: Matt Campbell, February 2002
// Description: Quoted-printable encode/decode
////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/QuotedPrintable.h"

#define MAGIC_CHAR '_'

// takes an integer and returns an ASCII representation
static char intToHexDigit(int num)
{
	if (num < 0 || num >15) return '\0';
	if (num < 10)
	{
		return '0' + num;
	}
	return 'A' + (num - 10);
}

// convert an ASCII representation of a hex digit into the digit itself
static int hexDigitToInt(char c)
{
	if (c <= '9' && c >= '0') return (c - '0');
	if (c <= 'f' && c >= 'a') return (c - 'a' + 10);
	if (c <= 'F' && c >= 'A') return (c - 'A' + 10);
	return 0;
}

#ifdef __APPLE__
static void appendQuotedPrintableByte(char* dest, int& i, unsigned char byte)
{
	if (!isalnum(byte))
	{
		dest[i++] = MAGIC_CHAR;
		dest[i++] = intToHexDigit(byte >> 4);
		dest[i++] = intToHexDigit(byte & 0xf);
		return;
	}
	dest[i++] = byte;
}

// Convert unicode strings into ascii quoted-printable strings
AsciiString UnicodeStringToQuotedPrintable(UnicodeString original)
{
	static char dest[1024];
	const WideChar* src = original.str();
	int i = 0;
	while (*src != 0 && i < 1018)
	{
		const UnsignedInt utf16Unit = static_cast<UnsignedInt>(*src) & 0xffff;
		appendQuotedPrintableByte(dest, i, static_cast<unsigned char>(utf16Unit & 0xff));
		appendQuotedPrintableByte(dest, i, static_cast<unsigned char>(utf16Unit >> 8));
		src++;
	}
	dest[i] = '\0';

	return dest;
}
#else
// Convert unicode strings into ascii quoted-printable strings
AsciiString UnicodeStringToQuotedPrintable(UnicodeString original)
{
	static char dest[1024];
	const unsigned char* src = reinterpret_cast<const unsigned char*>(original.str());
	int i = 0;
	while (!(src[0] == '\0' && src[1] == '\0') && i < 1021)
	{
		if (!isalnum(*src))
		{
			dest[i++] = MAGIC_CHAR;
			dest[i++] = intToHexDigit((*src) >> 4);
			dest[i++] = intToHexDigit((*src) & 0xf);
		}
		else
		{
			dest[i++] = *src;
		}
		src++;
		if (!isalnum(*src))
		{
			dest[i++] = MAGIC_CHAR;
			dest[i++] = intToHexDigit((*src) >> 4);
			dest[i++] = intToHexDigit((*src) & 0xf);
		}
		else
		{
			dest[i++] = *src;
		}
		src++;
	}
	dest[i] = '\0';

	return dest;
}
#endif

// Convert ascii strings into ascii quoted-printable strings
AsciiString AsciiStringToQuotedPrintable(AsciiString original)
{
	static char dest[1024];
	const unsigned char* src = reinterpret_cast<const unsigned char*>(original.str());
	int i = 0;
	while (src[0] != '\0' && i < 1021)
	{
		if (!isalnum(*src))
		{
			dest[i++] = MAGIC_CHAR;
			dest[i++] = intToHexDigit((*src) >> 4);
			dest[i++] = intToHexDigit((*src) & 0xf);
		}
		else
		{
			dest[i++] = *src;
		}
		src++;
	}
	dest[i] = '\0';

	return dest;
}

#ifdef __APPLE__
static int decodeQuotedPrintableBytes(const unsigned char* src, unsigned char* bytes, int maxBytes)
{
	int count = 0;
	while (*src && count < maxBytes)
	{
		if (*src != MAGIC_CHAR)
		{
			bytes[count++] = *src++;
			continue;
		}
		if (src[1] == '\0')
		{
			break;
		}
		unsigned char value = hexDigitToInt(src[1]);
		src += 2;
		if (*src != '\0')
		{
			value = (value << 4) | hexDigitToInt(*src);
			src++;
		}
		bytes[count++] = value;
	}
	return count;
}

// Convert ascii quoted-printable strings into unicode strings
UnicodeString QuotedPrintableToUnicodeString(AsciiString original)
{
	static WideChar dest[1024];
	static unsigned char bytes[2046];
	const int byteCount = decodeQuotedPrintableBytes((const unsigned char*)original.str(), bytes, 2046);
	int i = 0;
	for (int b = 0; b < byteCount; b += 2)
	{
		const unsigned char highByte = (b + 1 < byteCount) ? bytes[b + 1] : 0;
		dest[i++] = static_cast<WideChar>(bytes[b] | (highByte << 8));
	}
	dest[i] = 0;

	UnicodeString out(dest);
	return out;
}
#else
// Convert ascii quoted-printable strings into unicode strings
UnicodeString QuotedPrintableToUnicodeString(AsciiString original)
{
	static WideChar dest[1024];
	int i = 0;

	unsigned char* c = (unsigned char*)dest;
	const unsigned char* src = (const unsigned char*)original.str();

	while (*src && i < 1023)
	{
		if (*src == MAGIC_CHAR)
		{
			if (src[1] == '\0')
			{
				// string ends with MAGIC_CHAR
				break;
			}
			*c = hexDigitToInt(src[1]);
			src++;
			if (src[1] != '\0')
			{
				*c = *c << 4;
				*c = *c | hexDigitToInt(src[1]);
				src++;
			}
		}
		else
		{
			*c = *src;
		}
		src++;
		c++;
	}

	// Fixup odd-length strings
	if ((c - (unsigned char*)dest) % 2)
	{
		// OK
	}
	else
	{
		*c = '\0';
		c++;
	}

	*c = 0;

	UnicodeString out(dest);
	return out;
}
#endif

// Convert ascii quoted-printable strings into ascii strings
AsciiString QuotedPrintableToAsciiString(AsciiString original)
{
	static unsigned char dest[1024];
	int i = 0;

	unsigned char* c = (unsigned char*)dest;
	const unsigned char* src = (const unsigned char*)original.str();

	while (*src && i < 1023)
	{
		if (*src == MAGIC_CHAR)
		{
			if (src[1] == '\0')
			{
				// string ends with MAGIC_CHAR
				break;
			}
			*c = hexDigitToInt(src[1]);
			src++;
			if (src[1] != '\0')
			{
				*c = *c << 4;
				*c = *c | hexDigitToInt(src[1]);
				src++;
			}
		}
		else
		{
			*c = *src;
		}
		src++;
		c++;
	}

	*c = 0;

	return AsciiString((const char*)dest);
}
