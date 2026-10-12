// MacOSInstallPaths.mm — install folder discovery shared by the game and WorldBuilder

#import <AppKit/AppKit.h>

#include "MacOSInstallPaths.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <strings.h>
#include <unistd.h>

static NSString* const InstallPathBookmarkKey = @"GeneralsInstallPathBookmark";

// macOS denies reads under ~/Documents, ~/Desktop and ~/Downloads until the user approves them.
// A denied prompt is never shown again, so the game has to say what happened and offer a way out.
static bool InstallPathAccessDenied(const std::string& path)
{
	if (path.empty()) {
		return false;
	}

	if (access(path.c_str(), R_OK | X_OK) == 0) {
		return false;
	}

	return errno == EACCES || errno == EPERM;
}

// A security scoped bookmark keeps the folder the user picked readable across restarts,
// which a plain path cannot do once TCC is involved.
static bool StartAccessingBookmarkedInstallPath(const std::string& path)
{
	NSData* bookmark = [[NSUserDefaults standardUserDefaults] dataForKey:InstallPathBookmarkKey];
	if (bookmark == nil) {
		return false;
	}

	BOOL stale = NO;
	NSError* error = nil;
	NSURL* url = [NSURL URLByResolvingBookmarkData:bookmark
										   options:NSURLBookmarkResolutionWithSecurityScope
									 relativeToURL:nil
							   bookmarkDataIsStale:&stale
											 error:&error];

	if (url == nil || error != nil) {
		return false;
	}

	if (!path.empty() && strcmp([[url path] UTF8String], path.c_str()) != 0) {
		return false;
	}

	return [url startAccessingSecurityScopedResource] == YES;
}

static void SaveInstallPathBookmark(NSURL* url)
{
	NSError* error = nil;
	NSData* bookmark = [url bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
					 includingResourceValuesForKeys:nil
									  relativeToURL:nil
											  error:&error];

	if (bookmark == nil || error != nil) {
		return;
	}

	[[NSUserDefaults standardUserDefaults] setObject:bookmark forKey:InstallPathBookmarkKey];
}

static std::string AskUserForInstallPath(const std::string& deniedPath)
{
	NSAlert* alert = [[NSAlert alloc] init];
	[alert setMessageText:@"No access to the game folder"];
	[alert setInformativeText:[NSString stringWithFormat:
		@"macOS blocked reading:\n%s\n\nChoose the folder to grant access, or open System Settings "
		 "> Privacy & Security > Files and Folders and enable it there.",
		deniedPath.c_str()]];
	[alert addButtonWithTitle:@"Choose Folder…"];
	[alert addButtonWithTitle:@"Open Settings"];
	[alert addButtonWithTitle:@"Quit"];

	const NSModalResponse choice = [alert runModal];

	if (choice == NSAlertSecondButtonReturn) {
		[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:
			@"x-apple.systempreferences:com.apple.preference.security?Privacy_FilesAndFolders"]];
		return std::string();
	}

	if (choice != NSAlertFirstButtonReturn) {
		return std::string();
	}

	NSOpenPanel* panel = [NSOpenPanel openPanel];
	[panel setCanChooseFiles:NO];
	[panel setCanChooseDirectories:YES];
	[panel setAllowsMultipleSelection:NO];
	[panel setMessage:@"Select the folder that contains the Generals installations"];

	if (!deniedPath.empty()) {
		[panel setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:deniedPath.c_str()]]];
	}

	if ([panel runModal] != NSModalResponseOK) {
		return std::string();
	}

	NSURL* picked = [panel URL];
	if (picked == nil) {
		return std::string();
	}

	[picked startAccessingSecurityScopedResource];
	SaveInstallPathBookmark(picked);

	return std::string([[picked path] UTF8String]);
}

