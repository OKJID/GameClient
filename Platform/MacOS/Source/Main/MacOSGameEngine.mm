// MacOSGameEngine.mm — macOS game engine following Win32GameEngine structure
//
// 10 of 12 factory methods are identical to Win32GameEngine.
// Only LocalFileSystem, ArchiveFileSystem, WebBrowser, and AudioManager differ.

#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>

#include "MacOSGameEngine.h"
#include "MacOSCrashTrail.h"
#include "MacOSHostServices.h"
#include "MacOSHotkeyApply.h"
#include "MacOSHotkeyDump.h"
#include "MacOSHotkeyExport.h"
#include "MacOSHotkeyImport.h"
#include "MacOSInstallPaths.h"

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
#include "GameClient/Shell.h"
#include "GameClient/WindowLayout.h"
#include "GameLogic/GameLogic.h"
#include "Common/GlobalData.h"
#include "Common/System/NativeFileSystem.h"
#include "GameClient/Display.h"
#include "GameClient/GameText.h"
#include "Common/OptionPreferences.h"
#include "MacOSRichMessageBox.h"

extern HWND ApplicationHWnd;

#include "../System/MacOSLocalFileSystem.h"
#include "../Utils/MacDebug.h"
#include "StdDevice/Common/StdBIGFileSystem.h"
#include <unistd.h>
#include <strings.h>
#include <cerrno>
#include <algorithm>
#include <mach/mach.h>

#include "GameNetwork/LANAPICallbacks.h"
#if defined(RTS_ZEROHOUR)
#include "GameNetwork/GeneralsOnline/OnlineServices_Init.h"
#endif
#include "../Audio/MacOSAudioManager.h"

extern DWORD TheMessageTime;
extern "C" bool MacOS_ToggleFullscreen();

static const char* GameModeName(GameMode mode)
{
	switch (mode) {
		case GAME_SINGLE_PLAYER: return "single player";
		case GAME_LAN: return "LAN";
		case GAME_SKIRMISH: return "skirmish";
		case GAME_REPLAY: return "replay";
		case GAME_SHELL: return "shell";
		case GAME_INTERNET: return "online";
		case GAME_NONE: return "none";
	}
	return "unknown";
}

class GameStateTrail
{
public:
	void update()
	{
		if (TheGlobalData != nullptr) {
			markChange(m_mapName, TheGlobalData->m_mapName.str(), "map");
			markChange(m_pendingMap, TheGlobalData->m_pendingFile.str(), "pending map");
		}

		if (TheShell != nullptr) {
			WindowLayout* screen = TheShell->top();
			markChange(m_shellScreen, screen != nullptr ? screen->getFilename().str() : "", "menu screen");
		}

		if (TheGameLogic != nullptr) {
			markChange(m_gameMode, GameModeName(TheGameLogic->getGameMode()), "game mode");
			markMatchRunningChange();
		}
	}

private:
	void markMatchRunningChange()
	{
		const bool isMatchRunning = TheGameLogic->isInInteractiveGame() && TheGameLogic->getFrame() > 0;
		if (isMatchRunning == m_isMatchRunning) {
			return;
		}

		m_isMatchRunning = isMatchRunning;
		MacOSCrashTrail::mark("%s", isMatchRunning ? "match running" : "match ended");
	}

	static void markChange(std::string& remembered, const char* current, const char* what)
	{
		if (remembered == current) {
			return;
		}

		remembered = current;
		MacOSCrashTrail::mark("%s: %s", what, current[0] != '\0' ? current : "(none)");
	}

	std::string m_mapName;
	std::string m_pendingMap;
	std::string m_shellScreen;
	std::string m_gameMode;
	bool m_isMatchRunning = false;
};

static GameStateTrail s_gameStateTrail;

static const char* ThermalStateName(NSProcessInfoThermalState state)
{
	switch (state) {
		case NSProcessInfoThermalStateNominal: return "nominal";
		case NSProcessInfoThermalStateFair: return "fair";
		case NSProcessInfoThermalStateSerious: return "serious";
		case NSProcessInfoThermalStateCritical: return "critical";
	}
	return "unknown";
}

static unsigned long long PhysicalFootprintMB()
{
	task_vm_info_data_t info;
	mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
	if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&info, &count) != KERN_SUCCESS) {
		return 0;
	}

	return info.phys_footprint / (1024 * 1024);
}

