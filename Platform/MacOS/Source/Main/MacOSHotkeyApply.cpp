// MacOSHotkeyApply.cpp — writes the launcher's hotkey edits into the profile's string table and CommandMap.ini

#include "MacOSHotkeyApply.h"
#include "MacOSCommandMapText.h"
#include "MacOSHotkeyDump.h"
#include "MacOSHotkeyEdits.h"
#include "MacOSHotkeyText.h"

#include "Common/AsciiString.h"
#include "Common/UnicodeString.h"
#include "Common/Debug.h"
#include "Common/FileSystem.h"
#include "Common/SubsystemInterface.h"
#include "Common/System/NativeFileSystem.h"
#include "Common/file.h"
#include "GameClient/GameText.h"
#include "GameClient/KeyDefs.h"
#include "GameClient/MetaEvent.h"

#include <cctype>
#include <cstdio>
#include <string>
#include <strings.h>
#include <vector>

namespace
{
	const char* const kOriginalSuffix = ".orig";
	const char* const kNoKey = "KEY_NONE";
	const char* const kNoModifiers = "NONE";

	struct GlobalEdit
	{
		MetaMapRec* record;
		Int key;
		Int modifiers;
		MacOSCommandMapEntry entry;
	};

	std::string readText( const std::string& enginePath )
	{
		FILE* file = NativeFileSystem::fopen( enginePath, "rb" );
		if ( file == nullptr )
			return std::string();

		std::string text;
		char buffer[64 * 1024];
		for ( size_t count = fread( buffer, 1, sizeof( buffer ), file ); count > 0; count = fread( buffer, 1, sizeof( buffer ), file ) )
			text.append( buffer, count );

		fclose( file );
		return text;
	}

	bool writeText( const std::string& enginePath, const std::string& text )
	{
		FILE* file = NativeFileSystem::fopen( enginePath, "wb" );
		if ( file == nullptr )
			return false;

		fwrite( text.data(), 1, text.size(), file );
		const bool isWritten = ferror( file ) == 0;
		fclose( file );
		return isWritten;
	}

	std::string effectiveText( const std::string& enginePath )
	{
		File* file = TheFileSystem->openFile( enginePath.c_str(), File::READ | File::BINARY );
		if ( file == nullptr )
			return std::string();

		std::string text( (size_t)file->size(), '\0' );
		const Int count = file->read( &text[0], (Int)text.size() );
		file->close();
		text.resize( count > 0 ? (size_t)count : 0 );
		return text;
	}

	bool copyOriginal( const std::string& target, const std::string& original )
	{
		NativeFileSystem::copy( target, original, std::filesystem::copy_options::overwrite_existing );
		return NativeFileSystem::exists( original );
	}

	bool keepOriginalTable( const std::string& target )
	{
		const std::string original = target + kOriginalSuffix;
		if ( NativeFileSystem::exists( original ) )
			return true;

		if ( NativeFileSystem::exists( target ) )
			return copyOriginal( target, original );

		return TheGameText->writeTable( original.c_str() );
	}