static bool DetectGameModes(const std::string& rootPath, std::string& outZH, std::string& outBase)
{
	std::error_code ec;
	auto rootIter = std::filesystem::directory_iterator(rootPath, ec);
	if (ec) {
		printf("DetectGameModes - failed to scan: '%s' (%s)\n", rootPath.c_str(), ec.message().c_str());
		fflush(stdout);
		return false;
	}

	for (auto it = rootIter; it != std::filesystem::directory_iterator(); ) {
		const std::filesystem::directory_entry& entry = *it;

		std::error_code statEc;
		if (!entry.is_directory(statEc) || statEc) {
			it.increment(ec);
			if (ec) {
				break;
			}
			continue;
		}

		bool hasINIZH = false;
		bool hasINI = false;
		std::string subdir = entry.path().string();

		auto subIter = std::filesystem::directory_iterator(subdir, ec);

		for (auto subIt = subIter; !ec && subIt != std::filesystem::directory_iterator(); ) {
			std::error_code subStatEc;
			if (!subIt->is_directory(subStatEc) && !subStatEc) {
				std::string name = subIt->path().filename().string();
				if (strcasecmp(name.c_str(), "INIZH.big") == 0) { hasINIZH = true; }
				if (strcasecmp(name.c_str(), "INI.big") == 0) { hasINI = true; }
			}

			std::error_code subIterEc;
			subIt.increment(subIterEc);
			if (subIterEc) {
				break;
			}
		}

		ec.clear();

		if (hasINIZH && outZH.empty()) {
			outZH = subdir;
		}
		if (hasINI && outBase.empty()) {
			outBase = subdir;
		}

		it.increment(ec);
		if (ec) {
			break;
		}
	}

	printf("DetectGameModes - ZH: '%s', Base: '%s'\n",
		outZH.empty() ? "(not found)" : outZH.c_str(),
		outBase.empty() ? "(not found)" : outBase.c_str());
	fflush(stdout);

	// Each build only needs its own install; the other game may not be present at all.
#if defined(RTS_GENERALS)
	return !outBase.empty();
#else
	return !outZH.empty();
#endif
}

void MacOS_PrepareInstallPaths()
{
	const char* rootPathEnv = getenv("GENERALS_INSTALL_PATH");
	std::string rootPath = rootPathEnv != nullptr ? rootPathEnv : "";

	if (!rootPath.empty()) {
		StartAccessingBookmarkedInstallPath(rootPath);

		if (InstallPathAccessDenied(rootPath)) {
			printf("MacOSGameEngine::init - access denied: '%s'\n", rootPath.c_str());
			fflush(stdout);

			const std::string picked = AskUserForInstallPath(rootPath);
			if (picked.empty()) {
				printf("MacOSGameEngine::init - no readable install path, aborting startup\n");
				fflush(stdout);
				exit(1);
			}

			rootPath = picked;
			setenv("GENERALS_INSTALL_PATH", rootPath.c_str(), 1);
		}
	}

	if (!rootPath.empty()) {
		std::string zhPath, basePath;
		if (!DetectGameModes(rootPath, zhPath, basePath)) {
			// Continuing without a CWD sends the archive scan across the whole file system.
			NSAlert* alert = [[NSAlert alloc] init];
			[alert setMessageText:@"Game installation not found"];
			[alert setInformativeText:[NSString stringWithFormat:
				@"No Generals installation was found in:\n%s", rootPath.c_str()]];
			[alert runModal];

			printf("MacOSGameEngine::init - no installation under '%s', aborting startup\n", rootPath.c_str());
			fflush(stdout);
			exit(1);
		}

		{
#if defined(RTS_GENERALS)
			const std::string& gamePath = basePath;
			const char* gameName = "Generals";
#else
			const std::string& gamePath = zhPath;
			const char* gameName = "Zero Hour";
#endif
			if (chdir(gamePath.c_str()) != 0) {
				printf("MacOSGameEngine::init - chdir FAILED: '%s', aborting startup\n", gamePath.c_str());
				fflush(stdout);
				exit(1);
			}

			printf("MacOSGameEngine::init - CWD set to %s: '%s'\n", gameName, gamePath.c_str());
			fflush(stdout);

			// The file system only ever scans the running game's own install: loose files
			// from the other game would shadow its archives and crash INI parsing on enums
			// that this executable does not know.
			setenv("GENERALS_ACTIVE_INSTALL_PATH", gamePath.c_str(), 1);

			if (!zhPath.empty()) {
				setenv("GENERALS_ZH_INSTALL_PATH", zhPath.c_str(), 1);
			}

			if (!basePath.empty()) {
				setenv("GENERALS_BASE_INSTALL_PATH", basePath.c_str(), 1);
				printf("MacOSGameEngine::init - Base path: '%s'\n", basePath.c_str());
				fflush(stdout);
			}
		}
	}
}
