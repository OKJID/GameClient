// MacOSHotkeyEdits.mm — reads hotkey-dump-edit.json

#import <Foundation/Foundation.h>

#include "MacOSHotkeyEdits.h"

namespace
{
	std::string utf8Of( id value )
	{
		if ( ![value isKindOfClass:[NSString class]] )
			return std::string();

		return std::string( [(NSString*)value UTF8String] );
	}

	void readHotkeys( id node, std::map<std::string, std::string>& hotkeys )
	{
		if ( ![node isKindOfClass:[NSDictionary class]] )
			return;

		NSDictionary* entries = (NSDictionary*)node;
		for ( NSString* label in entries )
			hotkeys[utf8Of( label )] = utf8Of( entries[label] );
	}

	void readGlobals( id node, std::map<std::string, MacOSGlobalKeyEdit>& globals )
	{
		if ( ![node isKindOfClass:[NSDictionary class]] )
			return;

		NSDictionary* entries = (NSDictionary*)node;
		for ( NSString* name in entries )
		{
			id entry = entries[name];
			if ( ![entry isKindOfClass:[NSDictionary class]] )
				continue;

			globals[utf8Of( name )] = { utf8Of( entry[@"key"] ), utf8Of( entry[@"modifiers"] ) };
		}
	}
}

bool MacOS_ReadHotkeyEdits( const char* path, MacOSHotkeyEdits& edits )
{
	@autoreleasepool
	{
		NSData* data = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:path]];
		if ( data == nil )
			return false;

		id root = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
		if ( ![root isKindOfClass:[NSDictionary class]] )
			return false;

		readHotkeys( root[@"hotkeys"], edits.hotkeys );
		readGlobals( root[@"globals"], edits.globals );
		return true;
	}
}
