// MacOSGameEngine.mm — macOS game engine following Win32GameEngine structure
//
// 10 of 12 factory methods are identical to Win32GameEngine.
// Only LocalFileSystem, ArchiveFileSystem, WebBrowser, and AudioManager differ.

#import <AppKit/AppKit.h>

#include "MacOSGameEngine.h"

#include "W3DDevice/GameLogic/W3DGameLogic.h"
#include "W3DDevice/GameClient/W3DGameClient.h"
#include "W3DDevice/Common/W3DModuleFactory.h"
#include "W3DDevice/Common/W3DThingFactory.h"
#include "W3DDevice/Common/W3DFunctionLexicon.h"
#include "W3DDevice/Common/W3DRadar.h"
#include "W3DDevice/GameClient/W3DWebBrowser.h"
#include "GameClient/ParticleSys.h"
#include "GameNetwork/NetworkInterface.h"

#include "StdDevice/Common/StdLocalFileSystem.h"
#include "StdDevice/Common/StdBIGFileSystem.h"

#include "GameNetwork/LANAPICallbacks.h"
#include "../OnlineServices_Init.h"
#include "../Audio/MacOSAudioManager.h"

extern DWORD TheMessageTime;

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
	GameEngine::init();
}

void MacOSGameEngine::reset()
{
	GameEngine::reset();
}

// ── update() mirrors Win32GameEngine::update() lines 87-132 ──

void MacOSGameEngine::update()
{
	GameEngine::update();
	serviceWindowsOS();
}

// ── serviceWindowsOS() mirrors Win32GameEngine lines 140-175 ──
// NSEvent polling replaces PeekMessage/GetMessage/DispatchMessage

void MacOSGameEngine::serviceWindowsOS()
{
	@autoreleasepool {
		NSEvent* event;
		while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
		                                  untilDate:nil
		                                     inMode:NSDefaultRunLoopMode
		                                    dequeue:YES])) {
			[NSApp sendEvent:event];
			[NSApp updateWindows];
		}
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

LocalFileSystem* MacOSGameEngine::createLocalFileSystem() { return NEW StdLocalFileSystem; }
ArchiveFileSystem* MacOSGameEngine::createArchiveFileSystem() { return NEW StdBIGFileSystem; }
WebBrowser* MacOSGameEngine::createWebBrowser() { return nullptr; }
AudioManager* MacOSGameEngine::createAudioManager() { return NEW MacOSAudioManager; }
