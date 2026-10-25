/*
**	Command & Conquer Generals(tm)
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

#pragma once

#include "Common/FileSystem.h"

class SystemMaps
{
public:
	static void getNames(FilenameList &names);
	static Bool isOfficialName(const AsciiString &name);
	static AsciiString getOfficialGameName(const AsciiString &mapName);
	static AsciiString getCopyName(const AsciiString &name);
	static AsciiString getUserMapPath(const AsciiString &name);
	static Bool copyToUserMaps(const AsciiString &name, const AsciiString &copyName);
};
