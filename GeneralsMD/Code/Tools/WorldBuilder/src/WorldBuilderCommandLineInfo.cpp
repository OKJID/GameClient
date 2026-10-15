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

#include "StdAfx.h"
#include "WorldBuilderCommandLineInfo.h"

void WorldBuilderCommandLineInfo::ParseParam(LPCSTR lpszParam, BOOL bFlag, BOOL bLast)
{
	if (m_isModPathNext)
	{
		m_isModPathNext = false;
		ParseLast(bLast);
		return;
	}

	if (bFlag && stricmp(lpszParam, "mod") == 0)
	{
		m_isModPathNext = true;
		ParseLast(bLast);
		return;
	}

	CCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);
}
