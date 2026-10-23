// MacOSMain.mm — macOS entry point following WinMain.cpp flow
//
// This file mirrors the initialization sequence of WinMain.cpp (lines 797-973)
// with platform-specific substitutions for macOS.

#define __INTLRESOURCES__
#define __FINDER__
#define __AIFF__

#define Byte MacByte
#define RGBColor MacRGBColor
#define BOOL MacBOOL
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#undef Byte
#undef RGBColor
#undef BOOL

#include <cstdlib>
#include <cstring>
#include <clocale>
#include <ctime>
#include <unistd.h>

#include "WWLib/always.h"
#include <windows.h>

#include "Lib/BaseType.h"
#include "Common/AsciiString.h"
#include "Common/CommandLine.h"
#include "Common/CriticalSection.h"
#include "Common/GlobalData.h"
#include "Common/GameEngine.h"
#include "Common/GameMemory.h"
#include "Common/Debug.h"
#include "Common/INI.h"
#include "Common/System/NativeFileSystem.h"
#include "Common/version.h"
#include "GameClient/ClientInstance.h"
#include "GameClient/Mouse.h"
#include "BuildVersion.h"
#include "GeneratedVersion.h"
#include "MacOSCrashHandler.h"
#include "MacOSCrashTrail.h"
#include "MacOSHostServices.h"
#include "MacOSHotkeyApply.h"
#include "MacOSHotkeyDump.h"
#include "MacOSHotkeyExport.h"
#include "MacOSHotkeyImport.h"

#if defined(RTS_ZEROHOUR)
#include "GameNetwork/GeneralsOnline/OnlineServices_Init.h"
#endif

// ── Globals (mirrors WinMain.cpp lines 77-88) ──

HINSTANCE ApplicationHInstance = nullptr;
HWND ApplicationHWnd = nullptr;
DWORD TheMessageTime = 0;

const Char* g_strFile = "data/Generals.str";
const Char* g_csfFile = "data/%s/Generals.csf";
const char* gAppPrefix = "";

static Bool isAppActive = true;

// ── External declarations (mirrors WinMain.cpp) ──

extern GameEngine* CreateGameEngine();
extern Int GameMain();

// ── Critical sections (mirrors WinMain.cpp line 773) ──

static CriticalSection critSec1, critSec2, critSec3, critSec4, critSec5;

static void observeSystemSleep() {
    NSNotificationCenter* center = [[NSWorkspace sharedWorkspace] notificationCenter];
    [center addObserverForName:NSWorkspaceWillSleepNotification object:nil queue:nil usingBlock:^(NSNotification*) {
        MacOSCrashTrail::mark("system going to sleep");
    }];
    [center addObserverForName:NSWorkspaceDidWakeNotification object:nil queue:nil usingBlock:^(NSNotification*) {
        MacOSCrashTrail::mark("system woke up");
    }];
}

// ── External engine bridges ──

extern "C" void MacOS_ApplyDisplayResolution(int w, int h, bool isWindowed);
extern "C" void MacOS_UpdateMetalDeviceScreenSize(int width, int height);
extern "C" void MacOS_DiscardPendingMouseDown();

// ── NSApplication delegate ──

@interface GeneralsAppDelegate : NSObject<NSApplicationDelegate, NSWindowDelegate>
@property (strong) NSWindow* window;
@end

@implementation GeneralsAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    dispatch_async(dispatch_get_main_queue(), ^{
        [self runGame];
    });
}

// The press that starts dragging the window edge lands inside the content view, but
// AppKit keeps its release for the resize. Windows never delivers a border press to
// the game (WM_NCLBUTTONDOWN), so the press is taken back before the game reads it.
- (void)windowWillStartLiveResize:(NSNotification *)notification {
    MacOS_DiscardPendingMouseDown();
}

// TheSuperHackers @feature macOS: Sync engine resolution when user finishes
// dragging the window edge. Uses windowDidEndLiveResize (not windowDidResize)
// to avoid reacting to system-triggered resize events during initialization.
- (void)windowDidEndLiveResize:(NSNotification *)notification {
    if (!self.window) return;

    NSView* contentView = self.window.contentView;
    CGSize newSize = contentView.bounds.size;
    int newW = (int)newSize.width;
    int newH = (int)newSize.height;

    printf("[MacOS] windowDidEndLiveResize: logical=%dx%d (bsf=%.1f)\n",
           newW, newH, self.window.backingScaleFactor);
    fflush(stdout);

    bool isWindowed = TheGlobalData ? TheGlobalData->m_windowed : false;
    MacOS_ApplyDisplayResolution(newW, newH, isWindowed);
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
    if (TheMouse) {
        TheMouse->regainFocus();
    }
}

