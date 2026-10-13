#include "afx.h"

#include "Win32Paths.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

IMPLEMENT_DYNAMIC(CFile, CObject)

namespace
{

const char* const kFileErrorMessages[] = {
	"No error occurred.",
	"An unspecified error occurred while accessing %s.",
	"%s was not found.",
	"%s contains an invalid path.",
	"%s could not be opened because there are too many open files.",
	"Access to %s was denied.",
	"An invalid file handle was associated with %s.",
	"%s could not be removed because it is the current directory.",
	"%s could not be created because the directory is full.",
	"Seek failed on %s.",
	"A hardware I/O error was reported while accessing %s.",
	"A sharing violation occurred while accessing %s.",
	"A locking violation occurred while accessing %s.",
	"Disk full while accessing %s.",
	"An attempt was made to access %s past its end.",
};

std::string nativePath(LPCSTR path)
{
	return Win32NativePath(path != nullptr ? path : "");
}

int openFlagsFor(UINT nOpenFlags)
{
	const UINT access = nOpenFlags & 0x3;
	int flags = access == CFile::modeWrite ? O_WRONLY : (access == CFile::modeReadWrite ? O_RDWR : O_RDONLY);
	if ((nOpenFlags & CFile::modeCreate) != 0)
	{
		flags |= O_CREAT;
		flags |= (nOpenFlags & CFile::modeNoTruncate) == 0 ? O_TRUNC : 0;
	}
	return flags;
}

size_t lastSeparator(const CString& path)
{
	const std::string text = static_cast<LPCSTR>(path);
	const size_t separator = text.find_last_of("\\/");
	return separator == std::string::npos ? 0 : separator + 1;
}

}

CFileException::CFileException(int cause, LONG lOsError, LPCSTR lpszArchiveName) :
	m_cause(cause),
	m_lOsError(lOsError),
	m_strFileName(lpszArchiveName)
{
}

IMPLEMENT_DYNAMIC(CException, CObject)
IMPLEMENT_DYNAMIC(CFileException, CException)

BOOL CFileException::GetErrorMessage(LPSTR lpszError, UINT nMaxError, UINT* pnHelpContext)
{
	if (pnHelpContext != nullptr)
	{
		*pnHelpContext = 0;
	}
	if (lpszError == nullptr || nMaxError == 0)
	{
		return FALSE;
	}
	const int cause = m_cause >= none && m_cause <= endOfFile ? m_cause : generic;
	const CString fileName = m_strFileName.IsEmpty() ? CString("an unnamed file") : m_strFileName;
	snprintf(lpszError, nMaxError, kFileErrorMessages[cause], static_cast<LPCSTR>(fileName));
	return TRUE;
}

int CFileException::OsErrorToException(LONG lOsError)
{
	switch (lOsError)
	{
	case 0:
		return none;
	case ENOENT:
		return fileNotFound;
	case ENOTDIR:
	case ENAMETOOLONG:
	case ELOOP:
		return badPath;
	case EMFILE:
	case ENFILE:
		return tooManyOpenFiles;
	case EACCES:
	case EPERM:
	case EROFS:
	case EISDIR:
		return accessDenied;
	case EBADF:
		return invalidFile;
	case EBUSY:
		return sharingViolation;
	case ENOSPC:
	case EDQUOT:
		return diskFull;
	case EIO:
		return hardIO;
	case ESPIPE:
	case EINVAL:
		return badSeek;
	default:
		return generic;
	}
}

void AFXAPI AfxThrowFileException(int cause, LONG lOsError, LPCSTR lpszFileName)
{
	throw new CFileException(cause, lOsError, lpszFileName);
}

CFile::CFile() :
	m_hFile(hFileNull),
	m_bCloseOnDelete(FALSE)
{
}

CFile::CFile(LPCSTR lpszFileName, UINT nOpenFlags) :
	m_hFile(hFileNull),
	m_bCloseOnDelete(FALSE)
{
	CFileException error;
	if (!Open(lpszFileName, nOpenFlags, &error))
	{
		AfxThrowFileException(error.m_cause, error.m_lOsError, error.m_strFileName);
	}
}

CFile::~CFile()
{
	if (m_hFile != hFileNull && m_bCloseOnDelete)
	{
		Abort();
	}
}

BOOL CFile::Open(LPCSTR lpszFileName, UINT nOpenFlags, CFileException* pError)
{
	m_strFileName = lpszFileName;
	const int handle = ::open(nativePath(lpszFileName).c_str(), openFlagsFor(nOpenFlags), 0644);
	if (handle < 0)
	{
		if (pError != nullptr)
		{
			pError->m_lOsError = errno;
			pError->m_cause = CFileException::OsErrorToException(errno);
			pError->m_strFileName = lpszFileName;
		}
		return FALSE;
	}
	m_hFile = handle;
	m_bCloseOnDelete = TRUE;
	return TRUE;
}

void CFile::throwLastError() const
{
	AfxThrowFileException(CFileException::OsErrorToException(errno), errno, m_strFileName);
}

void CFile::Close()
{
	const int handle = m_hFile;
	m_hFile = hFileNull;
	m_bCloseOnDelete = FALSE;
	m_strFileName.Empty();
	if (handle != hFileNull && ::close(handle) != 0)
	{
		throwLastError();
	}
}

void CFile::Abort()
{
	if (m_hFile != hFileNull)
	{
		::close(m_hFile);
		m_hFile = hFileNull;
	}
	m_bCloseOnDelete = FALSE;
	m_strFileName.Empty();
}

