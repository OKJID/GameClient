// MacOSHotkeyImport.cpp — letters of a community hotkey table (.csf, or a .big holding one) for every button label of the profile

#include "MacOSHotkeyImport.h"
#include "MacOSHotkeyDump.h"
#include "MacOSJsonWriter.h"

#include "Common/AsciiString.h"
#include "Common/UnicodeString.h"
#include "Common/ArchiveFile.h"
#include "Common/ArchiveFileSystem.h"
#include "Common/Debug.h"
#include "Common/Registry.h"
#include "Common/System/NativeFileSystem.h"
#include "Common/file.h"
#include "GameClient/GameText.h"
#include "GameClient/HotKey.h"

#include <cctype>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

namespace
{
	const Int kPresetSchema = 1;
	const char* const kArchiveExtension = ".big";
	const char* const kExtractedTableSuffix = ".csf";
	const char* const kArchivedTablePattern = "data\\%s\\generals.csf";
	const char* const kFallbackLanguage = "english";

	using TextsByLabel = std::map<std::string, UnicodeString>;

	std::string lowerCase( std::string value )
	{
		for ( char& c : value )
			c = (char)tolower( (unsigned char)c );
		return value;
	}

	std::string upperCase( const AsciiString& value )
	{
		std::string upper( value.str() );
		for ( char& c : upper )
			c = (char)toupper( (unsigned char)c );
		return upper;
	}

	bool isButtonLetter( const std::string& letter )
	{
		return letter.size() == 1 && letter[0] >= 'A' && letter[0] <= 'Z';
	}

	bool endsWithNoCase( const std::string& value, const std::string& suffix )
	{
		if ( value.size() < suffix.size() )
			return false;

		return lowerCase( value.substr( value.size() - suffix.size() ) ) == lowerCase( suffix );
	}

	std::string fileNameOf( const std::string& enginePath )
	{
		const size_t separator = enginePath.find_last_of( '\\' );
		if ( separator == std::string::npos )
			return enginePath;

		return enginePath.substr( separator + 1 );
	}

	std::string stemOf( const std::string& fileName )
	{
		const size_t dot = fileName.find_last_of( '.' );
		if ( dot == std::string::npos )
			return fileName;

		return fileName.substr( 0, dot );
	}

	std::string archivedTableName( const std::string& language )
	{
		AsciiString name;
		name.format( kArchivedTablePattern, language.c_str() );
		return lowerCase( name.str() );
	}

	File* openArchivedTable( ArchiveFile& archive )
	{
		const Int access = File::READ | File::BINARY;
		const std::string language = lowerCase( GetRegistryLanguage().str() );

		File* table = archive.openFile( archivedTableName( language ).c_str(), access );
		if ( table != nullptr )
			return table;

		return archive.openFile( archivedTableName( kFallbackLanguage ).c_str(), access );
	}

	bool copyToFile( File& source, const std::string& enginePath )
	{
		std::vector<unsigned char> bytes( source.size() );
		const Int readBytes = bytes.empty() ? 0 : source.read( bytes.data(), (Int)bytes.size() );
		source.close();

		if ( readBytes != (Int)bytes.size() )
			return false;

		FILE* file = NativeFileSystem::fopen( enginePath, "wb" );
		if ( file == nullptr )
			return false;

		fwrite( bytes.data(), 1, bytes.size(), file );
		const bool isWritten = ferror( file ) == 0;
		fclose( file );
		return isWritten;
	}

	bool extractArchivedTable( const std::string& archivePath, const std::string& tablePath )
	{
		if ( !NativeFileSystem::exists( archivePath ) )
			return false;

		ArchiveFile* archive = TheArchiveFileSystem->openArchiveFile( archivePath.c_str() );
		if ( archive == nullptr )
			return false;

		File* table = openArchivedTable( *archive );
		const bool isExtracted = table != nullptr && copyToFile( *table, tablePath );
		delete archive;
		return isExtracted;
	}

