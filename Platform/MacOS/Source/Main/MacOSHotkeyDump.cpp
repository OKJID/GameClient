// MacOSHotkeyDump.cpp — factions, command panels, hotkey strings and global keys after engine init

#include "MacOSHotkeyDump.h"

#include "Common/AsciiString.h"
#include "Common/UnicodeString.h"
#include "Common/Debug.h"
#include "Common/GlobalData.h"
#include "Common/MessageStream.h"
#include "Common/PlayerTemplate.h"
#include "Common/Registry.h"
#include "Common/SpecialPower.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "GameClient/ControlBar.h"
#include "GameClient/GameText.h"
#include "GameClient/HotKey.h"
#include "GameClient/KeyDefs.h"
#include "GameClient/MetaEvent.h"
#include "GameLogic/Module/CommandSetUpgrade.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <unistd.h>
#include <vector>

extern const Char *g_csfFile;

namespace
{
	const Int kSchemaVersion = 1;
	const char* const kCommandMapFile = "data\\%s\\CommandMap.ini";
	const char* const kCommandSetUpgradeModule = "CommandSetUpgrade";

	class JsonWriter
	{
		FILE* m_file;

	public:
		explicit JsonWriter( FILE* file ) : m_file( file ) {}

		void beginObject() { openScope( '{' ); }
		void endObject() { closeScope( '}' ); }
		void beginArray() { openScope( '[' ); }
		void endArray() { closeScope( ']' ); }

		void key( const char* name )
		{
			separate();
			writeQuoted( name );
			fputc( ':', m_file );
			m_isAfterKey = true;
		}

		void text( const char* value )
		{
			separate();
			writeQuoted( value );
		}

		void number( Int value )
		{
			separate();
			fprintf( m_file, "%d", value );
		}

		void boolean( bool value )
		{
			separate();
			fputs( value ? "true" : "false", m_file );
		}

		void null()
		{
			separate();
			fputs( "null", m_file );
		}

	private:
		std::vector<bool> m_scopeHasItems;
		bool m_isAfterKey = false;

		void openScope( char bracket )
		{
			separate();
			fputc( bracket, m_file );
			m_scopeHasItems.push_back( false );
		}

		void closeScope( char bracket )
		{
			m_scopeHasItems.pop_back();
			fputc( bracket, m_file );
		}

		void separate()
		{
			if ( m_isAfterKey )
			{
				m_isAfterKey = false;
				return;
			}

			if ( m_scopeHasItems.empty() )
				return;

			if ( m_scopeHasItems.back() )
				fputc( ',', m_file );

			m_scopeHasItems.back() = true;
		}

		void writeQuoted( const char* value )
		{
			fputc( '"', m_file );
			for ( const unsigned char* c = (const unsigned char*)value; *c; ++c )
				writeEscaped( *c );
			fputc( '"', m_file );
		}

		void writeEscaped( unsigned char c )
		{
			if ( c == '"' || c == '\\' )
			{
				fputc( '\\', m_file );
				fputc( c, m_file );
				return;
			}

			if ( c < 0x20 )
			{
				fprintf( m_file, "\\u%04x", c );
				return;
			}

			fputc( c, m_file );
		}
	};

	void appendUtf8( std::string& out, UnsignedInt codePoint )
	{
		if ( codePoint < 0x80 )
		{
			out += (char)codePoint;
			return;
		}

		if ( codePoint < 0x800 )
		{
			out += (char)( 0xC0 | ( codePoint >> 6 ) );
			out += (char)( 0x80 | ( codePoint & 0x3F ) );
			return;
		}

		if ( codePoint < 0x10000 )
		{
			out += (char)( 0xE0 | ( codePoint >> 12 ) );
			out += (char)( 0x80 | ( ( codePoint >> 6 ) & 0x3F ) );
			out += (char)( 0x80 | ( codePoint & 0x3F ) );
			return;
		}

		out += (char)( 0xF0 | ( codePoint >> 18 ) );
		out += (char)( 0x80 | ( ( codePoint >> 12 ) & 0x3F ) );
		out += (char)( 0x80 | ( ( codePoint >> 6 ) & 0x3F ) );
		out += (char)( 0x80 | ( codePoint & 0x3F ) );
	}

	std::string toUtf8( const std::wstring& value )
	{
		std::string utf8;
		for ( wchar_t c : value )
			appendUtf8( utf8, (UnsignedInt)c );
		return utf8;
	}

	std::wstring toWide( const UnicodeString& value )
	{
		return std::wstring( value.str() );
	}

