#pragma once

#include "AfxCore.h"

#include <cstdarg>
#include <string>

class CString
{
public:
	CString() = default;
	CString(const CString& other) = default;
	CString(LPCSTR lpsz) : m_text(lpsz != nullptr && !IS_INTRESOURCE(lpsz) ? lpsz : "") { loadIfResource(lpsz); }
	CString(LPCSTR lpch, int nLength) : m_text(lpch, static_cast<size_t>(nLength)) {}
	CString(char ch, int nRepeat = 1) : m_text(static_cast<size_t>(nRepeat), ch) {}
	CString(const unsigned char* lpsz) : CString(reinterpret_cast<LPCSTR>(lpsz)) {}

	CString& operator=(const CString& other) = default;
	CString& operator=(LPCSTR lpsz);
	CString& operator=(char ch);
	CString& operator=(const unsigned char* lpsz) { return *this = reinterpret_cast<LPCSTR>(lpsz); }

	CString& operator+=(const CString& other) { m_text += other.m_text; return *this; }
	CString& operator+=(LPCSTR lpsz) { m_text += lpsz != nullptr ? lpsz : ""; return *this; }
	CString& operator+=(char ch) { m_text += ch; return *this; }

	friend CString operator+(const CString& first, const CString& second) { return CString(first) += second; }
	friend CString operator+(const CString& first, LPCSTR second) { return CString(first) += second; }
	friend CString operator+(LPCSTR first, const CString& second) { return CString(first) += second; }
	friend CString operator+(const CString& first, char second) { return CString(first) += second; }
	friend CString operator+(char first, const CString& second) { return CString(first) += second; }

	friend bool operator==(const CString& first, const CString& second) { return first.Compare(second) == 0; }
	friend bool operator==(const CString& first, LPCSTR second) { return first.Compare(second) == 0; }
	friend bool operator==(LPCSTR first, const CString& second) { return second.Compare(first) == 0; }
	friend bool operator!=(const CString& first, const CString& second) { return first.Compare(second) != 0; }
	friend bool operator!=(const CString& first, LPCSTR second) { return first.Compare(second) != 0; }
	friend bool operator!=(LPCSTR first, const CString& second) { return second.Compare(first) != 0; }
	friend bool operator<(const CString& first, const CString& second) { return first.Compare(second) < 0; }
	friend bool operator>(const CString& first, const CString& second) { return first.Compare(second) > 0; }
	friend bool operator<=(const CString& first, const CString& second) { return first.Compare(second) <= 0; }
	friend bool operator>=(const CString& first, const CString& second) { return first.Compare(second) >= 0; }

	operator LPCSTR() const { return m_text.c_str(); }
	char operator[](int nIndex) const { return m_text[static_cast<size_t>(nIndex)]; }

	int GetLength() const { return static_cast<int>(m_text.size()); }
	BOOL IsEmpty() const { return m_text.empty() ? TRUE : FALSE; }
	void Empty() { m_text.clear(); }
	char GetAt(int nIndex) const { return m_text[static_cast<size_t>(nIndex)]; }
	void SetAt(int nIndex, char ch) { m_text[static_cast<size_t>(nIndex)] = ch; }

	int Compare(LPCSTR lpsz) const;
	int CompareNoCase(LPCSTR lpsz) const;
	int Collate(LPCSTR lpsz) const { return Compare(lpsz); }
	int CollateNoCase(LPCSTR lpsz) const { return CompareNoCase(lpsz); }

	CString Mid(int nFirst) const;
	CString Mid(int nFirst, int nCount) const;
	CString Left(int nCount) const;
	CString Right(int nCount) const;
	CString SpanIncluding(LPCSTR lpszCharSet) const;
	CString SpanExcluding(LPCSTR lpszCharSet) const;

	void MakeUpper();
	void MakeLower();
	void MakeReverse();
	void TrimRight();
	void TrimRight(char chTarget);
	void TrimRight(LPCSTR lpszTargets);
	void TrimLeft();
	void TrimLeft(char chTarget);
	void TrimLeft(LPCSTR lpszTargets);

	int Replace(char chOld, char chNew);
	int Replace(LPCSTR lpszOld, LPCSTR lpszNew);
	int Remove(char ch);
	int Insert(int nIndex, char ch);
	int Insert(int nIndex, LPCSTR pstr);
	int Delete(int nIndex, int nCount = 1);

	int Find(char ch) const { return Find(ch, 0); }
	int Find(char ch, int nStart) const;
	int Find(LPCSTR lpszSub) const { return Find(lpszSub, 0); }
	int Find(LPCSTR lpszSub, int nStart) const;
	int ReverseFind(char ch) const;
	int FindOneOf(LPCSTR lpszCharSet) const;

	void AFX_CDECL Format(LPCSTR lpszFormat, ...);
	void AFX_CDECL Format(UINT nFormatID, ...);
	void FormatV(LPCSTR lpszFormat, va_list argList);
	BOOL LoadString(UINT nID);

	LPSTR GetBuffer(int nMinBufLength);
	void ReleaseBuffer(int nNewLength = -1);
	LPSTR GetBufferSetLength(int nNewLength);
	void FreeExtra() { m_text.shrink_to_fit(); }
	LPSTR LockBuffer() { return GetBuffer(0); }
	void UnlockBuffer() {}

private:
	void loadIfResource(LPCSTR lpsz);

	std::string m_text;
};

BOOL AFXAPI AfxExtractSubString(CString& rString, LPCSTR lpszFullString, int iSubString, char chSep = '\n');
void AFXAPI AfxFormatString1(CString& rString, UINT nIDS, LPCSTR lpsz1);
void AFXAPI AfxFormatString2(CString& rString, UINT nIDS, LPCSTR lpsz1, LPCSTR lpsz2);
