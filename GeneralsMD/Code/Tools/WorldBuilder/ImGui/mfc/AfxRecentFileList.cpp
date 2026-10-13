#include "AfxInternal.h"

#include <cstdio>
#include <strings.h>

namespace
{

int fileNameStart(const CString& path)
{
	return path.ReverseFind('\\') + 1;
}

CString fileTitle(LPCSTR lpszFileName)
{
	char title[_MAX_PATH] = {};
	AfxGetFileTitle(lpszFileName, title, sizeof(title));
	return title;
}

void abbreviateName(CString& path, int cchMax, BOOL bAtLeastName)
{
	const int fullLength = path.GetLength();
	const int nameStart = fileNameStart(path);
	const int nameLength = fullLength - nameStart;
	if (cchMax >= fullLength)
	{
		return;
	}
	if (cchMax < nameLength)
	{
		path = bAtLeastName ? path.Mid(nameStart) : CString();
		return;
	}

	int current = nameStart > 1 ? path.Find('\\', 1) : 0;
	const int volumeLength = current;
	if (cchMax < volumeLength + 5 + nameLength)
	{
		path = path.Mid(nameStart);
		return;
	}
	while (volumeLength + 4 + (fullLength - current) > cchMax)
	{
		current = path.Find('\\', current + 1);
	}
	path = path.Left(volumeLength) + "\\..." + path.Mid(current);
}

CString escapedMenuText(const CString& text)
{
	CString escaped;
	for (int index = 0; index < text.GetLength(); ++index)
	{
		if (text[index] == '&')
		{
			escaped += '&';
		}
		escaped += text[index];
	}
	return escaped;
}

}

CRecentFileList::CRecentFileList(UINT nStart, LPCSTR lpszSection, LPCSTR lpszEntryFormat, int nSize, int nMaxDispLen) :
	m_arrNames(static_cast<size_t>(nSize)),
	m_strSectionName(lpszSection),
	m_strEntryFormat(lpszEntryFormat),
	m_nStart(nStart),
	m_nMaxDisplayLength(nMaxDispLen)
{
}

CString CRecentFileList::entryName(int nIndex) const
{
	char entry[64];
	snprintf(entry, sizeof(entry), static_cast<LPCSTR>(m_strEntryFormat), nIndex + 1);
	return entry;
}

void CRecentFileList::Add(LPCSTR lpszPathName)
{
	char fullPath[_MAX_PATH];
	AfxFullPath(fullPath, lpszPathName);
	int index = 0;
	while (index < GetSize() - 1 && strcasecmp(m_arrNames[static_cast<size_t>(index)], fullPath) != 0)
	{
		++index;
	}
	for (; index > 0; --index)
	{
		m_arrNames[static_cast<size_t>(index)] = m_arrNames[static_cast<size_t>(index - 1)];
	}
	m_arrNames[0] = fullPath;
}

void CRecentFileList::Remove(int nIndex)
{
	for (int index = nIndex; index < GetSize() - 1; ++index)
	{
		m_arrNames[static_cast<size_t>(index)] = m_arrNames[static_cast<size_t>(index + 1)];
	}
	m_arrNames.back().Empty();
}

BOOL CRecentFileList::GetDisplayName(CString& strName, int nIndex, LPCSTR lpszCurDir, int nCurDir, BOOL bAtLeastName) const
{
	const CString& path = m_arrNames[static_cast<size_t>(nIndex)];
	if (path.IsEmpty())
	{
		return FALSE;
	}
	const int directoryLength = fileNameStart(path);
	if (lpszCurDir != nullptr && directoryLength == nCurDir && strncasecmp(path, lpszCurDir, static_cast<size_t>(nCurDir)) == 0)
	{
		strName = fileTitle(static_cast<LPCSTR>(path) + nCurDir);
		return TRUE;
	}
	strName = path;
	if (m_nMaxDisplayLength == -1)
	{
		return TRUE;
	}
	strName = path.Left(directoryLength) + fileTitle(static_cast<LPCSTR>(path) + directoryLength);
	abbreviateName(strName, m_nMaxDisplayLength, bAtLeastName);
	return TRUE;
}

void CRecentFileList::UpdateMenu(CCmdUI* pCmdUI)
{
	CMenu* menu = pCmdUI->m_pMenu;
	if (m_strOriginal.IsEmpty() && menu != nullptr)
	{
		menu->GetMenuString(pCmdUI->m_nID, m_strOriginal, MF_BYCOMMAND);
	}
	if (m_arrNames[0].IsEmpty())
	{
		if (!m_strOriginal.IsEmpty())
		{
			pCmdUI->SetText(m_strOriginal);
		}
		pCmdUI->Enable(FALSE);
		return;
	}
	if (menu == nullptr)
	{
		return;
	}
	for (int index = 0; index < GetSize(); ++index)
	{
		menu->DeleteMenu(pCmdUI->m_nID + static_cast<UINT>(index), MF_BYCOMMAND);
	}
	insertMenuItems(pCmdUI);
}

void CRecentFileList::insertMenuItems(CCmdUI* pCmdUI)
{
	char currentDirectory[_MAX_PATH] = {};
	::GetCurrentDirectoryA(_MAX_PATH - 1, currentDirectory);
	CString directory = currentDirectory;
	directory += '\\';

	CString name;
	for (int index = 0; index < GetSize(); ++index)
	{
		if (!GetDisplayName(name, index, directory, directory.GetLength()))
		{
			break;
		}
		CString text;
		text.Format("&%d ", (index + 1 + static_cast<int>(m_nStart)) % 10);
		text += escapedMenuText(name);
		pCmdUI->m_pMenu->InsertMenu(pCmdUI->m_nIndex++, MF_STRING | MF_BYPOSITION, pCmdUI->m_nID++, text);
	}
	pCmdUI->m_nIndex--;
	pCmdUI->m_nIndexMax = pCmdUI->m_pMenu->GetMenuItemCount();
	pCmdUI->m_bEnableChanged = TRUE;
}

void CRecentFileList::WriteList()
{
	CWinApp* app = AfxGetApp();
	app->WriteProfileString(m_strSectionName, nullptr, nullptr);
	for (int index = 0; index < GetSize(); ++index)
	{
		const CString& name = m_arrNames[static_cast<size_t>(index)];
		if (!name.IsEmpty())
		{
			app->WriteProfileString(m_strSectionName, entryName(index), name);
		}
	}
}

void CRecentFileList::ReadList()
{
	CWinApp* app = AfxGetApp();
	for (int index = 0; index < GetSize(); ++index)
	{
		m_arrNames[static_cast<size_t>(index)] = app->GetProfileString(m_strSectionName, entryName(index), "");
	}
}
