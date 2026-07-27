// MacOSGameEngine.mm — macOS game engine following Win32GameEngine structure
//
// 10 of 12 factory methods are identical to Win32GameEngine.
// Only LocalFileSystem, ArchiveFileSystem, WebBrowser, and AudioManager differ.

#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>

#include "MacOSGameEngine.h"

#include "Utility/time_compat.h"

#include "W3DDevice/GameLogic/W3DGameLogic.h"
#include "W3DDevice/GameClient/W3DGameClient.h"
#include "W3DDevice/Common/W3DModuleFactory.h"
#include "W3DDevice/Common/W3DThingFactory.h"
#include "W3DDevice/Common/W3DFunctionLexicon.h"

// Hardware devices
#include "../Input/MacOSKeyboard.h"
#include "../Input/MacOSMouse.h"

extern MacOSKeyboard *TheMacOSKeyboard;
extern MacOSMouse *TheMacOSMouse;
#include "W3DDevice/Common/W3DRadar.h"
#include "Common/Radar.h"
#include "Common/GameAudio.h"
#include "W3DDevice/GameClient/W3DWebBrowser.h"
#include "GameClient/ParticleSys.h"
#include "GameNetwork/NetworkInterface.h"
#include "GameClient/IMEManager.h"

extern HWND ApplicationHWnd;

#include "../System/MacOSLocalFileSystem.h"
#include "StdDevice/Common/StdBIGFileSystem.h"
#include <unistd.h>
#include <strings.h>
#include <cerrno>

#include "GameNetwork/LANAPICallbacks.h"
#if defined(RTS_ZEROHOUR)
#include "GameNetwork/GeneralsOnline/OnlineServices_Init.h"
#endif
#include "../Audio/MacOSAudioManager.h"

extern DWORD TheMessageTime;

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

// ── Constructor/Destructor (mirrors Win32GameEngine) ──

MacOSGameEngine::MacOSGameEngine()
{
}

MacOSGameEngine::~MacOSGameEngine()
{
}

// ── Lifecycle (mirrors Win32GameEngine) ──

void MacOSGameEngine::init()
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

	GameEngine::init();
}

void MacOSGameEngine::reset()
{
	GameEngine::reset();
}

// ── update() mirrors Win32GameEngine::update() lines 87-132 ──
// On Win32, keyboard uses DirectInput (hardware buffer independent of message loop),
// so the order of GameEngine::update() vs serviceWindowsOS() doesn't matter.
// On macOS, MacOSKeyboard ring buffer is filled ONLY by serviceWindowsOS(),
// so we MUST poll events first, then let the engine read the buffer.

void MacOSGameEngine::update()
{
	@autoreleasepool {
		serviceWindowsOS();
		GameEngine::update();
	}
}

// ── serviceWindowsOS() mirrors Win32GameEngine lines 140-175 ──
// NSEvent polling replaces PeekMessage/GetMessage/DispatchMessage

