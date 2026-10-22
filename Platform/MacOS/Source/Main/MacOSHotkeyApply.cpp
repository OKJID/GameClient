// MacOSHotkeyApply.cpp — writes the launcher's hotkey edits into the profile's string table

#include "MacOSHotkeyApply.h"
#include "MacOSHotkeyDump.h"
#include "MacOSHotkeyEdits.h"
#include "MacOSHotkeyText.h"

#include "Common/AsciiString.h"
#include "Common/UnicodeString.h"
#include "Common/Debug.h"
#include "Common/SubsystemInterface.h"
#include "GameClient/GameText.h"

#include <cctype>
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
	const char* const kOriginalSuffix = ".orig";

	bool fileExists( const std::string& path )
	{
		struct stat info;
		return stat( path.c_str(), &info ) == 0;
	}

	void createParentDirectories( const std::string& filePath )
	{
		for ( size_t slash = filePath.find( '/', 1 ); slash != std::string::npos; slash = filePath.find( '/', slash + 1 ) )
			mkdir( filePath.substr( 0, slash ).c_str(), 0755 );
	}

	bool copyFile( const std::string& from, const std::string& to )
	{
		FILE* source = fopen( from.c_str(), "rb" );
		if ( source == nullptr )
			return false;

		FILE* target = fopen( to.c_str(), "wb" );
		if ( target == nullptr )
		{
			fclose( source );
			return false;
		}

		char buffer[64 * 1024];
		for ( size_t count = fread( buffer, 1, sizeof( buffer ), source ); count > 0; count = fread( buffer, 1, sizeof( buffer ), source ) )
			fwrite( buffer, 1, count, target );

		const bool isCopied = ferror( source ) == 0 && ferror( target ) == 0;
		fclose( source );
		fclose( target );
		return isCopied;
	}

	bool keepOriginal( const std::string& target )
	{
		const std::string original = target + kOriginalSuffix;
		if ( fileExists( original ) )
			return true;

		if ( fileExists( target ) )
			return copyFile( target, original );

		return TheGameText->writeTable( original.c_str() );
	}

	wchar_t keyOf( const std::string& key )
	{
		if ( key.empty() )
			return 0;

		return (wchar_t)toupper( (unsigned char)key[0] );
	}

	Int applyStringEdits( const std::map<std::string, std::string>& hotkeys )
	{
		Int applied = 0;
		for ( const std::pair<const std::string, std::string>& edit : hotkeys )
		{
			Bool exists = FALSE;
			const UnicodeString current = TheGameText->fetch( edit.first.c_str(), &exists );
			if ( !exists )
			{
				DEBUG_INFO_MAC(( "HotkeyApply - label %s not found", edit.first.c_str() ));
				continue;
			}

			const std::wstring updated = MacOSHotkeyText::withKey( std::wstring( current.str() ), keyOf( edit.second ) );
			if ( TheGameText->setLabelText( edit.first.c_str(), UnicodeString( updated.c_str() ) ) )
				++applied;
		}
		return applied;
	}
}

bool MacOSHotkeyApply::apply( const char* editPath )
{
	MacOSHotkeyEdits edits;
	if ( !MacOS_ReadHotkeyEdits( editPath, edits ) )
	{
		DEBUG_INFO_MAC(( "HotkeyApply - cannot read %s", editPath ));
		return false;
	}

	const std::string target = MacOSHotkeyDump::stringsTargetPath();
	createParentDirectories( target );
	if ( !keepOriginal( target ) )
	{
		DEBUG_INFO_MAC(( "HotkeyApply - cannot keep the original of %s", target.c_str() ));
		return false;
	}

	const Int applied = applyStringEdits( edits.hotkeys );
	if ( !TheGameText->writeTable( target.c_str() ) )
	{
		DEBUG_INFO_MAC(( "HotkeyApply - cannot write %s", target.c_str() ));
		return false;
	}

	DEBUG_INFO_MAC(( "HotkeyApply - %d of %d hotkey strings written to %s, %d global edits pending",
		applied, (Int)edits.hotkeys.size(), target.c_str(), (Int)edits.globals.size() ));

	if ( edits.globals.empty() )
		unlink( editPath );

	return true;
}
