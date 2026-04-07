// MacOSGameEngine.mm — macOS game engine following Win32GameEngine structure
//
// 10 of 12 factory methods are identical to Win32GameEngine.
// Only LocalFileSystem, ArchiveFileSystem, WebBrowser, and AudioManager differ.

#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>

#include "MacOSGameEngine.h"

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
#include "W3DDevice/GameClient/W3DWebBrowser.h"
#include "GameClient/ParticleSys.h"
#include "GameNetwork/NetworkInterface.h"

#include "../System/MacOSLocalFileSystem.h"
#include "StdDevice/Common/StdBIGFileSystem.h"
#include <unistd.h>
#include <strings.h>

#include "GameNetwork/LANAPICallbacks.h"
#include "GameNetwork/GeneralsOnline/OnlineServices_Init.h"
#include "../Audio/MacOSAudioManager.h"

extern DWORD TheMessageTime;

static bool DetectGameModes(const std::string& rootPath, std::string& outZH, std::string& outBase)
{
	std::error_code ec;
	auto rootIter = std::filesystem::directory_iterator(rootPath, ec);
	if (ec) {
		printf("DetectGameModes - failed to scan: '%s'\n", rootPath.c_str());
		fflush(stdout);
		return false;
	}

	for (const auto& entry : rootIter) {
		if (!entry.is_directory()) {
			continue;
		}

		bool hasINIZH = false;
		bool hasINI = false;
		std::string subdir = entry.path().string();

		auto subIter = std::filesystem::directory_iterator(subdir, ec);
		if (ec) {
			continue;
		}

		for (const auto& file : subIter) {
			if (file.is_directory()) {
				continue;
			}
			std::string name = file.path().filename().string();
			if (strcasecmp(name.c_str(), "INIZH.big") == 0) { hasINIZH = true; }
			if (strcasecmp(name.c_str(), "INI.big") == 0) { hasINI = true; }
		}

		if (hasINIZH && outZH.empty()) {
			outZH = subdir;
		} else if (hasINI && !hasINIZH && outBase.empty()) {
			outBase = subdir;
		}
	}

	printf("DetectGameModes - ZH: '%s', Base: '%s'\n",
		outZH.empty() ? "(not found)" : outZH.c_str(),
		outBase.empty() ? "(not found)" : outBase.c_str());
	fflush(stdout);
	return !outZH.empty();
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
	const char* rootPath = getenv("GENERALS_INSTALL_PATH");
	if (rootPath && rootPath[0]) {
		std::string zhPath, basePath;
		if (DetectGameModes(rootPath, zhPath, basePath)) {
			if (chdir(zhPath.c_str()) == 0) {
				printf("MacOSGameEngine::init - CWD set to ZH: '%s'\n", zhPath.c_str());
			} else {
				printf("MacOSGameEngine::init - chdir FAILED: '%s'\n", zhPath.c_str());
			}
			fflush(stdout);

			setenv("GENERALS_ZH_INSTALL_PATH", zhPath.c_str(), 1);

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

void MacOSGameEngine::update()
{
	@autoreleasepool {
		GameEngine::update();
		serviceWindowsOS();
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
			
			unsigned int timeMs = (unsigned int)([event timestamp] * 1000.0);
			NSEventType type = [event type];
			
			if (type == NSEventTypeKeyDown || type == NSEventTypeKeyUp) {
				if (TheMacOSKeyboard) {
					TheMacOSKeyboard->setModifiers([event modifierFlags], timeMs);
					// The user specifically requested to NOT filter out 'isARepeat' right now.
					TheMacOSKeyboard->addEvent([event keyCode], type == NSEventTypeKeyDown, timeMs);
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
        
		// CRITICAL: Because GameMain() runs an infinite loop on the Main Thread (matching Windows),
		// the main RunLoop never returns. Core Animation transactions (which show and update
		// the window) never automatically flush. We must manually flush them!
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
Radar* MacOSGameEngine::createRadar() { return NEW W3DRadar; }

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
AudioManager* MacOSGameEngine::createAudioManager() { return NEW MacOSAudioManager; }