void MacOSGameEngine::serviceWindowsOS()
{
	@autoreleasepool {
		NSEvent* event;
		while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
		                                  untilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]
		                                     inMode:NSDefaultRunLoopMode
		                                    dequeue:YES])) {
			
			// NSEvent timestamps run on mach_absolute_time, which stops during sleep, while
			// timeGetTime() runs on CLOCK_MONOTONIC. Mixing the two makes Keyboard::checkKeyRepeat()
			// see a multi-hour key hold and repeat on the very next frame.
			unsigned int timeMs = timeGetTime();
			TheMessageTime = timeMs;
			NSEventType type = [event type];
			
			if (type == NSEventTypeKeyDown || type == NSEventTypeKeyUp) {
				if (TheMacOSKeyboard) {
					TheMacOSKeyboard->setModifiers([event modifierFlags], timeMs);
					// The user specifically requested to NOT filter out 'isARepeat' right now.
					TheMacOSKeyboard->addEvent([event keyCode], type == NSEventTypeKeyDown, timeMs);
				}
				
				if (type == NSEventTypeKeyDown && TheIMEManager) {
					NSString *chars = [event characters];
					if (chars && [chars length] > 0) {
						for (NSUInteger i = 0; i < [chars length]; i++) {
							unichar ch = [chars characterAtIndex:i];
							// Convert macOS keypad enter (0x03) to standard CR (0x0D)
							if (ch == 0x03 || ch == 0x0A) ch = 0x0D;
							// Pass printable characters and Enter (used by GUI textboxes)
							if (ch >= 32 || ch == 0x0D) {
								TheIMEManager->serviceIMEMessage(ApplicationHWnd, 0x0102 /* WM_CHAR */, ch, 0);
							}
						}
					}
				}
			} else if (type == NSEventTypeFlagsChanged) {
				if (TheMacOSKeyboard) {
					TheMacOSKeyboard->setModifiers([event modifierFlags], timeMs);
				}
			} else if (type == NSEventTypeMouseMoved || type == NSEventTypeLeftMouseDragged || type == NSEventTypeRightMouseDragged || type == NSEventTypeOtherMouseDragged) {
				if (TheMacOSMouse) {
					NSPoint loc = [event locationInWindow];
					if ([event window]) {
						loc.y = NSHeight([[event window] contentView].bounds) - loc.y;
					}
					TheMacOSMouse->addEvent(MACOS_MOUSE_MOVE, loc.x, loc.y, 0, 0, timeMs);
				}
			} else if (type == NSEventTypeLeftMouseDown) {
				if (TheMacOSMouse) {
					NSPoint loc = [event locationInWindow];
					if ([event window]) {
						loc.y = NSHeight([[event window] contentView].bounds) - loc.y;
					}
					if ([event clickCount] == 2) {
						TheMacOSMouse->addEvent(MACOS_MOUSE_LBUTTON_DBLCLK, loc.x, loc.y, 1, 0, timeMs);
					} else {
						TheMacOSMouse->addEvent(MACOS_MOUSE_LBUTTON_DOWN, loc.x, loc.y, 1, 0, timeMs);
					}
				}
			} else if (type == NSEventTypeLeftMouseUp) {
				if (TheMacOSMouse) {
					NSPoint loc = [event locationInWindow];
					if ([event window]) {
						loc.y = NSHeight([[event window] contentView].bounds) - loc.y;
					}
					TheMacOSMouse->addEvent(MACOS_MOUSE_LBUTTON_UP, loc.x, loc.y, 1, 0, timeMs);
				}
			} else if (type == NSEventTypeRightMouseDown) {
				if (TheMacOSMouse) {
					NSPoint loc = [event locationInWindow];
					if ([event window]) {
						loc.y = NSHeight([[event window] contentView].bounds) - loc.y;
					}
					if ([event clickCount] == 2) {
						TheMacOSMouse->addEvent(MACOS_MOUSE_RBUTTON_DBLCLK, loc.x, loc.y, 2, 0, timeMs);
					} else {
						TheMacOSMouse->addEvent(MACOS_MOUSE_RBUTTON_DOWN, loc.x, loc.y, 2, 0, timeMs);
					}
				}
			} else if (type == NSEventTypeRightMouseUp) {
				if (TheMacOSMouse) {
					NSPoint loc = [event locationInWindow];
					if ([event window]) {
						loc.y = NSHeight([[event window] contentView].bounds) - loc.y;
					}
					TheMacOSMouse->addEvent(MACOS_MOUSE_RBUTTON_UP, loc.x, loc.y, 2, 0, timeMs);
				}
			} else if (type == NSEventTypeScrollWheel) {
				if (TheMacOSMouse) {
					NSPoint loc = [event locationInWindow];
					if ([event window]) {
						loc.y = NSHeight([[event window] contentView].bounds) - loc.y;
					}
					int delta = (int)([event scrollingDeltaY] * 120);
					TheMacOSMouse->addEvent(MACOS_MOUSE_WHEEL, loc.x, loc.y, 0, delta, timeMs);
				}
			}
			
			[NSApp sendEvent:event];
			[NSApp updateWindows];
		}

		TheMessageTime = 0;
        
		[CATransaction flush];
	}
}

// ── Shared factories (identical to Win32GameEngine lines 90-100) ──

