#include "WBMacCocoa.h"

#include "afxwin.h"
#include "mfc/AfxStandardResources.h"

#include "MacOSHostServices.h"
#include "MacOSNativeWindow.h"
#include "WBMacFramePump.h"
#include "WBMacMessageBox.h"
#include "WBMacPlatform.h"
#include "Win32Controls.h"
#include "Win32Gdi.h"
#include "Win32Paths.h"
#include "Win32Renderer.h"
#include "Win32Resources.h"
#include "imgui.h"
#include "imgui_impl_dx8wrapper.h"
#include "imgui_impl_osx.h"

#include <string>
#include <vector>

static const CGFloat kInitialWidth = 1280.0;
static const CGFloat kInitialHeight = 800.0;

extern "C" void MacOS_ApplyWorldBuilderViewSize(int width, int height);
extern "C" void MacOS_RefreshBackBufferScale();

extern "C" void MacOS_DismissSplash()
{
}

static void MacOS_WBSplash()
{
}

static void requestHostClose()
{
	const Win32Window* host = Win32WindowManager::instance().hostWindow();
	if (host == nullptr)
	{
		return;
	}
	PostMessageA(host->hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
}

static void requestViewSize(int width, int height)
{
	const Win32Window* host = Win32WindowManager::instance().hostWindow();
	if (host == nullptr)
	{
		return;
	}
	Win32PostTask(host->hwnd, [width, height]() { MacOS_ApplyWorldBuilderViewSize(width, height); });
}

static void requestBackBufferScaleRefresh()
{
	const Win32Window* host = Win32WindowManager::instance().hostWindow();
	if (host == nullptr)
	{
		return;
	}
	Win32PostTask(host->hwnd, []() { MacOS_RefreshBackBufferScale(); });
}

@interface WBWindowDelegate : NSObject<NSWindowDelegate, NSApplicationDelegate>
@end

@implementation WBWindowDelegate
- (MacBOOL)windowShouldClose:(NSWindow*)sender
{
	requestHostClose();
	return NO;
}

- (void)windowDidEndLiveResize:(NSNotification*)notification
{
	NSWindow* window = notification.object;
	const NSSize size = window.contentView.bounds.size;
	requestViewSize(static_cast<int>(size.width), static_cast<int>(size.height));
}

- (void)windowDidChangeBackingProperties:(NSNotification*)notification
{
	requestBackBufferScaleRefresh();
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
	requestHostClose();
	return NSTerminateCancel;
}
@end

static void installApplicationMenu()
{
	NSMenu* menuBar = [[NSMenu alloc] init];
	NSMenuItem* appMenuItem = [[NSMenuItem alloc] init];
	[menuBar addItem:appMenuItem];

	NSMenu* appMenu = [[NSMenu alloc] init];
	[appMenu addItemWithTitle:@"Quit WorldBuilder" action:@selector(terminate:) keyEquivalent:@"q"];
	[appMenuItem setSubmenu:appMenu];

	[NSApp setMainMenu:menuBar];
}

static NSWindow* createMainWindow(WBWindowDelegate* delegate)
{
	const NSRect frame = NSMakeRect(0, 0, kInitialWidth, kInitialHeight);
	const NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
		| NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;

	NSWindow* window = [[NSWindow alloc] initWithContentRect:frame styleMask:style backing:NSBackingStoreBuffered defer:NO];
	[window setTitle:@"WorldBuilder - Zero Hour"];
	[window setReleasedWhenClosed:NO];
	[window setDelegate:delegate];
	[window center];
	[window makeKeyAndOrderFront:nil];
	return window;
}

static std::string engineArgument(const char* argument)
{
	if (argument[0] != '/')
	{
		return argument;
	}
	return Win32EnginePath(argument);
}

static std::string buildCommandLine(int argc, char* argv[], std::vector<std::string>& arguments)
{
	std::string commandLine;
	for (int index = 0; index < argc; ++index)
	{
		arguments.push_back(engineArgument(argv[index]));
		if (index == 0)
		{
			continue;
		}
		const std::string& argument = arguments.back();
		const bool needsQuotes = argument.find(' ') != std::string::npos;
		commandLine += commandLine.empty() ? "" : " ";
		commandLine += needsQuotes ? "\"" + argument + "\"" : argument;
	}
	return commandLine;
}

static bool hasMenuBar(const CREATESTRUCTA& create)
{
	return create.hMenu != nullptr && (create.style & WS_CHILD) == 0;
}

static bool loadResources()
{
	NSString* directory = [[NSBundle mainBundle].resourcePath stringByAppendingPathComponent:@"WorldBuilder"];
	AfxRegisterStandardResources();
	return Win32Resources::instance().load(Win32EnginePath(directory.UTF8String));
}

static void initializeImGui(NSWindow* window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplOSX_Init(window.contentView);
	ImGui_ImplDX8Wrapper_Init();
	Win32Renderer::instance().configure(ImGui::GetIO());
}

static void shutdownImGui()
{
	ImGui_ImplDX8Wrapper_Shutdown();
	ImGui_ImplOSX_Shutdown();
	ImGui::DestroyContext();
}

static void installWin32Host(WBMacPlatform& platform, WBMacFramePump& framePump)
{
	Win32SetPlatform(&platform);
	MacOS_SetBackBufferUsesBackingScale(true);
	MacOS_SetMessageBoxHandler(&WBMacMessageBox);
	Win32SetHostWindowPolicy(&hasMenuBar);
	Win32Gdi::initialize();
	Win32RegisterStandardClasses();
	framePump.install();
}

static NSWindow* launchApplication(WBWindowDelegate* delegate)
{
	[NSApplication sharedApplication];
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
	[NSApp setDelegate:delegate];
	installApplicationMenu();
	[NSApp finishLaunching];

	MacOS_WBSplash();

	NSWindow* window = createMainWindow(delegate);
	[NSApp activateIgnoringOtherApps:YES];
	return window;
}

int main(int argc, char* argv[])
{
	MacOS_StoreCommandLine(argc, argv);
	std::vector<std::string> arguments;
	std::string commandLine = buildCommandLine(argc, argv, arguments);
	std::vector<char*> argumentPointers;
	for (std::string& argument : arguments)
	{
		argumentPointers.push_back(argument.data());
	}
	__argc = argc;
	__argv = argumentPointers.data();

	int exitCode = 0;
	@autoreleasepool
	{
		WBWindowDelegate* delegate = [[WBWindowDelegate alloc] init];
		NSWindow* window = launchApplication(delegate);

		WBMacPlatform platform;
		WBMacFramePump framePump(window);
		installWin32Host(platform, framePump);
		if (!loadResources())
		{
			MessageBoxA(nullptr, "WorldBuilder resources are missing from the application bundle.", "WorldBuilder", MB_OK | MB_ICONERROR);
			return 1;
		}

		initializeImGui(window);
		exitCode = AfxWinMain(nullptr, commandLine.data(), SW_SHOWNORMAL);
		shutdownImGui();
		[window close];
	}
	return exitCode;
}
