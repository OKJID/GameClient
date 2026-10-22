// MacOSPngWriter.mm — encodes an RGBA pixel buffer as PNG bytes

#import <AppKit/AppKit.h>

#include "MacOSPngWriter.h"

#include <cstring>

bool MacOS_EncodeRgbaPng( const unsigned char* rgba, int width, int height, std::vector<unsigned char>& png )
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

		NSData* encoded = [bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
		if ( encoded == nil )
			return false;

		const unsigned char* bytes = (const unsigned char*)[encoded bytes];
		png.assign( bytes, bytes + [encoded length] );
		return true;
	}
}
