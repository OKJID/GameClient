#pragma once

#include "AfxString.h"

#include <ctime>
#include <vector>

class CDocument;

class CTime
{
public:
	CTime() = default;
	CTime(time_t time) : m_time(time) {}

	static CTime GetCurrentTime() { return CTime(::time(nullptr)); }
	time_t GetTime() const { return m_time; }

	BOOL operator==(CTime time) const { return m_time == time.m_time; }
	BOOL operator!=(CTime time) const { return m_time != time.m_time; }
	BOOL operator<(CTime time) const { return m_time < time.m_time; }
	BOOL operator>(CTime time) const { return m_time > time.m_time; }

private:
	time_t m_time = 0;
};

class CException : public CObject
{
	DECLARE_DYNAMIC(CException)

public:
	void Delete() { delete this; }
	virtual BOOL GetErrorMessage(LPSTR lpszError, UINT nMaxError, UINT* pnHelpContext = nullptr);
	virtual int ReportError(UINT nType = MB_OK, UINT nMessageID = 0);
};

class CMemoryException : public CException
{
	DECLARE_DYNAMIC(CMemoryException)

public:
	BOOL GetErrorMessage(LPSTR lpszError, UINT nMaxError, UINT* pnHelpContext = nullptr) override;
};

void AFXAPI AfxThrowMemoryException();

class CFileException : public CException
{
	DECLARE_DYNAMIC(CFileException)

public:
	enum
	{
		none,
		generic,
		fileNotFound,
		badPath,
		tooManyOpenFiles,
		accessDenied,
		invalidFile,
		removeCurrentDir,
		directoryFull,
		badSeek,
		hardIO,
		sharingViolation,
		lockViolation,
		diskFull,
		endOfFile,
	};

	CFileException(int cause = none, LONG lOsError = -1, LPCSTR lpszArchiveName = nullptr);

	BOOL GetErrorMessage(LPSTR lpszError, UINT nMaxError, UINT* pnHelpContext = nullptr) override;
	static int OsErrorToException(LONG lOsError);

	int m_cause;
	LONG m_lOsError;
	CString m_strFileName;
};

void AFXAPI AfxThrowFileException(int cause, LONG lOsError = -1, LPCSTR lpszFileName = nullptr);

struct CFileStatus
{
	CTime m_ctime;
	CTime m_mtime;
	CTime m_atime;
	LONG m_size = 0;
	BYTE m_attribute = 0;
	BYTE m_padding = 0;
	char m_szFullName[_MAX_PATH] = {};
};

class CFile : public CObject
{
	DECLARE_DYNAMIC(CFile)

public:
	enum OpenFlags
	{
		modeRead = 0x0000,
		modeWrite = 0x0001,
		modeReadWrite = 0x0002,
		shareCompat = 0x0000,
		shareExclusive = 0x0010,
		shareDenyWrite = 0x0020,
		shareDenyRead = 0x0030,
		shareDenyNone = 0x0040,
		modeNoInherit = 0x0080,
		modeCreate = 0x1000,
		modeNoTruncate = 0x2000,
		typeText = 0x4000,
		typeBinary = static_cast<int>(0x8000),
	};

	enum Attribute
	{
		normal = 0x00,
		readOnly = 0x01,
		hidden = 0x02,
		system = 0x04,
		volume = 0x08,
		directory = 0x10,
		archive = 0x20,
	};

	enum SeekPosition
	{
		begin = 0x0,
		current = 0x1,
		end = 0x2,
	};

	enum
	{
		hFileNull = -1
	};

	CFile();
	CFile(LPCSTR lpszFileName, UINT nOpenFlags);
	~CFile() override;

	virtual BOOL Open(LPCSTR lpszFileName, UINT nOpenFlags, CFileException* pError = nullptr);
	virtual void Close();
	virtual void Abort();
	virtual UINT Read(void* lpBuf, UINT nCount);
	virtual void Write(const void* lpBuf, UINT nCount);
	virtual LONG Seek(LONG lOff, UINT nFrom);
	virtual DWORD GetLength() const;
	virtual void SetLength(DWORD dwNewLen);
	virtual DWORD GetPosition() const;
	virtual void Flush();

	void SeekToBegin() { Seek(0, begin); }
	DWORD SeekToEnd() { return static_cast<DWORD>(Seek(0, end)); }

	virtual CString GetFileName() const;
	virtual CString GetFileTitle() const;
	virtual CString GetFilePath() const { return m_strFileName; }
	virtual void SetFilePath(LPCSTR lpszNewName) { m_strFileName = lpszNewName; }

	BOOL GetStatus(CFileStatus& rStatus) const;
	static BOOL PASCAL GetStatus(LPCSTR lpszFileName, CFileStatus& rStatus);
	static void PASCAL Rename(LPCSTR lpszOldName, LPCSTR lpszNewName);
	static void PASCAL Remove(LPCSTR lpszFileName);

	int m_hFile;

protected:
	void throwLastError() const;

	CString m_strFileName;
	BOOL m_bCloseOnDelete;
};

class CArchive
{
public:
	enum Mode
	{
		store = 0,
		load = 1,
		bNoFlushOnDelete = 2,
		bNoByteSwap = 4,
	};

	CArchive(CFile* pFile, UINT nMode, int nBufSize = 4096, void* lpBuf = nullptr);
	~CArchive();

	BOOL IsLoading() const { return (m_nMode & load) != 0 ? TRUE : FALSE; }
	BOOL IsStoring() const { return (m_nMode & load) == 0 ? TRUE : FALSE; }
	CFile* GetFile() const { return m_pFile; }

	UINT Read(void* lpBuf, UINT nMax);
	void Write(const void* lpBuf, UINT nMax);
	void Flush();
	void Close();
	void Abort();

	CDocument* m_pDocument = nullptr;
	BOOL m_bForceFlat = TRUE;

private:
	CFile* m_pFile;
	UINT m_nMode;
	size_t m_bufferSize;
	std::vector<char> m_buffer;
	bool m_isClosed = false;
};
