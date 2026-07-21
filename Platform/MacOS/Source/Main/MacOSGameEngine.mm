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

#include "GameNetwork/LANAPICallbacks.h"
#if defined(RTS_ZEROHOUR)
#include "GameNetwork/GeneralsOnline/OnlineServices_Init.h"
#endif
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
		}
		if (hasINI && outBase.empty()) {
			outBase = subdir;
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
	const char* rootPath = getenv("GENERALS_INSTALL_PATH");
	if (rootPath && rootPath[0]) {
		std::string zhPath, basePath;
		if (DetectGameModes(rootPath, zhPath, basePath)) {
#if defined(RTS_GENERALS)
			const std::string& gamePath = basePath;
			const char* gameName = "Generals";
#else
			const std::string& gamePath = zhPath;
			const char* gameName = "Zero Hour";
#endif
			if (chdir(gamePath.c_str()) == 0) {
				printf("MacOSGameEngine::init - CWD set to %s: '%s'\n", gameName, gamePath.c_str());
			} else {
				printf("MacOSGameEngine::init - chdir FAILED: '%s'\n", gamePath.c_str());
			}
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
			
			unsigned int timeMs = (unsigned int)([event timestamp] * 1000.0);
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