	bool keepOriginalText( const std::string& target, const std::string& originalText )
	{
		const std::string original = target + kOriginalSuffix;
		if ( NativeFileSystem::exists( original ) )
			return true;

		if ( NativeFileSystem::exists( target ) )
			return copyOriginal( target, original );

		return writeText( original, originalText );
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

	bool writeStrings( const std::map<std::string, std::string>& hotkeys )
	{
		const std::string target = MacOSHotkeyDump::stringsTargetPath();
		if ( !keepOriginalTable( target ) )
		{
			DEBUG_INFO_MAC(( "HotkeyApply - cannot keep the original of %s", target.c_str() ));
			return false;
		}

		const Int applied = applyStringEdits( hotkeys );
		if ( !TheGameText->writeTable( target.c_str() ) )
		{
			DEBUG_INFO_MAC(( "HotkeyApply - cannot write %s", target.c_str() ));
			return false;
		}

		DEBUG_INFO_MAC(( "HotkeyApply - %d of %d hotkey strings written to %s", applied, (Int)hotkeys.size(), target.c_str() ));
		return true;
	}

	const char* lookupName( const LookupListRec* list, Int value )
	{
		for ( const LookupListRec* entry = list; entry->name; ++entry )
		{
			if ( entry->value == value )
				return entry->name;
		}
		return "";
	}

	bool lookupValue( const LookupListRec* list, const std::string& name, Int& value )
	{
		for ( const LookupListRec* entry = list; entry->name; ++entry )
		{
			if ( strcasecmp( entry->name, name.c_str() ) != 0 )
				continue;

			value = entry->value;
			return true;
		}
		return false;
	}

	std::string usableInNames( Int usableIn )
	{
		std::string names;
		for ( Int bit = 0; TheCommandUsableInNames[bit]; ++bit )
		{
			if ( ( usableIn & ( 1 << bit ) ) == 0 )
				continue;

			names += names.empty() ? "" : " ";
			names += TheCommandUsableInNames[bit];
		}
		return names;
	}

	bool resolveGlobalEdit( const std::string& name, const MacOSGlobalKeyEdit& edit, GlobalEdit& resolved )
	{
		MetaMapRec* record = TheMetaMap->findMetaMapRecByName( name.c_str() );
		const std::string keyName = edit.key.empty() ? kNoKey : edit.key;
		const std::string modifiersName = edit.modifiers.empty() ? kNoModifiers : edit.modifiers;
		const bool isKnown = record != nullptr
			&& lookupValue( KeyNames, keyName, resolved.key )
			&& lookupValue( ModifierNames, modifiersName, resolved.modifiers );
		if ( !isKnown )
		{
			DEBUG_INFO_MAC(( "HotkeyApply - global %s with %s %s skipped", name.c_str(), keyName.c_str(), modifiersName.c_str() ));
			return false;
		}

		resolved.record = record;
		resolved.entry = { name, keyName, modifiersName, lookupName( TransitionNames, record->m_transition ),
			usableInNames( record->m_usableIn ), lookupName( CategoryListName, record->m_category ) };
		return true;
	}

	std::string sourceOf( const MetaMapRec& record )
	{
		if ( record.m_sourceFile.isEmpty() )
			return MacOSHotkeyDump::defaultCommandMapPath();

		return record.m_sourceFile.str();
	}

	bool writeCommandMap( const std::string& source, const std::vector<GlobalEdit>& edits )
	{
		const std::string target = MacOSHotkeyDump::profileRoot() + source;
		std::string ini = effectiveText( source );
		if ( !keepOriginalText( target, ini ) )
			return false;

		for ( const GlobalEdit& edit : edits )
			ini = MacOSCommandMapText::withEntry( ini, edit.entry );

		return writeText( target, ini );
	}

	Int applyGlobalEdits( const std::map<std::string, MacOSGlobalKeyEdit>& globals )
	{
		std::map<std::string, std::vector<GlobalEdit>> editsBySource;
		for ( const std::pair<const std::string, MacOSGlobalKeyEdit>& edit : globals )
		{
			GlobalEdit resolved;
			if ( resolveGlobalEdit( edit.first, edit.second, resolved ) )
				editsBySource[sourceOf( *resolved.record )].push_back( resolved );
		}

		Int applied = 0;
		for ( const std::pair<const std::string, std::vector<GlobalEdit>>& group : editsBySource )
		{
			if ( !writeCommandMap( group.first, group.second ) )
			{
				DEBUG_INFO_MAC(( "HotkeyApply - cannot write %s", group.first.c_str() ));
				continue;
			}

			for ( const GlobalEdit& edit : group.second )
			{
				edit.record->m_key = (MappableKeyType)edit.key;
				edit.record->m_modState = (MappableKeyModState)edit.modifiers;
				++applied;
			}
		}
		return applied;
	}
}

bool MacOSHotkeyApply::apply( const std::string& editPath )
{
	MacOSHotkeyEdits edits;
	if ( !MacOS_ParseHotkeyEdits( readText( editPath ), edits ) )
	{
		DEBUG_INFO_MAC(( "HotkeyApply - cannot read %s", editPath.c_str() ));
		return false;
	}

	if ( !edits.hotkeys.empty() && !writeStrings( edits.hotkeys ) )
		return false;

	const Int appliedGlobals = applyGlobalEdits( edits.globals );
	DEBUG_INFO_MAC(( "HotkeyApply - %d of %d global keys written", appliedGlobals, (Int)edits.globals.size() ));

	NativeFileSystem::remove( editPath );
	return true;
}
