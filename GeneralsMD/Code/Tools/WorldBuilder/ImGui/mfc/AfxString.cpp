#include "afx.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <strings.h>

namespace
{

const int kResourceStringCapacity = 4096;

bool contains(LPCSTR characters, char ch)
{
	return characters != nullptr && ch != '\0' && strchr(characters, ch) != nullptr;
}

}

void CString::loadIfResource(LPCSTR lpsz)
{
	if (lpsz != nullptr && IS_INTRESOURCE(lpsz))
	{
		LoadString(static_cast<UINT>(reinterpret_cast<uintptr_t>(lpsz)));
	}
}

CString& CString::operator=(LPCSTR lpsz)
{
	if (lpsz != nullptr && IS_INTRESOURCE(lpsz))
	{
		LoadString(static_cast<UINT>(reinterpret_cast<uintptr_t>(lpsz)));
		return *this;
	}
	m_text = lpsz != nullptr ? lpsz : "";
	return *this;
}

CString& CString::operator=(char ch)
{
	m_text.assign(1, ch);
	return *this;
}

int CString::Compare(LPCSTR lpsz) const
{
	return strcmp(m_text.c_str(), lpsz != nullptr ? lpsz : "");
}

int CString::CompareNoCase(LPCSTR lpsz) const
{
	return strcasecmp(m_text.c_str(), lpsz != nullptr ? lpsz : "");
}

CString CString::Mid(int nFirst) const
{
	return Mid(nFirst, GetLength() - nFirst);
}

CString CString::Mid(int nFirst, int nCount) const
{
	const int first = std::clamp(nFirst, 0, GetLength());
	const int count = std::clamp(nCount, 0, GetLength() - first);
	return CString(m_text.c_str() + first, count);
}

CString CString::Left(int nCount) const
{
	return CString(m_text.c_str(), std::clamp(nCount, 0, GetLength()));
}

CString CString::Right(int nCount) const
{
	const int count = std::clamp(nCount, 0, GetLength());
	return CString(m_text.c_str() + GetLength() - count, count);
}

CString CString::SpanIncluding(LPCSTR lpszCharSet) const
{
	return Left(static_cast<int>(strspn(m_text.c_str(), lpszCharSet)));
}

CString CString::SpanExcluding(LPCSTR lpszCharSet) const
{
	return Left(static_cast<int>(strcspn(m_text.c_str(), lpszCharSet)));
}

void CString::MakeUpper()
{
	std::transform(m_text.begin(), m_text.end(), m_text.begin(), [](char ch) { return static_cast<char>(toupper(static_cast<unsigned char>(ch))); });
}

void CString::MakeLower()
{
	std::transform(m_text.begin(), m_text.end(), m_text.begin(), [](char ch) { return static_cast<char>(tolower(static_cast<unsigned char>(ch))); });
}

void CString::MakeReverse()
{
	std::reverse(m_text.begin(), m_text.end());
}

void CString::TrimRight()
{
	while (!m_text.empty() && isspace(static_cast<unsigned char>(m_text.back())))
	{
		m_text.pop_back();
	}
}

void CString::TrimRight(char chTarget)
{
	while (!m_text.empty() && m_text.back() == chTarget)
	{
		m_text.pop_back();
	}
}

void CString::TrimRight(LPCSTR lpszTargets)
{
	while (!m_text.empty() && contains(lpszTargets, m_text.back()))
	{
		m_text.pop_back();
	}
}

void CString::TrimLeft()
{
	const auto first = std::find_if(m_text.begin(), m_text.end(), [](char ch) { return !isspace(static_cast<unsigned char>(ch)); });
	m_text.erase(m_text.begin(), first);
}

void CString::TrimLeft(char chTarget)
{
	const auto first = std::find_if(m_text.begin(), m_text.end(), [chTarget](char ch) { return ch != chTarget; });
	m_text.erase(m_text.begin(), first);
}

void CString::TrimLeft(LPCSTR lpszTargets)
{
	const auto first = std::find_if(m_text.begin(), m_text.end(), [lpszTargets](char ch) { return !contains(lpszTargets, ch); });
	m_text.erase(m_text.begin(), first);
}

int CString::Replace(char chOld, char chNew)
{
	const int count = static_cast<int>(std::count(m_text.begin(), m_text.end(), chOld));
	std::replace(m_text.begin(), m_text.end(), chOld, chNew);
	return count;
}

int CString::Replace(LPCSTR lpszOld, LPCSTR lpszNew)
{
	const size_t oldLength = lpszOld != nullptr ? strlen(lpszOld) : 0;
	if (oldLength == 0)
	{
		return 0;
	}
	const std::string replacement = lpszNew != nullptr ? lpszNew : "";
	int count = 0;
	for (size_t position = m_text.find(lpszOld); position != std::string::npos; position = m_text.find(lpszOld, position + replacement.size()))
	{
		m_text.replace(position, oldLength, replacement);
		++count;
	}
	return count;
}

int CString::Remove(char ch)
{
	const size_t before = m_text.size();
	m_text.erase(std::remove(m_text.begin(), m_text.end(), ch), m_text.end());
	return static_cast<int>(before - m_text.size());
}

int CString::Insert(int nIndex, char ch)
{
	m_text.insert(static_cast<size_t>(std::clamp(nIndex, 0, GetLength())), 1, ch);
	return GetLength();
}

int CString::Insert(int nIndex, LPCSTR pstr)
{
	m_text.insert(static_cast<size_t>(std::clamp(nIndex, 0, GetLength())), pstr != nullptr ? pstr : "");
	return GetLength();
}