GameLogic* MacOSGameEngine::createGameLogic() { return NEW W3DGameLogic; }
GameClient* MacOSGameEngine::createGameClient() { return NEW W3DGameClient; }
ModuleFactory* MacOSGameEngine::createModuleFactory() { return NEW W3DModuleFactory; }
ThingFactory* MacOSGameEngine::createThingFactory() { return NEW W3DThingFactory; }
FunctionLexicon* MacOSGameEngine::createFunctionLexicon() { return NEW W3DFunctionLexicon; }
NetworkInterface* MacOSGameEngine::createNetwork() { return NetworkInterface::createNetwork(); }
Radar* MacOSGameEngine::createRadar(Bool dummy)
{
	if (dummy) {
		return NEW RadarDummy;
	}
	return NEW W3DRadar;
}

ParticleSystemManager* MacOSGameEngine::createParticleSystemManager(Bool dummy)
{
	if (dummy) {
		return static_cast<ParticleSystemManager*>(NEW ParticleSystemManagerDummy);
	}
	return NEW W3DParticleSystemManager;
}

// ── macOS-specific factories ──

LocalFileSystem* MacOSGameEngine::createLocalFileSystem() { return NEW MacOSLocalFileSystem; }
ArchiveFileSystem* MacOSGameEngine::createArchiveFileSystem() { return NEW StdBIGFileSystem; }
WebBrowser* MacOSGameEngine::createWebBrowser() { return nullptr; }
class MacOSAudioManagerDummy : public MacOSAudioManager
{
public:
	virtual void init() override { AudioManager::init(); }
	virtual void stopAudio(AudioAffect which) override {}
	virtual void pauseAudio(AudioAffect which) override {}
	virtual void resumeAudio(AudioAffect which) override {}
	virtual void pauseAmbient(Bool shouldPause) override {}
	virtual void killAudioEventImmediately(AudioHandle audioEvent) override {}
	virtual void nextMusicTrack() override {}
	virtual void prevMusicTrack() override {}
	virtual Bool isMusicPlaying() const override { return FALSE; }
	virtual Bool hasMusicTrackCompleted(const AsciiString& trackName, Int numberOfTimes) const override { return FALSE; }
	virtual AsciiString getMusicTrackName() const override { return ""; }
	virtual void notifyOfAudioCompletion(UnsignedInt audioCompleted, UnsignedInt flags) override {}
	virtual UnsignedInt getProviderCount() const override { return 0; }
	virtual AsciiString getProviderName(UnsignedInt providerNum) const override { return ""; }
	virtual UnsignedInt getProviderIndex(AsciiString providerName) const override { return 0; }
	virtual void selectProvider(UnsignedInt providerNdx) override {}
	virtual void unselectProvider() override {}
	virtual UnsignedInt getSelectedProvider() const override { return 0; }
	virtual void setSpeakerType(UnsignedInt speakerType) override {}
	virtual UnsignedInt getSpeakerType() override { return 0; }
	virtual UnsignedInt getNum2DSamples() const override { return 0; }
	virtual UnsignedInt getNum3DSamples() const override { return 0; }
	virtual UnsignedInt getNumStreams() const override { return 0; }
	virtual Bool doesViolateLimit(AudioEventRTS* event) const override { return FALSE; }
	virtual Bool isPlayingLowerPriority(AudioEventRTS* event) const override { return FALSE; }
	virtual Bool isPlayingAlready(AudioEventRTS* event) const override { return FALSE; }
	virtual Bool isObjectPlayingVoice(UnsignedInt objID) const override { return FALSE; }
	virtual void adjustVolumeOfPlayingAudio(AsciiString eventName, Real newVolume) override {}
	virtual void removePlayingAudio(AsciiString eventName) override {}
	virtual void removeAllDisabledAudio() override {}
	virtual Bool has3DSensitiveStreamsPlaying() const override { return FALSE; }
	virtual void* getHandleForBink() override { return nullptr; }
	virtual void releaseHandleForBink() override {}
	virtual void friend_forcePlayAudioEventRTS(const AudioEventRTS* eventToPlay) override {}
	virtual void setPreferredProvider(AsciiString providerNdx) override {}
	virtual void setPreferredSpeaker(AsciiString speakerType) override {}
	virtual void setDeviceListenerPosition() override {}
};

AudioManager* MacOSGameEngine::createAudioManager(Bool dummy)
{
	if (dummy) {
		return NEW MacOSAudioManagerDummy;
	}
	return NEW MacOSAudioManager;
}