- (void)windowDidResignKey:(NSNotification *)notification {
    if (TheMouse) {
        TheMouse->loseFocus();
    }
}

- (void)runGame {
    Int exitcode = 1;

    // 1. Signal handlers (mirrors SetUnhandledExceptionFilter, line 808)
    MacOS_InstallCrashHandlers(gAppPrefix);
    observeSystemSleep();

    // 2. Critical sections (mirrors lines 817-821)
    TheAsciiStringCriticalSection = &critSec1;
    TheUnicodeStringCriticalSection = &critSec2;
    TheDmaCriticalSection = &critSec3;
    TheMemoryPoolCriticalSection = &critSec4;
    TheDebugLogCriticalSection = &critSec5;

    // 3. Memory manager (mirrors line 824)
    initMemoryManager();

    // 4. Working directory (mirrors lines 827-833)
    // WinMain: GetModuleFileName + SetCurrentDirectory
    // macOS: Do not change directory so that we stay in the project root 
    // where Data/ exists, just like in GeneralsGameCode.
    // NSString* execPath = [[NSBundle mainBundle] executablePath];
    // NSString* execDir = [execPath stringByDeletingLastPathComponent];
    // chdir([execDir UTF8String]);

    // 5. Command line (mirrors line 874)
    CommandLine::parseCommandLineForStartup();
    printf("[DIAG] MacOSMain: parseCommandLineForStartup done, TheGlobalData=%p\n", (void*)TheGlobalData);
    fflush(stdout);

    // 6. Create window (mirrors initializeAppWindows, line 881)
    if (TheGlobalData && !TheGlobalData->m_headless) {
        [self createWindow];
        printf("[DIAG] MacOSMain: window created, ApplicationHWnd=%p\n", ApplicationHWnd);
        fflush(stdout);
    } else {
        printf("[DIAG] MacOSMain: SKIPPING window creation! TheGlobalData=%p\n", (void*)TheGlobalData);
        fflush(stdout);
    }

    // 7. Steam (mirrors line 886)
#if defined(RTS_ZEROHOUR)
    NGMP_OnlineServicesManager::AttemptLoadSteam();
#endif

    // 8. ApplicationHInstance (mirrors line 889)
    ApplicationHInstance = nullptr;

    // 9. Version (mirrors lines 903-918)
    TheVersion = NEW Version;
#if defined(GENERALS_ONLINE)
    TheVersion->setVersion(VERSION_MAJOR, VERSION_MINOR, GENERALS_ONLINE_VERSION, GENERALS_ONLINE_NET_VERSION,
#if !defined(_DEBUG)
        AsciiString("Generals Online Development Team | GitHub Buildserver"), AsciiString(""),
#else
        AsciiString("Generals Online Development Team | Development Test Build"), AsciiString(""),
#endif
        AsciiString(__TIME__), AsciiString(__DATE__));
#else
    TheVersion->setVersion(VERSION_MAJOR, VERSION_MINOR, VERSION_BUILDNUM, VERSION_LOCALBUILDNUM,
        AsciiString(VERSION_BUILDUSER), AsciiString(VERSION_BUILDLOC),
        AsciiString(__TIME__), AsciiString(__DATE__));
#endif

    // 10. Instance check (mirrors lines 922-936)
    // Skip mutex-based instance check on macOS

    // 11. GameMain — SHARED CODE (mirrors line 942)
    exitcode = GameMain();

    // 12. Cleanup (mirrors lines 944-954)
    delete TheVersion;
    TheVersion = nullptr;

    shutdownMemoryManager();

    TheUnicodeStringCriticalSection = nullptr;
    TheDmaCriticalSection = nullptr;
    TheMemoryPoolCriticalSection = nullptr;

    [NSApp terminate:nil];
}

extern "C" void MacOS_InitWindowedState(bool isWindowed, int xRes, int yRes);