class FramePaceTrail
{
public:
	void update()
	{
		const double now = CACurrentMediaTime();
		if (m_minuteStart == 0.0) {
			startMinute(now);
			return;
		}

		recordFrame(now);

		if (now - m_secondStart >= 1.0) {
			closeSecond(now);
		}

		if (now - m_minuteStart >= 60.0) {
			logMinute(now);
			startMinute(now);
		}
	}

private:
	void startMinute(double now)
	{
		m_minuteStart = now;
		m_secondStart = now;
		m_lastFrame = now;
		m_framesThisSecond = 0;
		m_framesThisMinute = 0;
		m_minSecondFps = -1.0;
		m_maxFrameSeconds = 0.0;
		m_logicFrameAtMinuteStart = currentLogicFrame();
	}

	void recordFrame(double now)
	{
		m_maxFrameSeconds = std::max(m_maxFrameSeconds, now - m_lastFrame);
		m_lastFrame = now;
		++m_framesThisSecond;
		++m_framesThisMinute;
	}

	void closeSecond(double now)
	{
		const double secondFps = m_framesThisSecond / (now - m_secondStart);
		if (m_minSecondFps < 0.0 || secondFps < m_minSecondFps) {
			m_minSecondFps = secondFps;
		}

		m_framesThisSecond = 0;
		m_secondStart = now;
		logThermalChange();
		logDisplayChange();
	}

	void logMinute(double now)
	{
		NSProcessInfo* processInfo = [NSProcessInfo processInfo];
		DEBUG_PERF_MAC(("minute fps avg=%.1f min1s=%.1f maxFrameMs=%.1f logicFrames=%u thermal=%s lowPower=%d footprintMB=%llu mode=%s",
			m_framesThisMinute / (now - m_minuteStart),
			m_minSecondFps,
			m_maxFrameSeconds * 1000.0,
			logicFramesThisMinute(),
			ThermalStateName(processInfo.thermalState),
			processInfo.isLowPowerModeEnabled ? 1 : 0,
			PhysicalFootprintMB(),
			TheGameLogic != nullptr ? GameModeName(TheGameLogic->getGameMode()) : "none"));
	}

	void logThermalChange()
	{
		NSProcessInfo* processInfo = [NSProcessInfo processInfo];
		const NSProcessInfoThermalState state = processInfo.thermalState;
		if (state == m_thermalState) {
			return;
		}

		m_thermalState = state;
		DEBUG_PERF_MAC(("thermal=%s lowPower=%d logicFrame=%u",
			ThermalStateName(state),
			processInfo.isLowPowerModeEnabled ? 1 : 0,
			currentLogicFrame()));
	}

	void logDisplayChange()
	{
		char description[256];
		describeDisplay(description, sizeof(description));
		if (m_display == description) {
			return;
		}

		m_display = description;
		DEBUG_PERF_MAC(("display %s", description));
	}

	static void describeDisplay(char* buffer, size_t size)
	{
		NSWindow* window = (__bridge NSWindow*)ApplicationHWnd;
		NSScreen* screen = window != nil ? window.screen : nil;
		if (screen == nil || TheDisplay == nullptr) {
			snprintf(buffer, size, "(none)");
			return;
		}

		snprintf(buffer, size, "screen=\"%s\" points=%.0fx%.0f scale=%.0f maxHz=%ld render=%dx%d windowed=%d",
			screen.localizedName.UTF8String,
			screen.frame.size.width,
			screen.frame.size.height,
			screen.backingScaleFactor,
			(long)screen.maximumFramesPerSecond,
			TheDisplay->getWidth(),
			TheDisplay->getHeight(),
			TheDisplay->getWindowed() ? 1 : 0);
	}

	UnsignedInt logicFramesThisMinute() const
	{
		const UnsignedInt frame = currentLogicFrame();
		if (frame < m_logicFrameAtMinuteStart) {
			return frame;
		}

		return frame - m_logicFrameAtMinuteStart;
	}

	static UnsignedInt currentLogicFrame()
	{
		return TheGameLogic != nullptr ? TheGameLogic->getFrame() : 0;
	}

