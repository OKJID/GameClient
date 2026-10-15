#include "WBMacCocoa.h"

#include "WBMacIdentity.h"
#include "WBMacModName.h"

#include <strings.h>

namespace
{

NSString* const kModMedallionEnvironmentKey = @"GENERALS_WB_MOD_MEDALLION";
const char* const kTitleSeparator = " — ";
const CGFloat kBadgeScale = 0.5;
const CGFloat kBadgeMargin = 0.02;

NSImage* bundledIcon(const char* name)
{
	NSString* path = [[NSBundle mainBundle] pathForResource:@(name) ofType:@"png"];
	if (path == nil)
	{
		return nil;
	}
	return [[NSImage alloc] initWithContentsOfFile:path];
}

NSImage* modMedallion()
{
	NSString* path = NSProcessInfo.processInfo.environment[kModMedallionEnvironmentKey];
	if (path.length == 0)
	{
		return nil;
	}
	return [[NSImage alloc] initWithContentsOfFile:path];
}

NSImage* withBadge(NSImage* base, NSImage* badge)
{
	if (base == nil || badge == nil)
	{
		return base;
	}
	const NSSize size = base.size;
	return [NSImage imageWithSize:size flipped:NO drawingHandler:^MacBOOL(NSRect rect) {
		[base drawInRect:rect];
		const CGFloat side = size.width * kBadgeScale;
		const CGFloat margin = size.width * kBadgeMargin;
		[badge drawInRect:NSMakeRect(size.width - side - margin, margin, side, side)];
		return YES;
	}];
}

}

bool WBMacIsModLaunch(int argc, char* argv[])
{
#if defined(WORLDBUILDER_MODS)
	for (int index = 1; index + 1 < argc; ++index)
	{
		if (strcasecmp(argv[index], "-mod") == 0)
		{
			return true;
		}
	}
#endif
	return false;
}

NSImage* WBMacApplicationIcon(bool isModLaunch)
{
#if defined(WORLDBUILDER_MODS)
	if (isModLaunch)
	{
		return withBadge(bundledIcon(WORLDBUILDER_MOD_ICON_NAME), modMedallion());
	}
#endif
	return bundledIcon(WORLDBUILDER_ICON_NAME);
}

NSImage* WBMacSplashImage(const std::string& gameSplashPath, bool isModLaunch)
{
	NSImage* splash = [[NSImage alloc] initWithContentsOfFile:@(gameSplashPath.c_str())];
	if (!isModLaunch)
	{
		return splash;
	}
	return withBadge(splash, modMedallion());
}

std::string WBMacIdentityName()
{
	const std::string modName = WBMacActiveModName();
	return modName.empty() ? WORLDBUILDER_GAME_NAME : modName;
}

std::string WBMacWindowTitle(const std::string& frameTitle)
{
	return frameTitle + kTitleSeparator + WBMacIdentityName();
}
