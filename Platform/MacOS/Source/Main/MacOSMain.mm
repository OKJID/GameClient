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
#include <unistd.h>
#include <signal.h>

#include "always.h"
#include <windows.h>

#include "Lib/BaseType.h"
#include "Common/AsciiString.h"
#include "Common/CommandLine.h"
#include "Common/CriticalSection.h"
#include "Common/GlobalData.h"
#include "Common/GameEngine.h"
#include "Common/GameMemory.h"
#include "Common/Debug.h"
#include "Common/version.h"
#include "GameClient/ClientInstance.h"
#include "BuildVersion.h"
#include "GeneratedVersion.h"

#include "../OnlineServices_Init.h"

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

// ── Signal handler (mirrors UnHandledExceptionFilter) ──

static void macosSignalHandler(int sig) {
    DEBUG_LOG(("Caught signal %d", sig));
    _exit(1);
}

// ── External engine bridges ──

extern "C" void MacOS_ApplyDisplayResolution(int w, int h);
extern "C" void MacOS_UpdateMetalDeviceScreenSize(int width, int height);

// TheSuperHackers @feature macOS: Compute 90% of main screen dimensions for
// first-time users with no saved resolution. Called from GlobalData.cpp when
// OptionPreferences has no "Resolution" key (i.e., default 800x600 is returned).
extern "C" void MacOS_GetAdaptiveResolution(int *w, int *h) {
    NSScreen *screen = [NSScreen mainScreen];
    if (!screen) return;

    NSRect frame = [screen visibleFrame];
    *w = (int)(frame.size.width * 0.9);
    *h = (int)(frame.size.height * 0.9);

    // Ensure even dimensions (Metal drawable requirement)
    *w &= ~1;
    *h &= ~1;

    printf("[MacOS] GetAdaptiveResolution: screen=%.0fx%.0f -> adaptive=%dx%d\n",
           frame.size.width, frame.size.height, *w, *h);
    fflush(stdout);
}

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

// TheSuperHackers @feature macOS: Sync engine resolution when user finishes
// dragging the window edge. Uses windowDidEndLiveResize (not windowDidResize)
// to avoid reacting to system-triggered resize events during initialization.
- (void)windowDidEndLiveResize:(NSNotification *)notification {
    if (!self.window) return;

    NSView* contentView = self.window.contentView;
    CGSize newSize = contentView.bounds.size;
    int newW = (int)newSize.width;
    int newH = (int)newSize.height;

    printf("[MacOS] windowDidEndLiveResize: %dx%d\n", newW, newH);
    fflush(stdout);

    // Update CAMetalLayer drawable size
    if (contentView.layer && [contentView.layer isKindOfClass:[CAMetalLayer class]]) {
        CAMetalLayer* layer = (CAMetalLayer*)contentView.layer;
        layer.contentsScale = 1.0;
        layer.drawableSize = CGSizeMake(newW, newH);
    }

    // Update MetalDevice8 (viewport, depth texture, screen dimensions)
    MacOS_UpdateMetalDeviceScreenSize(newW, newH);

    // Apply through the full engine path (mirrors OptionsMenu Accept)
    MacOS_ApplyDisplayResolution(newW, newH);
}

- (void)runGame {
    Int exitcode = 1;

    // 1. Signal handlers (mirrors SetUnhandledExceptionFilter, line 808)
    signal(SIGSEGV, macosSignalHandler);
    signal(SIGBUS, macosSignalHandler);
    signal(SIGABRT, macosSignalHandler);

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
    NGMP_OnlineServicesManager::AttemptLoadSteam();

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

- (void)createWindow {
    int width = TheGlobalData ? TheGlobalData->m_xResolution : 800;
    int height = TheGlobalData ? TheGlobalData->m_yResolution : 600;
    printf("[DIAG] createWindow: %dx%d xRes=%d yRes=%d\n", width, height,
           TheGlobalData ? TheGlobalData->m_xResolution : -1,
           TheGlobalData ? TheGlobalData->m_yResolution : -1);
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
    [self.window setTitle:@"Command and Conquer Generals"];
    [self.window center];
    [self.window makeKeyAndOrderFront:nil];
    [self.window setDelegate:self];

    ApplicationHWnd = (__bridge void*)self.window;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)app {
    return YES;
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
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular]; // Force foreground application even from terminal
        
        GeneralsAppDelegate* delegate = [[GeneralsAppDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}
