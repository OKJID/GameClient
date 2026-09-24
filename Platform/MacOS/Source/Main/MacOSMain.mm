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
#include <atomic>
#include <ctime>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <sys/ucontext.h>

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
#include "Common/System/NativeFileSystem.h"
#include "Common/version.h"
#include "GameClient/ClientInstance.h"
#include "GameClient/Mouse.h"
#include "BuildVersion.h"
#include "GeneratedVersion.h"

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

// ── Signal handler (mirrors UnHandledExceptionFilter) ──

static const int kCrashReportFiles = 4;
static const size_t kCrashReportPathSize = 1024;
static char s_crashReportPaths[kCrashReportFiles][kCrashReportPathSize];
static char s_crashReportBuild[256];
static char s_crashReportArgs[2048];
static int s_crashReportFd = -1;
static std::atomic<pthread_t> s_crashThread{nullptr};

static void prepareCrashReportPaths() {
    const char* home = getenv("HOME");
    if (home == nullptr) {
        return;
    }

#if defined(RTS_ZEROHOUR)
    const char* userDataDir = "Command and Conquer Generals Zero Hour Data";
#else
    const char* userDataDir = "Command and Conquer Generals Data";
#endif
    snprintf(s_crashReportPaths[0], kCrashReportPathSize, "%s/%s/MacCrash.txt", home, userDataDir);
    snprintf(s_crashReportPaths[1], kCrashReportPathSize, "%s.bak", s_crashReportPaths[0]);
    for (int backup = 2; backup < kCrashReportFiles; ++backup) {
        snprintf(s_crashReportPaths[backup], kCrashReportPathSize, "%s.bak%d", s_crashReportPaths[0], backup);
    }
}

