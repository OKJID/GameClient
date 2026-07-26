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

// ModManifest.h
// Mod package manifest: Mods/<id>/config.json

#pragma once

#include "Common/AsciiString.h"

class ModManifest
{
public:

	static const char *fileName;

	// Reads the manifest into TheWritableGlobalData. Paths are engine paths
	// (backslashes); translation stays inside the file system layer.
	static Bool load( const AsciiString& manifestPath );
};
