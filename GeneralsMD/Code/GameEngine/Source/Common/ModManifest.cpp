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

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/ModManifest.h"
#include "Common/GlobalData.h"
#include "Common/System/NativeFileSystem.h"

#include "GameNetwork/GeneralsOnline/json.hpp"

const char *ModManifest::fileName = "config.json";

static Bool readWholeFile( const AsciiString& path, std::string& contents )
{
	FILE *file = NativeFileSystem::fopen( path.str(), "rb" );
	if (file == nullptr)
		return FALSE;

	fseek( file, 0, SEEK_END );
	long size = ftell( file );
	fseek( file, 0, SEEK_SET );

	if (size <= 0)
	{
		fclose( file );
		return FALSE;
	}

	contents.resize( (size_t)size );
	size_t read = fread( &contents[0], 1, (size_t)size, file );
	fclose( file );

	contents.resize( read );
	return read > 0;
}

static AsciiString manifestString( const nlohmann::json& manifest, const char *key )
{
	nlohmann::json::const_iterator it = manifest.find( key );
	if (it == manifest.end() || !it->is_string())
		return AsciiString::TheEmptyString;

	return AsciiString( it->get<std::string>().c_str() );
}

static Bool manifestBool( const nlohmann::json& manifest, const char *key, Bool defaultValue )
{
	nlohmann::json::const_iterator it = manifest.find( key );
	if (it == manifest.end() || !it->is_boolean())
		return defaultValue;

	return it->get<bool>() ? TRUE : FALSE;
}

Bool ModManifest::load( const AsciiString& manifestPath )
{
	std::string contents;
	if (!readWholeFile( manifestPath, contents ))
	{
		DEBUG_LOG(( "ModManifest: cannot read '%s'.", manifestPath.str() ));
		return FALSE;
	}

	nlohmann::json manifest;
	try
	{
		manifest = nlohmann::json::parse( contents );
	}
	catch (...)
	{
		DEBUG_LOG(( "ModManifest: '%s' is not valid JSON.", manifestPath.str() ));
		return FALSE;
	}

	if (!manifest.is_object())
	{
		DEBUG_LOG(( "ModManifest: '%s' is not a JSON object.", manifestPath.str() ));
		return FALSE;
	}

	AsciiString id = manifestString( manifest, "id" );
	if (id.isEmpty())
	{
		DEBUG_LOG(( "ModManifest: '%s' has no id.", manifestPath.str() ));
		return FALSE;
	}

	AsciiString baseGame = manifestString( manifest, "baseGame" );
	if (!baseGame.isEmpty() && baseGame.compareNoCase( "zh" ) != 0)
	{
		DEBUG_LOG(( "ModManifest: '%s' targets baseGame '%s', not Zero Hour.",
			manifestPath.str(), baseGame.str() ));
		return FALSE;
	}

	TheWritableGlobalData->m_modId = id;
	TheWritableGlobalData->m_modDisplayName = manifestString( manifest, "displayName" );
	TheWritableGlobalData->m_modVersion = manifestString( manifest, "version" );
	TheWritableGlobalData->m_modOnline = manifestBool( manifest, "online", TRUE );
	TheWritableGlobalData->m_modMaskBaseScripts = manifestBool( manifest, "maskBaseScripts", FALSE );

	DEBUG_LOG(( "ModManifest: id='%s' version='%s' online=%d maskBaseScripts=%d",
		TheWritableGlobalData->m_modId.str(),
		TheWritableGlobalData->m_modVersion.str(),
		TheWritableGlobalData->m_modOnline,
		TheWritableGlobalData->m_modMaskBaseScripts ));

	return TRUE;
}