int CString::Delete(int nIndex, int nCount)
{
	const int index = std::clamp(nIndex, 0, GetLength());
	m_text.erase(static_cast<size_t>(index), static_cast<size_t>(std::clamp(nCount, 0, GetLength() - index)));
	return GetLength();
}

int CString::Find(char ch, int nStart) const
{
	if (nStart < 0 || nStart >= GetLength())
	{
		return -1;
	}
	const size_t position = m_text.find(ch, static_cast<size_t>(nStart));
	return position == std::string::npos ? -1 : static_cast<int>(position);
}

int CString::Find(LPCSTR lpszSub, int nStart) const
{
	if (lpszSub == nullptr || nStart < 0 || nStart > GetLength())
	{
		return -1;
	}
	const size_t position = m_text.find(lpszSub, static_cast<size_t>(nStart));
	return position == std::string::npos ? -1 : static_cast<int>(position);
}

int CString::ReverseFind(char ch) const
{
	const size_t position = m_text.rfind(ch);
	return position == std::string::npos ? -1 : static_cast<int>(position);
}

int CString::FindOneOf(LPCSTR lpszCharSet) const
{
	const size_t position = m_text.find_first_of(lpszCharSet != nullptr ? lpszCharSet : "");
	return position == std::string::npos ? -1 : static_cast<int>(position);
}

void AFX_CDECL CString::Format(LPCSTR lpszFormat, ...)
{
	va_list arguments;
	va_start(arguments, lpszFormat);
	FormatV(lpszFormat, arguments);
	va_end(arguments);
}

void AFX_CDECL CString::Format(UINT nFormatID, ...)
{
	CString format;
	format.LoadString(nFormatID);
	va_list arguments;
	va_start(arguments, nFormatID);
	FormatV(format, arguments);
	va_end(arguments);
}

void CString::FormatV(LPCSTR lpszFormat, va_list argList)
{
	va_list measuring;
	va_copy(measuring, argList);
	const int length = vsnprintf(nullptr, 0, lpszFormat, measuring);
	va_end(measuring);
	if (length < 0)
	{
		m_text.clear();
		return;
	}
	std::string formatted(static_cast<size_t>(length) + 1, '\0');
	vsnprintf(formatted.data(), formatted.size(), lpszFormat, argList);
	formatted.resize(static_cast<size_t>(length));
	m_text = formatted;
}

BOOL CString::LoadString(UINT nID)
{
	char buffer[kResourceStringCapacity];
	const int length = ::LoadStringA(nullptr, nID, buffer, kResourceStringCapacity);
	if (length == 0)
	{
		return FALSE;
	}
	m_text.assign(buffer, static_cast<size_t>(length));
	return TRUE;
}

LPSTR CString::GetBuffer(int nMinBufLength)
{
	if (nMinBufLength > GetLength())
	{
		m_text.resize(static_cast<size_t>(nMinBufLength));
	}
	return m_text.data();
}

void CString::ReleaseBuffer(int nNewLength)
{
	const size_t length = nNewLength < 0 ? strlen(m_text.c_str()) : static_cast<size_t>(nNewLength);
	m_text.resize(length);
}

LPSTR CString::GetBufferSetLength(int nNewLength)
{
	m_text.resize(static_cast<size_t>(std::max(0, nNewLength)));
	return m_text.data();
}

namespace
{

int insertIndexFor(char marker)
{
	return marker > '9' ? 9 + (marker - 'A') : marker - '1';
}

bool isInsertMarker(LPCSTR source)
{
	const char marker = source[1];
	return source[0] == '%' && ((marker >= '0' && marker <= '9') || (marker >= 'A' && marker <= 'Z'));
}

void formatStrings(CString& rString, UINT nIDS, LPCSTR const* inserts, int insertCount)
{
	CString format;
	if (!format.LoadString(nIDS))
	{
		return;
	}
	std::string result;
	for (LPCSTR source = format; *source != '\0'; ++source)
	{
		if (!isInsertMarker(source))
		{
			result += *source;
			continue;
		}
		const int index = insertIndexFor(*++source);
		if (index >= insertCount)
		{
			result += '?';
		}
		else if (index >= 0 && inserts[index] != nullptr)
		{
			result += inserts[index];
		}
	}
	rString = result.c_str();
}

}

BOOL AFXAPI AfxExtractSubString(CString& rString, LPCSTR lpszFullString, int iSubString, char chSep)
{
	if (lpszFullString == nullptr)
	{
		return FALSE;
	}
	for (; iSubString > 0; --iSubString)
	{
		lpszFullString = strchr(lpszFullString, chSep);
		if (lpszFullString == nullptr)
		{
			rString.Empty();
			return FALSE;
		}
		++lpszFullString;
	}
	const char* end = strchr(lpszFullString, chSep);
	const int length = end != nullptr ? static_cast<int>(end - lpszFullString) : static_cast<int>(strlen(lpszFullString));
	memcpy(rString.GetBufferSetLength(length), lpszFullString, static_cast<size_t>(length));
	return TRUE;
}

void AFXAPI AfxFormatString1(CString& rString, UINT nIDS, LPCSTR lpsz1)
{
	formatStrings(rString, nIDS, &lpsz1, 1);
}

void AFXAPI AfxFormatString2(CString& rString, UINT nIDS, LPCSTR lpsz1, LPCSTR lpsz2)
{
	const LPCSTR inserts[] = { lpsz1, lpsz2 };
	formatStrings(rString, nIDS, inserts, 2);
}
