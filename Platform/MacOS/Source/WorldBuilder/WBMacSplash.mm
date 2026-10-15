#include "WBMacCocoa.h"

#include "WBMacSplash.h"

namespace
{

NSWindow* splashWindow = nil;
NSWindow* hiddenMainWindow = nil;

NSWindow* createSplashWindow(NSImage* image)
{
	const NSRect frame = NSMakeRect(0, 0, image.size.width, image.size.height);
	NSWindow* window = [[NSWindow alloc] initWithContentRect:frame styleMask:NSWindowStyleMaskBorderless backing:NSBackingStoreBuffered defer:NO];
	[window setReleasedWhenClosed:NO];
	[window setLevel:NSFloatingWindowLevel];
	[window center];

	NSImageView* imageView = [NSImageView imageViewWithImage:image];
	imageView.frame = window.contentView.bounds;
	imageView.imageScaling = NSImageScaleAxesIndependently;
	imageView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
	[window.contentView addSubview:imageView];
	return window;
}

}

void WBMacShowSplash(NSImage* image, NSWindow* mainWindow)
{
	if (image == nil)
	{
		return;
	}
	splashWindow = createSplashWindow(image);
	hiddenMainWindow = mainWindow;
	[hiddenMainWindow setAlphaValue:0.0];
	[splashWindow orderFrontRegardless];
	[splashWindow display];
}

void WBMacDismissSplash()
{
	if (splashWindow == nil)
	{
		return;
	}
	[splashWindow orderOut:nil];
	splashWindow = nil;
	[hiddenMainWindow setAlphaValue:1.0];
	hiddenMainWindow = nil;
}