static void prepareCrashReportBuild() {
    NSBundle* bundle = [NSBundle mainBundle];
    NSString* version = [bundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
    NSString* build = [bundle objectForInfoDictionaryKey:@"CFBundleVersion"];
    snprintf(s_crashReportBuild, sizeof(s_crashReportBuild), "CRASH BUILD: %s (%s)",
        version != nil ? version.UTF8String : "?", build != nil ? build.UTF8String : "?");

    NSString* arguments = [[[NSProcessInfo processInfo] arguments] componentsJoinedByString:@" "];
    snprintf(s_crashReportArgs, sizeof(s_crashReportArgs), "CRASH ARGS: %s", arguments.UTF8String);
}

static void writeCrashText(int fd, const char* text) {
    if (fd < 0) {
        return;
    }

    write(fd, text, strlen(text));
    write(fd, "\n", 1);
}

static void logCrashLine(const char* line) {
    writeCrashText(STDOUT_FILENO, line);
    writeCrashText(s_crashReportFd, line);
}

static void flushStdoutIfUnlocked() {
    if (ftrylockfile(stdout) != 0) {
        return;
    }

    fflush(stdout);
    funlockfile(stdout);
}

static void openCrashReport() {
    if (s_crashReportPaths[0][0] == '\0') {
        return;
    }

    for (int backup = kCrashReportFiles - 1; backup > 0; --backup) {
        rename(s_crashReportPaths[backup - 1], s_crashReportPaths[backup]);
    }

    s_crashReportFd = open(s_crashReportPaths[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
}

static void closeCrashReport() {
    if (s_crashReportFd < 0) {
        return;
    }

    close(s_crashReportFd);
    s_crashReportFd = -1;
}

static void logCrashTime() {
    char line[64];
    const time_t now = time(nullptr);
    struct tm utc;
    gmtime_r(&now, &utc);
    snprintf(line, sizeof(line), "CRASH TIME: %04d-%02d-%02d %02d:%02d:%02d UTC",
        utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday, utc.tm_hour, utc.tm_min, utc.tm_sec);
    logCrashLine(line);
}

static void logCrashBacktrace() {
    void* callstack[128];
    const int frames = backtrace(callstack, 128);
    logCrashLine("CRASH BACKTRACE:");
    backtrace_symbols_fd(callstack, frames, STDOUT_FILENO);
    if (s_crashReportFd >= 0) {
        backtrace_symbols_fd(callstack, frames, s_crashReportFd);
    }
}

static void logFaultMemoryRegion(uintptr_t address) {
    char line[256];
    mach_vm_address_t regionStart = address;
    mach_vm_size_t regionSize = 0;
    vm_region_basic_info_data_64_t info;
    mach_msg_type_number_t infoCount = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t objectName = MACH_PORT_NULL;
    const kern_return_t result = mach_vm_region(mach_task_self(), &regionStart, &regionSize, VM_REGION_BASIC_INFO_64,
        (vm_region_info_t)&info, &infoCount, &objectName);

    if (result != KERN_SUCCESS) {
        snprintf(line, sizeof(line), "CRASH FAULT REGION: none above 0x%lx (kern %d)", (unsigned long)address, result);
        logCrashLine(line);
        return;
    }

    if (regionStart > address) {
        snprintf(line, sizeof(line), "CRASH FAULT REGION: unmapped, next region 0x%llx",
            (unsigned long long)regionStart);
        logCrashLine(line);
        return;
    }

    snprintf(line, sizeof(line), "CRASH FAULT REGION: 0x%llx-0x%llx protection %d max %d",
        (unsigned long long)regionStart, (unsigned long long)(regionStart + regionSize), info.protection, info.max_protection);
    logCrashLine(line);
}

static void logFaultContext(const siginfo_t* info, void* context) {
    char line[256];
    const uintptr_t faultAddress = (uintptr_t)info->si_addr;
    snprintf(line, sizeof(line), "CRASH FAULT ADDRESS: 0x%lx code %d", (unsigned long)faultAddress, info->si_code);
    logCrashLine(line);

#if defined(__arm64__)
    const ucontext_t* userContext = (const ucontext_t*)context;
    const __darwin_arm_thread_state64& state = userContext->uc_mcontext->__ss;
    snprintf(line, sizeof(line), "CRASH REGISTERS: pc 0x%llx lr 0x%llx sp 0x%llx fp 0x%llx x0 0x%llx x19 0x%llx x20 0x%llx x21 0x%llx",
        (unsigned long long)__darwin_arm_thread_state64_get_pc(state), (unsigned long long)__darwin_arm_thread_state64_get_lr(state),
        (unsigned long long)__darwin_arm_thread_state64_get_sp(state), (unsigned long long)__darwin_arm_thread_state64_get_fp(state),
        (unsigned long long)state.__x[0], (unsigned long long)state.__x[19], (unsigned long long)state.__x[20],
        (unsigned long long)state.__x[21]);
    logCrashLine(line);
#endif

    logFaultMemoryRegion(faultAddress);
}

static void waitForCrashReportOrExit() {
    pthread_t reportingThread = nullptr;
    if (s_crashThread.compare_exchange_strong(reportingThread, pthread_self())) {
        return;
    }

    if (pthread_equal(reportingThread, pthread_self())) {
        _exit(1);
    }

    for (;;) {
        pause();
    }
}

static void macosSignalHandler(int sig, siginfo_t* info, void* context) {
    waitForCrashReportOrExit();
    flushStdoutIfUnlocked();
    openCrashReport();
    logCrashTime();

    char line[64];
    snprintf(line, sizeof(line), "FATAL: Caught signal %d", sig);
    logCrashLine(line);

    logFaultContext(info, context);
    logCrashBacktrace();
    logCrashLine(s_crashReportBuild);
    logCrashLine(s_crashReportArgs);
    closeCrashReport();

    _exit(1);
}

static void installCrashHandler(int sig) {
    struct sigaction action = {};
    action.sa_sigaction = macosSignalHandler;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    sigaction(sig, &action, nullptr);
}

static void installCrashHandlers() {
    prepareCrashReportPaths();
    prepareCrashReportBuild();
    installCrashHandler(SIGSEGV);
    installCrashHandler(SIGBUS);
    installCrashHandler(SIGABRT);
}

// ── External engine bridges ──

extern "C" void MacOS_ApplyDisplayResolution(int w, int h, bool isWindowed);
extern "C" void MacOS_UpdateMetalDeviceScreenSize(int width, int height);

#include <sys/types.h>
#include <sys/sysctl.h>

extern "C" unsigned long long MacOS_GetTotalPhysicalMemory() {
    int64_t physical_memory = 0;
    size_t length = sizeof(int64_t);
    int res = sysctlbyname("hw.memsize", &physical_memory, &length, nullptr, 0);
    printf("DEBUG: MacOS_GetTotalPhysicalMemory -> res=%d, mem=%llu\n", res, (unsigned long long)physical_memory); fflush(stdout);
    return 2048ULL * 1024 * 1024; // Force it to 2GB to be safe
}

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
    installCrashHandlers();

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

char MacOSCommandLineString[4096] = "";

int main(int argc, char* argv[]) {
    std::setlocale(LC_CTYPE, "en_US.UTF-8");
    MacOSCommandLineString[0] = '\0';
    for (int i=0; i<argc; ++i) {
        if (i>0) strncat(MacOSCommandLineString, " ", sizeof(MacOSCommandLineString)-1 - strlen(MacOSCommandLineString));
        bool hasSpace = strchr(argv[i], ' ') != nullptr;
        if (hasSpace) strncat(MacOSCommandLineString, "\"", sizeof(MacOSCommandLineString)-1 - strlen(MacOSCommandLineString));
        strncat(MacOSCommandLineString, argv[i], sizeof(MacOSCommandLineString)-1 - strlen(MacOSCommandLineString));
        if (hasSpace) strncat(MacOSCommandLineString, "\"", sizeof(MacOSCommandLineString)-1 - strlen(MacOSCommandLineString));
    }
    printf("[DIAG] MacOSMain: Built cmd line: %s\n", MacOSCommandLineString);
    fflush(stdout);
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular]; // Force foreground application even from terminal
        
        GeneralsAppDelegate* delegate = [[GeneralsAppDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}
