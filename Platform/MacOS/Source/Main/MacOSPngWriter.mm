// MacOSPngWriter.mm — writes an RGBA pixel buffer as a PNG file

#import <AppKit/AppKit.h>

#include "MacOSPngWriter.h"

#include <cstring>

bool MacOS_WriteRgbaPng( const char* path, const unsigned char* rgba, int width, int height )
{
	if ( width <= 0 || height <= 0 )
		return false;

	@autoreleasepool
	{
		NSBitmapImageRep* bitmap = [[NSBitmapImageRep alloc]
			initWithBitmapDataPlanes:nullptr
			pixelsWide:width
			pixelsHigh:height
			bitsPerSample:8
			samplesPerPixel:4
			hasAlpha:YES
			isPlanar:NO
			colorSpaceName:NSDeviceRGBColorSpace
			bitmapFormat:NSBitmapFormatAlphaNonpremultiplied
			bytesPerRow:width * 4
			bitsPerPixel:32];
		if ( bitmap == nil )
			return false;

		memcpy( [bitmap bitmapData], rgba, (size_t)width * (size_t)height * 4 );

		NSData* png = [bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
		return [png writeToFile:[NSString stringWithUTF8String:path] atomically:YES];
	}
}