UINT CFile::Read(void* lpBuf, UINT nCount)
{
	const ssize_t count = ::read(m_hFile, lpBuf, nCount);
	if (count < 0)
	{
		throwLastError();
	}
	return static_cast<UINT>(count);
}

void CFile::Write(const void* lpBuf, UINT nCount)
{
	const ssize_t count = ::write(m_hFile, lpBuf, nCount);
	if (count < 0)
	{
		throwLastError();
	}
	if (static_cast<UINT>(count) < nCount)
	{
		AfxThrowFileException(CFileException::diskFull, ENOSPC, m_strFileName);
	}
}

LONG CFile::Seek(LONG lOff, UINT nFrom)
{
	const int whence = nFrom == end ? SEEK_END : (nFrom == current ? SEEK_CUR : SEEK_SET);
	const off_t position = ::lseek(m_hFile, lOff, whence);
	if (position < 0)
	{
		throwLastError();
	}
	return static_cast<LONG>(position);
}

DWORD CFile::GetLength() const
{
	struct stat info;
	if (::fstat(m_hFile, &info) != 0)
	{
		throwLastError();
	}
	return static_cast<DWORD>(info.st_size);
}

void CFile::SetLength(DWORD dwNewLen)
{
	if (::ftruncate(m_hFile, static_cast<off_t>(dwNewLen)) != 0)
	{
		throwLastError();
	}
}

DWORD CFile::GetPosition() const
{
	const off_t position = ::lseek(m_hFile, 0, SEEK_CUR);
	if (position < 0)
	{
		throwLastError();
	}
	return static_cast<DWORD>(position);
}

void CFile::Flush()
{
	if (m_hFile != hFileNull && ::fsync(m_hFile) != 0)
	{
		throwLastError();
	}
}

CString CFile::GetFileName() const
{
	return m_strFileName.Mid(static_cast<int>(lastSeparator(m_strFileName)));
}

CString CFile::GetFileTitle() const
{
	const CString name = GetFileName();
	const int extension = name.ReverseFind('.');
	return extension > 0 ? name.Left(extension) : name;
}

BOOL CFile::GetStatus(CFileStatus& rStatus) const
{
	return GetStatus(m_strFileName, rStatus);
}

BOOL PASCAL CFile::GetStatus(LPCSTR lpszFileName, CFileStatus& rStatus)
{
	const std::string native = nativePath(lpszFileName);
	struct stat info;
	if (::stat(native.c_str(), &info) != 0)
	{
		return FALSE;
	}

	const size_t nameStart = native.find_last_of('/');
	const char* name = native.c_str() + (nameStart == std::string::npos ? 0 : nameStart + 1);
	rStatus.m_ctime = CTime(info.st_birthtimespec.tv_sec);
	rStatus.m_mtime = CTime(info.st_mtimespec.tv_sec);
	rStatus.m_atime = CTime(info.st_atimespec.tv_sec);
	rStatus.m_size = static_cast<LONG>(info.st_size);
	rStatus.m_attribute = static_cast<BYTE>((S_ISDIR(info.st_mode) ? directory : normal)
		| (::access(native.c_str(), W_OK) != 0 ? readOnly : normal) | (name[0] == '.' ? hidden : normal));

	char fullPath[PATH_MAX];
	const std::string enginePath = Win32EnginePath(::realpath(native.c_str(), fullPath) != nullptr ? fullPath : native.c_str());
	strlcpy(rStatus.m_szFullName, enginePath.c_str(), sizeof(rStatus.m_szFullName));
	return TRUE;
}

void PASCAL CFile::Rename(LPCSTR lpszOldName, LPCSTR lpszNewName)
{
	if (::rename(nativePath(lpszOldName).c_str(), nativePath(lpszNewName).c_str()) != 0)
	{
		AfxThrowFileException(CFileException::OsErrorToException(errno), errno, lpszOldName);
	}
}

void PASCAL CFile::Remove(LPCSTR lpszFileName)
{
	if (::unlink(nativePath(lpszFileName).c_str()) != 0)
	{
		AfxThrowFileException(CFileException::OsErrorToException(errno), errno, lpszFileName);
	}
}

CArchive::CArchive(CFile* pFile, UINT nMode, int nBufSize, void*) :
	m_pFile(pFile),
	m_nMode(nMode),
	m_bufferSize(static_cast<size_t>(nBufSize > 0 ? nBufSize : 4096))
{
	m_buffer.reserve(m_bufferSize);
}

CArchive::~CArchive()
{
	if (m_isClosed || (m_nMode & bNoFlushOnDelete) != 0)
	{
		return;
	}
	try
	{
		Close();
	}
	catch (CException* exception)
	{
		exception->Delete();
	}
}

UINT CArchive::Read(void* lpBuf, UINT nMax)
{
	return m_pFile->Read(lpBuf, nMax);
}

void CArchive::Write(const void* lpBuf, UINT nMax)
{
	const char* bytes = static_cast<const char*>(lpBuf);
	m_buffer.insert(m_buffer.end(), bytes, bytes + nMax);
	if (m_buffer.size() >= m_bufferSize)
	{
		Flush();
	}
}

void CArchive::Flush()
{
	if (!IsStoring() || m_buffer.empty())
	{
		return;
	}
	m_pFile->Write(m_buffer.data(), static_cast<UINT>(m_buffer.size()));
	m_buffer.clear();
}

void CArchive::Close()
{
	Flush();
	m_isClosed = true;
}

void CArchive::Abort()
{
	m_buffer.clear();
	m_isClosed = true;
}