- (void)createWindow {
    [self showSplashWindow];

    int width = TheGlobalData ? TheGlobalData->m_xResolution : 800;
    int height = TheGlobalData ? TheGlobalData->m_yResolution : 600;
    bool isWindowed = true;

    MacOS_InitWindowedState(isWindowed, width, height);

    printf("[DIAG] createWindow: %dx%d windowed=%d\n", width, height, isWindowed);
    fflush(stdout);

    NSRect frame = NSMakeRect(0, 0, width, height);
    NSWindowStyleMask style = NSWindowStyleMaskTitled
                            | NSWindowStyleMaskClosable
                            | NSWindowStyleMaskMiniaturizable
                            | NSWindowStyleMaskResizable;

    self.window = [[NSWindow alloc] initWithContentRect:frame
                                    styleMask:style
                                    backing:NSBackingStoreBuffered
                                    defer:NO];
    [self.window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenNone];
    [self.window setContentMinSize:NSMakeSize(DEFAULT_DISPLAY_WIDTH, DEFAULT_DISPLAY_HEIGHT)];
    [self.window setTitle:@"Command and Conquer Generals"];
    [self.window center];
    [self.window setAlphaValue:0.0];
    [self.window makeKeyAndOrderFront:nil];
    [self.window setDelegate:self];

    ApplicationHWnd = (__bridge void*)self.window;
}

static NSWindow* g_splashWindow = nil;

static NSString* ModSplashPath() {
    if (!TheGlobalData || TheGlobalData->m_modDir.isEmpty()) {
        return nil;
    }

    AsciiString enginePath = TheGlobalData->m_modDir;
    enginePath.concat("Install_Final.bmp");

    std::string nativePath = NativeFileSystem::get_safe_path(enginePath.str());
    if (!NativeFileSystem::exists(nativePath)) {
        return nil;
    }

    return [NSString stringWithUTF8String:nativePath.c_str()];
}

- (void)showSplashWindow {
    NSString* splashPath = ModSplashPath();
    if (!splashPath) {
        splashPath = [[NSBundle mainBundle] pathForResource:@"Install_Final" ofType:@"bmp"];
    }

    if (!splashPath) {
        return;
    }

    NSImage* splashImage = [[NSImage alloc] initWithContentsOfFile:splashPath];
    if (!splashImage) {
        return;
    }

    NSRect frame = NSMakeRect(0, 0, 800, 600);
    g_splashWindow = [[NSWindow alloc] initWithContentRect:frame
                                       styleMask:NSWindowStyleMaskBorderless
                                       backing:NSBackingStoreBuffered
                                       defer:NO];
    [g_splashWindow setBackgroundColor:[NSColor blackColor]];
    [g_splashWindow setLevel:NSStatusWindowLevel];
    [g_splashWindow center];

    NSImageView* splashView = [NSImageView imageViewWithImage:splashImage];
    splashView.frame = g_splashWindow.contentView.bounds;
    splashView.imageScaling = NSImageScaleAxesIndependently;
    splashView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [g_splashWindow.contentView addSubview:splashView];

    [g_splashWindow makeKeyAndOrderFront:nil];
    [g_splashWindow display];
}

extern "C" void MacOS_DismissSplash() {
    if (!g_splashWindow) {
        return;
    }

    [g_splashWindow orderOut:nil];
    g_splashWindow = nil;

    NSWindow *mainWindow = (__bridge NSWindow *)ApplicationHWnd;
    if (mainWindow) {
        [mainWindow setAlphaValue:1.0];
    }

    fprintf(stderr, "[MacOS] Splash window dismissed\n");
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)app {
    return YES;
}

extern "C" bool MacOS_ToggleFullscreen();

- (BOOL)windowShouldZoom:(NSWindow *)window toFrame:(NSRect)newFrame {
    // Intercept green "zoom" button to trigger borderless fullscreen
    MacOS_ToggleFullscreen();
    return NO; // Prevent default macOS zoom
}

@end

// ── CreateGameEngine (mirrors WinMain.cpp lines 978-989) ──

#include "MacOSGameEngine.h"

GameEngine* CreateGameEngine() {
    MacOSGameEngine* engine = NEW MacOSGameEngine;
    engine->setIsActive(isAppActive);
    return engine;
}

// ── main() (mirrors WinMain entry point) ──

int main(int argc, char* argv[]) {
    std::setlocale(LC_CTYPE, "en_US.UTF-8");
    MacOS_StoreCommandLine(argc, argv);
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        const bool isHotkeyRun = MacOS_GetCommandLineValue(MacOSHotkeyDump::kFlag) != nullptr
            || MacOS_GetCommandLineValue(MacOSHotkeyApply::kFlag) != nullptr
            || MacOS_GetCommandLineValue(MacOSHotkeyImport::kFlag) != nullptr
            || MacOS_GetCommandLineValue(MacOSHotkeyExport::kFlag) != nullptr;
        [app setActivationPolicy:isHotkeyRun ? NSApplicationActivationPolicyAccessory : NSApplicationActivationPolicyRegular]; // Force foreground application even from terminal
        
        GeneralsAppDelegate* delegate = [[GeneralsAppDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}
