// MacOSHotkeyExport.cpp — the applied table as the engine loaded it: .csf alone, or a .big with the
// string table and every CommandMap.ini the global keys came from

#include "MacOSHotkeyExport.h"
#include "MacOSBigWriter.h"

#include "Common/AsciiString.h"
#include "Common/Debug.h"
#include "Common/FileSystem.h"
#include "Common/Registry.h"
#include "Common/System/NativeFileSystem.h"
#include "Common/file.h"
#include "GameClient/GameText.h"
#include "GameClient/KeyDefs.h"
#include "GameClient/MetaEvent.h"

#include <cctype>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

namespace
{
	const char* const kArchiveExtension = ".big";
	const char* const kTemporarySuffix = ".tmp";
	const char* const kArchivedTablePattern = "Data\\%s\\generals.csf";

	bool endsWithNoCase( const std::string& value, const std::string& suffix )
	{
		if ( value.size() < suffix.size() )
			return false;

		for ( size_t i = 0; i < suffix.size(); ++i )
		{
			const unsigned char left = (unsigned char)value[value.size() - suffix.size() + i];
			if ( tolower( left ) != tolower( (unsigned char)suffix[i] ) )
				return false;
		}
		return true;
	}

	std::vector<unsigned char> readNative( const std::string& enginePath )
	{
		std::vector<unsigned char> bytes;
		FILE* file = NativeFileSystem::fopen( enginePath, "rb" );
		if ( file == nullptr )
			return bytes;

		unsigned char buffer[64 * 1024];
		for ( size_t count = fread( buffer, 1, sizeof( buffer ), file ); count > 0; count = fread( buffer, 1, sizeof( buffer ), file ) )
			bytes.insert( bytes.end(), buffer, buffer + count );

		fclose( file );
		return bytes;
	}

	std::vector<unsigned char> readEffective( const AsciiString& enginePath )
	{
		std::vector<unsigned char> bytes;
		File* file = TheFileSystem->openFile( enginePath.str(), File::READ | File::BINARY );
		if ( file == nullptr )
			return bytes;

		bytes.resize( (size_t)file->size() );
		const Int count = bytes.empty() ? 0 : file->read( bytes.data(), (Int)bytes.size() );
		file->close();
		bytes.resize( count > 0 ? (size_t)count : 0 );
		return bytes;
	}

	std::string archivedTablePath()
	{
		AsciiString path;
		path.format( kArchivedTablePattern, GetRegistryLanguage().str() );
		return path.str();
	}

	std::set<AsciiString> commandMapSources()
	{
		std::set<AsciiString> sources;
		for ( const MetaMapRec* map = TheMetaMap->getFirstMetaMapRec(); map; map = map->m_next )
		{
			if ( map->m_sourceFile.isNotEmpty() )
				sources.insert( map->m_sourceFile );
		}
		return sources;
	}

	bool appendCommandMaps( std::vector<MacOSBigWriter::Entry>& entries )
	{
		for ( const AsciiString& source : commandMapSources() )
		{
			std::vector<unsigned char> bytes = readEffective( source );
			if ( bytes.empty() )
				return false;

			entries.push_back( { source.str(), bytes } );
		}
		return true;
	}

	bool writeArchive( const std::string& path )
	{
		const std::string tablePath = path + ".csf" + kTemporarySuffix;
		const bool isTableWritten = TheGameText->writeTable( tablePath.c_str() );
		std::vector<MacOSBigWriter::Entry> entries = { { archivedTablePath(), readNative( tablePath ) } };
		NativeFileSystem::remove( tablePath );

		if ( !isTableWritten || entries.front().bytes.empty() || !appendCommandMaps( entries ) )
			return false;

		DEBUG_INFO_MAC(( "HotkeyExport - %d files packed", (Int)entries.size() ));
		return MacOSBigWriter::write( path, entries );
	}

	bool writeExport( const std::string& path, const std::string& temporaryPath )
	{
		if ( endsWithNoCase( path, kArchiveExtension ) )
			return writeArchive( temporaryPath );

		return TheGameText->writeTable( temporaryPath.c_str() );
	}
}

bool MacOSHotkeyExport::write( const std::string& path )
{
	const std::string temporaryPath = path + kTemporarySuffix;
	if ( !writeExport( path, temporaryPath ) || NativeFileSystem::rename( temporaryPath, path ) != 0 )
	{
		NativeFileSystem::remove( temporaryPath );
		DEBUG_INFO_MAC(( "HotkeyExport - failed to write %s", path.c_str() ));
		return false;
	}

	DEBUG_INFO_MAC(( "HotkeyExport - applied hotkeys written to %s", path.c_str() ));
	return true;
}