	bool readTexts( const std::string& tablePath, TextsByLabel& texts )
	{
		AsciiStringVec labels;
		std::vector<UnicodeString> values;
		if ( !TheGameText->readTable( tablePath.c_str(), labels, values ) )
			return false;

		for ( size_t i = 0; i < labels.size(); ++i )
			texts[lowerCase( labels[i].str() )] = values[i];
		return true;
	}

	bool readSourceTexts( const std::string& sourcePath, const std::string& presetPath, TextsByLabel& texts )
	{
		if ( !endsWithNoCase( sourcePath, kArchiveExtension ) )
			return readTexts( sourcePath, texts );

		const std::string tablePath = presetPath + kExtractedTableSuffix;
		const bool isRead = extractArchivedTable( sourcePath, tablePath ) && readTexts( tablePath, texts );
		NativeFileSystem::remove( tablePath );
		return isRead;
	}

	class PresetWriter
	{
	public:
		PresetWriter( const std::string& sourcePath, const TextsByLabel& texts )
			: m_sourcePath( sourcePath ), m_texts( texts ) {}

		void write( MacOSJsonWriter& json )
		{
			json.beginObject();
			json.key( "schema" );
			json.number( kPresetSchema );
			json.key( "name" );
			json.text( stemOf( fileNameOf( m_sourcePath ) ).c_str() );
			json.key( "source" );
			json.text( fileNameOf( m_sourcePath ).c_str() );
			json.key( "game" );
#if defined(RTS_ZEROHOUR)
			json.text( "zh" );
#else
			json.text( "generals" );
#endif
			writeHotkeys( json );
			json.key( "globals" );
			json.beginObject();
			json.endObject();
			json.endObject();
		}

		Int letterCount() const { return m_letterCount; }
		Int labelCount() const { return m_labelCount; }

	private:
		const std::string& m_sourcePath;
		const TextsByLabel& m_texts;
		Int m_letterCount = 0;
		Int m_labelCount = 0;

		void writeHotkeys( MacOSJsonWriter& json )
		{
			json.key( "hotkeys" );
			json.beginObject();
			for ( const std::string& label : MacOSHotkeyDump::buttonLabels() )
			{
				++m_labelCount;
				writeHotkey( json, label );
			}
			json.endObject();
		}

		void writeHotkey( MacOSJsonWriter& json, const std::string& label )
		{
			const TextsByLabel::const_iterator text = m_texts.find( lowerCase( label ) );
			if ( text == m_texts.end() )
				return;

			const std::string letter = upperCase( TheHotKeyManager->searchHotKey( text->second ) );
			if ( !isButtonLetter( letter ) )
				return;

			json.key( label.c_str() );
			json.text( letter.c_str() );
			++m_letterCount;
		}
	};
}

bool MacOSHotkeyImport::write( const std::string& sourcePath, const std::string& presetPath )
{
	TextsByLabel texts;
	if ( !readSourceTexts( sourcePath, presetPath, texts ) )
	{
		DEBUG_INFO_MAC(( "HotkeyImport - no string table in %s", sourcePath.c_str() ));
		return false;
	}

	const std::string temporaryPath = presetPath + ".tmp";
	FILE* file = NativeFileSystem::fopen( temporaryPath, "wb" );
	if ( file == nullptr )
	{
		DEBUG_INFO_MAC(( "HotkeyImport - cannot open %s", temporaryPath.c_str() ));
		return false;
	}

	PresetWriter preset( sourcePath, texts );
	MacOSJsonWriter json( file );
	preset.write( json );
	const bool isWritten = ferror( file ) == 0;
	fclose( file );

	if ( !isWritten || NativeFileSystem::rename( temporaryPath, presetPath ) != 0 )
	{
		DEBUG_INFO_MAC(( "HotkeyImport - failed to write %s", presetPath.c_str() ));
		return false;
	}

	DEBUG_INFO_MAC(( "HotkeyImport - %d of %d labels from %s (%d strings) written to %s", preset.letterCount(), preset.labelCount(), sourcePath.c_str(), (Int)texts.size(), presetPath.c_str() ));
	return true;
}