	double m_minuteStart = 0.0;
	double m_secondStart = 0.0;
	double m_lastFrame = 0.0;
	double m_minSecondFps = -1.0;
	double m_maxFrameSeconds = 0.0;
	int m_framesThisSecond = 0;
	int m_framesThisMinute = 0;
	UnsignedInt m_logicFrameAtMinuteStart = 0;
	NSProcessInfoThermalState m_thermalState = (NSProcessInfoThermalState)-1;
	std::string m_display;
};

static FramePaceTrail s_framePaceTrail;

class SavedFullscreenRestore
{
public:
	void restoreOnFirstFrame()
	{
		if (m_isDone) {
			return;
		}

		m_isDone = true;

		if (TheDisplay == nullptr || !TheDisplay->getWindowed()) {
			return;
		}

		if (!OptionPreferences().getMacFullscreen()) {
			return;
		}

		DEBUG_INFO_MAC(("FULLSCREEN: Options.ini has MacFullscreen = yes, entering full screen on startup"));
		MacOS_ToggleFullscreen();
	}

private:
	bool m_isDone = false;
};

static SavedFullscreenRestore s_savedFullscreenRestore;

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
	MacOS_PrepareInstallPaths();

	GameEngine::init();

	const char* hotkeyEditPath = MacOS_GetCommandLineValue(MacOSHotkeyApply::kFlag);
	const char* hotkeyDumpPath = MacOS_GetCommandLineValue(MacOSHotkeyDump::kFlag);
	const char* hotkeyImportPath = MacOS_GetCommandLineValue(MacOSHotkeyImport::kFlag);
	const char* hotkeyPresetPath = MacOS_GetCommandLineValue(MacOSHotkeyImport::kOutputFlag);
	const char* hotkeyExportPath = MacOS_GetCommandLineValue(MacOSHotkeyExport::kFlag);
	if (hotkeyEditPath == nullptr && hotkeyDumpPath == nullptr && hotkeyImportPath == nullptr && hotkeyExportPath == nullptr)
		return;

	if (hotkeyImportPath != nullptr && hotkeyPresetPath != nullptr)
		MacOSHotkeyImport::write(NativeFileSystem::get_engine_path(hotkeyImportPath), NativeFileSystem::get_engine_path(hotkeyPresetPath));

	if (hotkeyExportPath != nullptr)
		MacOSHotkeyExport::write(NativeFileSystem::get_engine_path(hotkeyExportPath));

	if (hotkeyEditPath != nullptr)
		MacOSHotkeyApply::apply(NativeFileSystem::get_engine_path(hotkeyEditPath));

	if (hotkeyDumpPath != nullptr)
		MacOSHotkeyDump::write(NativeFileSystem::get_engine_path(hotkeyDumpPath));

	setQuitting(TRUE);
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
		s_savedFullscreenRestore.restoreOnFirstFrame();
		GameEngine::update();
		s_gameStateTrail.update();
		s_framePaceTrail.update();
	}
}

static bool IsInsideContentView(NSEvent* event)
{
	NSWindow* window = [event window];
	if (window == nil) {
		return false;
	}

	NSView* contentView = [window contentView];
	const NSPoint location = [contentView convertPoint:[event locationInWindow] fromView:nil];
	return NSPointInRect(location, contentView.bounds);
}

class FullscreenExitHint
{
public:
	void show()
	{
		close();

		if (TheDisplay == nullptr || TheDisplay->getWindowed() || TheGameText == nullptr) {
			return;
		}

		m_box.show(
			TheGameText->fetchOrSubstitute("GUI:MacFullscreenHintTitle", L"Full Screen"),
			TheGameText->fetchOrSubstitute("GUI:MacFullscreenHintBody",
				L"To return to windowed mode, press\n"
				L"{gold}**Control + Command + F**{/}  (⌃⌘F)\n\n"
				L"**Command + Tab** (⌘⇥) does not work in full screen mode."));
	}

	void close()
	{
		m_box.close();
	}

private:
	MacOSRichMessageBox m_box;
};

static FullscreenExitHint s_fullscreenExitHint;

extern "C" void MacOS_ToggleFullscreenWithHint()
{
	if (TheDisplay == nullptr) {
		return;
	}

	s_fullscreenExitHint.close();
	MacOS_ToggleFullscreen();
	s_fullscreenExitHint.show();
}

