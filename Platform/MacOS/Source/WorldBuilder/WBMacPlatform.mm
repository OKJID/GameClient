#include "WBMacCocoa.h"

#include "WBMacPlatform.h"

#include "Win32Gdi.h"

namespace
{

NSMutableArray<NSSound*>* activeSounds()
{
	static NSMutableArray<NSSound*>* sounds = [[NSMutableArray alloc] init];
	return sounds;
}

NSCursor* cursorForSystemId(UINT systemCursorId)
{
	switch (systemCursorId)
	{
	case 32513:
		return [NSCursor IBeamCursor];
	case 32515:
		return [NSCursor crosshairCursor];
	case 32644:
		return [NSCursor resizeLeftRightCursor];
	case 32645:
		return [NSCursor resizeUpDownCursor];
	case 32646:
		return [NSCursor openHandCursor];
	case 32648:
		return [NSCursor operationNotAllowedCursor];
	case 32649:
		return [NSCursor pointingHandCursor];
	default:
		return [NSCursor arrowCursor];
	}
}

NSImage* imageFrom(const Win32Image& image)
{
	NSBitmapImageRep* bitmap = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nullptr pixelsWide:image.width pixelsHigh:image.height
		bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:image.width * 4
		bitsPerPixel:32];
	unsigned char* destination = bitmap.bitmapData;
	for (size_t index = 0; index < image.pixels.size(); ++index)
	{
		const uint32_t pixel = image.pixels[index];
		const unsigned char alpha = static_cast<unsigned char>(pixel >> 24);
		destination[index * 4 + 0] = static_cast<unsigned char>(((pixel >> 16) & 0xFF) * alpha / 255);
		destination[index * 4 + 1] = static_cast<unsigned char>(((pixel >> 8) & 0xFF) * alpha / 255);
		destination[index * 4 + 2] = static_cast<unsigned char>((pixel & 0xFF) * alpha / 255);
		destination[index * 4 + 3] = alpha;
	}
	NSImage* result = [[NSImage alloc] initWithSize:NSMakeSize(image.width, image.height)];
	[result addRepresentation:bitmap];
	return result;
}

NSArray<UTType*>* contentTypesFor(const std::vector<Win32FileFilter>& filters)
{
	NSMutableArray<UTType*>* types = [[NSMutableArray alloc] init];
	for (const Win32FileFilter& filter : filters)
	{
		for (const std::string& extension : filter.extensions)
		{
			if (extension == "*")
			{
				return nil;
			}
			UTType* type = [UTType typeWithFilenameExtension:[NSString stringWithUTF8String:extension.c_str()]];
			if (type != nil)
			{
				[types addObject:type];
			}
		}
	}
	return types.count > 0 ? types : nil;
}

void configurePanel(NSSavePanel* panel, const Win32FileDialogRequest& request)
{
	panel.title = [NSString stringWithUTF8String:request.title.c_str()];
	panel.canCreateDirectories = YES;
	if (!request.initialDirectory.empty())
	{
		panel.directoryURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:request.initialDirectory.c_str()] isDirectory:YES];
	}
	NSArray<UTType*>* types = contentTypesFor(request.filters);
	if (types != nil)
	{
		panel.allowedContentTypes = types;
	}
	panel.allowsOtherFileTypes = types == nil;
}

}

WBMacPlatform::WBMacPlatform(NSWindow* window)
	: m_window(window)
{
}

void* WBMacPlatform::createCursor(const Win32Image& image, POINT hotSpot)
{
	NSCursor* cursor = [[NSCursor alloc] initWithImage:imageFrom(image) hotSpot:NSMakePoint(hotSpot.x, hotSpot.y)];
	return (__bridge_retained void*)cursor;
}

void* WBMacPlatform::systemCursor(UINT systemCursorId)
{
	return (__bridge_retained void*)cursorForSystemId(systemCursorId);
}

void WBMacPlatform::setCursor(void* platformCursor)
{
	NSCursor* cursor = platformCursor != nullptr ? (__bridge NSCursor*)platformCursor : [NSCursor arrowCursor];
	[cursor set];
}

void WBMacPlatform::setCursorVisible(bool visible)
{
	if (visible == !m_isCursorHidden)
	{
		return;
	}
	m_isCursorHidden = !visible;
	if (visible)
	{
		[NSCursor unhide];
		return;
	}
	[NSCursor hide];
}

void WBMacPlatform::destroyCursor(void* platformCursor)
{
	if (platformCursor != nullptr)
	{
		CFRelease(platformCursor);
	}
}

void WBMacPlatform::beep()
{
	NSBeep();
}

bool WBMacPlatform::playSoundFile(const std::string& path, bool waitUntilFinished)
{
	NSSound* sound = [[NSSound alloc] initWithContentsOfFile:[NSString stringWithUTF8String:path.c_str()] byReference:YES];
	if (sound == nil || ![sound play])
	{
		return false;
	}
	[activeSounds() addObject:sound];
	while (waitUntilFinished && sound.isPlaying)
	{
		[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
	}
	return true;
}

void WBMacPlatform::stopSounds()
{
	for (NSSound* sound in activeSounds())
	{
		[sound stop];
	}
	[activeSounds() removeAllObjects];
}

bool WBMacPlatform::chooseFile(const Win32FileDialogRequest& request, std::string& path)
{
	NSSavePanel* panel = request.isOpenDialog ? [NSOpenPanel openPanel] : [NSSavePanel savePanel];
	configurePanel(panel, request);
	if (!request.isOpenDialog)
	{
		panel.nameFieldStringValue = [NSString stringWithUTF8String:request.initialFileName.c_str()];
	}
	if ([panel runModal] != NSModalResponseOK || panel.URL == nil)
	{
		return false;
	}
	path = panel.URL.path.UTF8String;
	return true;
}

std::string WBMacPlatform::fontFilePath(const std::string& faceName)
{
	NSString* name = [NSString stringWithUTF8String:faceName.c_str()];
	CTFontRef font = CTFontCreateWithName((__bridge CFStringRef)name, 0.0, nullptr);
	if (font == nullptr)
	{
		return std::string();
	}
	NSString* family = (__bridge_transfer NSString*)CTFontCopyFamilyName(font);
	NSURL* url = (__bridge_transfer NSURL*)CTFontCopyAttribute(font, kCTFontURLAttribute);
	CFRelease(font);
	if (url == nil || ![family isEqualToString:name])
	{
		return std::string();
	}
	return url.path.UTF8String;
}

void WBMacPlatform::resizeHostWindow(int width, int height)
{
	[m_window setContentSize:NSMakeSize(width, height)];
}