	std::string utf8Of( const UnicodeString& value )
	{
		return toUtf8( toWide( value ) );
	}

	std::wstring withoutHotkeyMarker( std::wstring value )
	{
		const size_t bracket = value.find( L"[&" );
		const bool isBracketMarker = bracket != std::wstring::npos && bracket + 3 < value.size() && value[bracket + 3] == L']';
		if ( isBracketMarker )
		{
			const size_t start = ( bracket > 0 && value[bracket - 1] == L' ' ) ? bracket - 1 : bracket;
			value.erase( start, bracket + 4 - start );
			return value;
		}

		const size_t ampersand = value.find( L'&' );
		if ( ampersand != std::wstring::npos )
			value.erase( ampersand, 1 );

		return value;
	}

	std::string nativePath( const AsciiString& enginePath )
	{
		std::string path( enginePath.str() );
		for ( char& c : path )
		{
			if ( c == '\\' )
				c = '/';
		}
		return path;
	}

	std::string upperCase( const AsciiString& value )
	{
		std::string upper( value.str() );
		for ( char& c : upper )
			c = (char)toupper( (unsigned char)c );
		return upper;
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

	AsciiString languagePath( const char* pattern )
	{
		AsciiString path;
		path.format( pattern, GetRegistryLanguage().str() );
		return path;
	}

	std::string targetRoot()
	{
		if ( TheGlobalData->m_modDir.isNotEmpty() )
			return nativePath( TheGlobalData->m_modDir );

		char workingDirectory[PATH_MAX];
		if ( getcwd( workingDirectory, sizeof( workingDirectory ) ) == nullptr )
			return "";

		return std::string( workingDirectory ) + "/";
	}

	enum PanelKind
	{
		PANEL_OBJECT,
		PANEL_SHORTCUT,
		PANEL_SCIENCE,
	};

	const char* panelKindName( PanelKind kind )
	{
		switch ( kind )
		{
			case PANEL_SHORTCUT: return "shortcut";
			case PANEL_SCIENCE: return "science";
			case PANEL_OBJECT: return "object";
		}
		return "object";
	}

	struct PanelSlot
	{
		Int slot;
		AsciiString button;
		AsciiString label;
		bool needsScience;
	};

	struct CommandPanel
	{
		PanelKind kind;
		std::set<std::string> owners;
		std::vector<PanelSlot> slots;
	};

	struct HotkeyString
	{
		std::string key;
		std::wstring text;
		std::wstring description;
		AsciiString sourceFile;
		AsciiString sourceLayer;
	};

	struct PlayableSide
	{
		const PlayerTemplate* playerTemplate;
		std::set<std::string> panels;
	};

	bool isRegisteredOnPanel( const CommandButton* button )
	{
		if ( button == nullptr )
			return false;

		if ( BitIsSet( button->getOptions(), SCRIPT_ONLY ) )
			return false;

		return button->getCommandType() != GUI_COMMAND_EXIT_CONTAINER;
	}

	bool isHiddenUntilScience( const CommandButton* button )
	{
		if ( !BitIsSet( button->getOptions(), NEED_SPECIAL_POWER_SCIENCE ) )
			return false;

		const SpecialPowerTemplate* power = button->getSpecialPowerTemplate();
		if ( power == nullptr || power->getRequiredScience() == SCIENCE_INVALID )
			return false;

		switch ( button->getCommandType() )
		{
			case GUI_COMMAND_PURCHASE_SCIENCE:
			case GUI_COMMAND_PLAYER_UPGRADE:
			case GUI_COMMAND_OBJECT_UPGRADE:
				return false;
			default:
				return true;
		}
	}

	void appendCommandSet( std::vector<AsciiString>& names, const AsciiString& name )
	{
		if ( name.isNotEmpty() )
			names.push_back( name );
	}

	std::vector<AsciiString> commandSetsOf( const ThingTemplate* thing )
	{
		std::vector<AsciiString> names;
		appendCommandSet( names, thing->friend_getCommandSetString() );

		const ModuleInfo& modules = thing->getBehaviorModuleInfo();
		for ( Int i = 0; i < modules.getCount(); ++i )
		{
			if ( modules.getNthName( i ).compareNoCase( kCommandSetUpgradeModule ) != 0 )
				continue;

			const CommandSetUpgradeModuleData* data = static_cast<const CommandSetUpgradeModuleData*>( modules.getNthData( i ) );
			appendCommandSet( names, data->m_newCommandSet );
#if defined(RTS_ZEROHOUR)
			appendCommandSet( names, data->m_newCommandSetAlt );
#endif
		}

		return names;
	}

	class HotkeyCatalog
	{
	public:
		void collect()
		{
			collectSides();
			collectObjectPanels();
			collectPlayerPanels();
		}

		void write( JsonWriter& json ) const
		{
			json.beginObject();
			json.key( "schema" );
			json.number( kSchemaVersion );
			writeProfile( json );
			writeTargets( json );
			writeGlobals( json );
			writeHotkeys( json );
			writePanels( json );
			writeFactions( json );
			json.endObject();
		}

		Int sideCount() const { return (Int)m_sides.size(); }
		Int panelCount() const { return (Int)m_panels.size(); }
		Int hotkeyCount() const { return (Int)m_hotkeys.size(); }

	private:
		std::vector<PlayableSide> m_sides;
		std::map<std::string, CommandPanel> m_panels;
		std::map<std::string, HotkeyString> m_hotkeys;

		void collectSides()
		{
			for ( Int i = 0; i < ThePlayerTemplateStore->getPlayerTemplateCount(); ++i )
			{
				const PlayerTemplate* playerTemplate = ThePlayerTemplateStore->getNthPlayerTemplate( i );
				if ( playerTemplate == nullptr || !playerTemplate->isPlayableSide() )
					continue;

				m_sides.push_back( { playerTemplate, {} } );
			}
		}

		void collectObjectPanels()
		{
			for ( const ThingTemplate* thing = TheThingFactory->firstTemplate(); thing; thing = thing->friend_getNextTemplate() )
			{
				const std::vector<AsciiString> commandSets = commandSetsOf( thing );
				if ( commandSets.empty() )
					continue;

				for ( PlayableSide& side : m_sides )
				{
					if ( side.playerTemplate->getSide() == thing->getDefaultOwningSide() )
						addSidePanels( side, commandSets, PANEL_OBJECT, thing->getDisplayName() );
				}
			}
		}

		void collectPlayerPanels()
		{
			for ( PlayableSide& side : m_sides )
			{
				const PlayerTemplate* player = side.playerTemplate;
				addSidePanel( side, player->getSpecialPowerShortcutCommandSet(), PANEL_SHORTCUT, player->getDisplayName() );
				addSidePanel( side, player->getPurchaseScienceCommandSetRank1(), PANEL_SCIENCE, player->getDisplayName() );
				addSidePanel( side, player->getPurchaseScienceCommandSetRank3(), PANEL_SCIENCE, player->getDisplayName() );
				addSidePanel( side, player->getPurchaseScienceCommandSetRank8(), PANEL_SCIENCE, player->getDisplayName() );
			}
		}

		void addSidePanels( PlayableSide& side, const std::vector<AsciiString>& commandSets, PanelKind kind, const UnicodeString& owner )
		{
			for ( const AsciiString& commandSet : commandSets )
				addSidePanel( side, commandSet, kind, owner );
		}

		void addSidePanel( PlayableSide& side, const AsciiString& commandSetName, PanelKind kind, const UnicodeString& owner )
		{
			CommandPanel* panel = findOrCreatePanel( commandSetName, kind );
			if ( panel == nullptr )
				return;

			if ( !owner.isEmpty() )
				panel->owners.insert( utf8Of( owner ) );

			side.panels.insert( commandSetName.str() );
		}

		CommandPanel* findOrCreatePanel( const AsciiString& commandSetName, PanelKind kind )
		{
			if ( commandSetName.isEmpty() )
				return nullptr;

			const std::map<std::string, CommandPanel>::iterator existing = m_panels.find( commandSetName.str() );
			if ( existing != m_panels.end() )
				return &existing->second;

			const CommandSet* commandSet = TheControlBar->findCommandSet( commandSetName );
			if ( commandSet == nullptr )
				return nullptr;

			CommandPanel& panel = m_panels[commandSetName.str()];
			panel.kind = kind;
			collectSlots( *commandSet, panel );
			return &panel;
		}

		void collectSlots( const CommandSet& commandSet, CommandPanel& panel )
		{
			for ( Int i = 0; i < MAX_COMMANDS_PER_SET; ++i )
			{
				const CommandButton* button = commandSet.getCommandButton( i );
				if ( !isRegisteredOnPanel( button ) )
					continue;

				const AsciiString key = TheHotKeyManager->searchHotKey( button->getTextLabel() );
				if ( key.isEmpty() )
					continue;

				const bool needsScience = panel.kind == PANEL_OBJECT && isHiddenUntilScience( button );
				panel.slots.push_back( { i + 1, button->getName(), button->getTextLabel(), needsScience } );
				recordHotkey( *button, key );
			}
		}

		void recordHotkey( const CommandButton& button, const AsciiString& key )
		{
			const std::string label( button.getTextLabel().str() );
			if ( m_hotkeys.count( label ) > 0 )
				return;

			HotkeyString hotkey;
			hotkey.key = upperCase( key );
			hotkey.text = toWide( TheGameText->fetch( button.getTextLabel() ) );
			if ( button.getDescriptionLabel().isNotEmpty() )
				hotkey.description = toWide( TheGameText->fetch( button.getDescriptionLabel() ) );

			TheGameText->findLabelSource( label.c_str(), hotkey.sourceFile, hotkey.sourceLayer );
			m_hotkeys[label] = hotkey;
		}

		void writeProfile( JsonWriter& json ) const
		{
			json.key( "profile" );
			json.beginObject();
			json.key( "game" );
#if defined(RTS_ZEROHOUR)
			json.text( "zh" );
#else
			json.text( "generals" );
#endif
			json.key( "mod" );
			writeModDirectory( json );
			json.key( "language" );
			json.text( GetRegistryLanguage().str() );
			json.endObject();
		}

		void writeModDirectory( JsonWriter& json ) const
		{
			if ( TheGlobalData->m_modDir.isEmpty() )
			{
				json.null();
				return;
			}

			json.text( nativePath( TheGlobalData->m_modDir ).c_str() );
		}

		void writeTargets( JsonWriter& json ) const
		{
			json.key( "targets" );
			json.beginObject();
			json.key( "root" );
			json.text( targetRoot().c_str() );
			json.key( "strings" );
			json.text( nativePath( languagePath( g_csfFile ) ).c_str() );
			json.key( "globals" );
			json.text( nativePath( languagePath( kCommandMapFile ) ).c_str() );
			json.endObject();
		}

		void writeGlobals( JsonWriter& json ) const
		{
			std::set<std::string> writtenIds;
			json.key( "globals" );
			json.beginObject();
			for ( const MetaMapRec* map = TheMetaMap->getFirstMetaMapRec(); map; map = map->m_next )
			{
				const std::string id( MetaMap::getMetaName( map->m_meta ) );
				if ( !writtenIds.insert( id ).second )
					continue;

				json.key( id.c_str() );
				writeGlobal( json, *map );
			}
			json.endObject();
		}

		void writeGlobal( JsonWriter& json, const MetaMapRec& map ) const
		{
			json.beginObject();
			json.key( "key" );
			json.text( lookupName( KeyNames, map.m_key ) );
			json.key( "modifiers" );
			json.text( lookupName( ModifierNames, map.m_modState ) );
			json.key( "transition" );
			json.text( lookupName( TransitionNames, map.m_transition ) );
			json.key( "category" );
			json.text( lookupName( CategoryListName, map.m_category ) );
			json.key( "usableIn" );
			writeUsableIn( json, map.m_usableIn );
			json.key( "name" );
			json.text( utf8Of( map.m_displayName ).c_str() );
			json.key( "description" );
			json.text( utf8Of( map.m_description ).c_str() );
			json.key( "source" );
			writeGlobalSource( json, map );
			json.endObject();
		}

		void writeGlobalSource( JsonWriter& json, const MetaMapRec& map ) const
		{
			if ( map.m_sourceFile.isEmpty() )
			{
				json.null();
				return;
			}

			json.beginObject();
			json.key( "file" );
			json.text( nativePath( map.m_sourceFile ).c_str() );
			json.endObject();
		}

		void writeUsableIn( JsonWriter& json, Int usableIn ) const
		{
			json.beginArray();
			for ( Int bit = 0; TheCommandUsableInNames[bit]; ++bit )
			{
				if ( usableIn & ( 1 << bit ) )
					json.text( TheCommandUsableInNames[bit] );
			}
			json.endArray();
		}

		void writeHotkeys( JsonWriter& json ) const
		{
			json.key( "hotkeys" );
			json.beginObject();
			for ( const std::pair<const std::string, HotkeyString>& entry : m_hotkeys )
			{
				json.key( entry.first.c_str() );
				writeHotkey( json, entry.second );
			}
			json.endObject();
		}

		void writeHotkey( JsonWriter& json, const HotkeyString& hotkey ) const
		{
			json.beginObject();
			json.key( "key" );
			json.text( hotkey.key.c_str() );
			json.key( "name" );
			json.text( toUtf8( withoutHotkeyMarker( hotkey.text ) ).c_str() );
			json.key( "text" );
			json.text( toUtf8( hotkey.text ).c_str() );
			json.key( "description" );
			json.text( toUtf8( hotkey.description ).c_str() );
			json.key( "source" );
			json.beginObject();
			json.key( "file" );
			json.text( nativePath( hotkey.sourceFile ).c_str() );
			json.key( "layer" );
			json.text( hotkey.sourceLayer.str() );
			json.endObject();
			json.endObject();
		}

		void writePanels( JsonWriter& json ) const
		{
			json.key( "panels" );
			json.beginObject();
			for ( const std::pair<const std::string, CommandPanel>& entry : m_panels )
			{
				json.key( entry.first.c_str() );
				writePanel( json, entry.second );
			}
			json.endObject();
		}

		void writePanel( JsonWriter& json, const CommandPanel& panel ) const
		{
			json.beginObject();
			json.key( "kind" );
			json.text( panelKindName( panel.kind ) );
			json.key( "owners" );
			json.beginArray();
			for ( const std::string& owner : panel.owners )
				json.text( owner.c_str() );
			json.endArray();
			json.key( "buttons" );
			json.beginArray();
			for ( const PanelSlot& slot : panel.slots )
				writePanelSlot( json, slot );
			json.endArray();
			json.endObject();
		}

		void writePanelSlot( JsonWriter& json, const PanelSlot& slot ) const
		{
			json.beginObject();
			json.key( "slot" );
			json.number( slot.slot );
			json.key( "button" );
			json.text( slot.button.str() );
			json.key( "label" );
			json.text( slot.label.str() );
			json.key( "needsScience" );
			json.boolean( slot.needsScience );
			json.endObject();
		}

		void writeFactions( JsonWriter& json ) const
		{
			json.key( "factions" );
			json.beginArray();
			for ( const AsciiString& baseSide : baseSidesInOrder() )
				writeFaction( json, baseSide );
			json.endArray();
		}

		std::vector<AsciiString> baseSidesInOrder() const
		{
			std::vector<AsciiString> baseSides;
			for ( const PlayableSide& side : m_sides )
			{
				const AsciiString baseSide = side.playerTemplate->getBaseSide();
				if ( std::find( baseSides.begin(), baseSides.end(), baseSide ) == baseSides.end() )
					baseSides.push_back( baseSide );
			}
			return baseSides;
		}

		void writeFaction( JsonWriter& json, const AsciiString& baseSide ) const
		{
			json.beginObject();
			json.key( "baseSide" );
			json.text( baseSide.str() );
			json.key( "sides" );
			json.beginArray();
			for ( const PlayableSide& side : m_sides )
			{
				if ( side.playerTemplate->getBaseSide() == baseSide )
					writeSide( json, side );
			}
			json.endArray();
			json.endObject();
		}

		void writeSide( JsonWriter& json, const PlayableSide& side ) const
		{
			json.beginObject();
			json.key( "side" );
			json.text( side.playerTemplate->getSide().str() );
			json.key( "template" );
			json.text( side.playerTemplate->getName().str() );
			json.key( "name" );
			json.text( utf8Of( side.playerTemplate->getDisplayName() ).c_str() );
			json.key( "panels" );
			json.beginArray();
			for ( const std::string& panel : side.panels )
				json.text( panel.c_str() );
			json.endArray();
			json.endObject();
		}
	};
}

bool MacOSHotkeyDump::write( const char* path )
{
	HotkeyCatalog catalog;
	catalog.collect();

	const std::string temporaryPath = std::string( path ) + ".tmp";
	FILE* file = fopen( temporaryPath.c_str(), "wb" );
	if ( file == nullptr )
	{
		DEBUG_INFO_MAC(( "HotkeyDump - cannot open %s", temporaryPath.c_str() ));
		return false;
	}

	JsonWriter json( file );
	catalog.write( json );
	const bool isWritten = ferror( file ) == 0;
	fclose( file );

	if ( !isWritten || rename( temporaryPath.c_str(), path ) != 0 )
	{
		DEBUG_INFO_MAC(( "HotkeyDump - failed to write %s", path ));
		return false;
	}

	DEBUG_INFO_MAC(( "HotkeyDump - %d sides, %d panels, %d hotkeys written to %s", catalog.sideCount(), catalog.panelCount(), catalog.hotkeyCount(), path ));
	return true;
}