class FullscreenShortcut
{
public:
	bool consume(NSEvent* event)
	{
		const NSEventType type = [event type];
		if (type != NSEventTypeKeyDown && type != NSEventTypeKeyUp) {
			return false;
		}

		if ([event keyCode] != FKeyCode) {
			return false;
		}

		if (type == NSEventTypeKeyUp) {
			return releaseHeldKey();
		}

		if (!hasShortcutModifiers([event modifierFlags])) {
			return false;
		}

		m_isKeyHeld = true;
		if (![event isARepeat]) {
			MacOS_ToggleFullscreenWithHint();
		}
		return true;
	}

private:
	bool releaseHeldKey()
	{
		const bool wasHeld = m_isKeyHeld;
		m_isKeyHeld = false;
		return wasHeld;
	}

	static bool hasShortcutModifiers(NSEventModifierFlags flags)
	{
		const NSEventModifierFlags shortcutFlags = NSEventModifierFlagControl | NSEventModifierFlagCommand;
		const NSEventModifierFlags consideredFlags = shortcutFlags | NSEventModifierFlagOption | NSEventModifierFlagShift;
		return (flags & consideredFlags) == shortcutFlags;
	}

	static const unsigned short FKeyCode = 3;
	bool m_isKeyHeld = false;
};

static FullscreenShortcut s_fullscreenShortcut;

// ── serviceWindowsOS() mirrors Win32GameEngine lines 140-175 ──
// NSEvent polling replaces PeekMessage/GetMessage/DispatchMessage

void MacOSGameEngine::serviceWindowsOS()
{
	@autoreleasepool {
		NSEvent* event;
		while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
		                                  untilDate:[NSDate distantPast]
		                                     inMode:NSDefaultRunLoopMode
		                                    dequeue:YES])) {
			
			// NSEvent timestamps run on mach_absolute_time, which stops during sleep, while
			// timeGetTime() runs on CLOCK_MONOTONIC. Mixing the two makes Keyboard::checkKeyRepeat()
			// see a multi-hour key hold and repeat on the very next frame.
			unsigned int timeMs = timeGetTime();
			TheMessageTime = timeMs;
			NSEventType type = [event type];
			if (s_fullscreenShortcut.consume(event)) {
				continue;
			}
			
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

							const bool isEnter = (ch == 0x0D);
							const bool isControl = (ch < 32 || ch == 0x7F);
							const bool isFunctionKey = (ch >= 0xF700 && ch <= 0xF8FF);

							if (!isEnter && (isControl || isFunctionKey)) {
								continue;
							}

							// Pass printable characters and Enter (used by GUI textboxes)
							TheIMEManager->serviceIMEMessage(ApplicationHWnd, 0x0102 /* WM_CHAR */, ch, 0);
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
				if (TheMacOSMouse && IsInsideContentView(event)) {
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
				if (TheMacOSMouse && IsInsideContentView(event)) {
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
			} else if (type == NSEventTypeOtherMouseDown && [event buttonNumber] == 2) {
				if (TheMacOSMouse && IsInsideContentView(event)) {
					NSPoint loc = [event locationInWindow];
					if ([event window]) {
						loc.y = NSHeight([[event window] contentView].bounds) - loc.y;
					}
					if ([event clickCount] == 2) {
						TheMacOSMouse->addEvent(MACOS_MOUSE_MBUTTON_DBLCLK, loc.x, loc.y, 3, 0, timeMs);
					} else {
						TheMacOSMouse->addEvent(MACOS_MOUSE_MBUTTON_DOWN, loc.x, loc.y, 3, 0, timeMs);
					}
				}
			} else if (type == NSEventTypeOtherMouseUp && [event buttonNumber] == 2) {
				if (TheMacOSMouse) {
					NSPoint loc = [event locationInWindow];
					if ([event window]) {
						loc.y = NSHeight([[event window] contentView].bounds) - loc.y;
					}
					TheMacOSMouse->addEvent(MACOS_MOUSE_MBUTTON_UP, loc.x, loc.y, 3, 0, timeMs);
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
	virtual AsciiString nextMusicTrack() override { return ""; }
	virtual AsciiString prevMusicTrack() override { return ""; }
	virtual Bool isMusicPlaying() const override { return FALSE; }
	virtual Bool hasMusicTrackCompleted(const AsciiString& trackName, Int numberOfTimes) const override { return FALSE; }
	virtual UnsignedInt getNumAvailable2DSamples() const override { return 0; }
	virtual UnsignedInt getNumAvailable3DSamples() const override { return 0; }
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
