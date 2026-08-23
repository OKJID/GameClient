#include "MacOSBundleImage.h"

#include "Common/GameMemory.h"
#include "GameClient/Image.h"
#include "WW3D2/texture.h"
#include "WW3D2/surfaceclass.h"
#include "WW3D2/ww3dformat.h"

#include <map>
#include <string>
#include <vector>

namespace
{

typedef std::vector<UnsignedByte> PixelBuffer;

CGImageRef createBundleCGImage(const char *resourceName)
{
	NSString *name = [NSString stringWithUTF8String:resourceName];
	NSString *path = [[NSBundle mainBundle] pathForResource:name ofType:@"png"];
	if (path == nil)
		return nullptr;

	NSImage *bundleImage = [[NSImage alloc] initWithContentsOfFile:path];
	if (bundleImage == nil)
		return nullptr;

	CGImageRef cgImage = [bundleImage CGImageForProposedRect:nullptr context:nil hints:nil];
	if (cgImage == nullptr)
		return nullptr;

	return CGImageRetain(cgImage);
}

void straightenAlpha(PixelBuffer &pixels)
{
	for (size_t offset = 0; offset < pixels.size(); offset += 4)
	{
		const UnsignedInt alpha = pixels[offset + 3];
		if (alpha == 0 || alpha == 255)
			continue;

		pixels[offset + 0] = (UnsignedByte)((pixels[offset + 0] * 255) / alpha);
		pixels[offset + 1] = (UnsignedByte)((pixels[offset + 1] * 255) / alpha);
		pixels[offset + 2] = (UnsignedByte)((pixels[offset + 2] * 255) / alpha);
	}
}

Bool decodeToBGRA(CGImageRef cgImage, PixelBuffer &pixels, Int &width, Int &height)
{
	width = (Int)CGImageGetWidth(cgImage);
	height = (Int)CGImageGetHeight(cgImage);
	if (width <= 0 || height <= 0)
		return FALSE;

	pixels.assign((size_t)width * (size_t)height * 4, 0);

	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	CGContextRef context = CGBitmapContextCreate(pixels.data(), width, height, 8, width * 4, colorSpace,
		kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
	CGColorSpaceRelease(colorSpace);

	if (context == nullptr)
		return FALSE;

	CGContextDrawImage(context, CGRectMake(0, 0, width, height), cgImage);
	CGContextRelease(context);

	straightenAlpha(pixels);
	return TRUE;
}

TextureClass *createTexture(const PixelBuffer &pixels, Int width, Int height)
{
	TextureClass *texture = MSGNEW("TextureClass") TextureClass(width, height, WW3D_FORMAT_A8R8G8B8, MIP_LEVELS_1);
	if (texture == nullptr)
		return nullptr;

	texture->Get_Filter().Set_Min_Filter(TextureFilterClass::FILTER_TYPE_DEFAULT);
	texture->Get_Filter().Set_Mag_Filter(TextureFilterClass::FILTER_TYPE_DEFAULT);

	SurfaceClass *surface = texture->Get_Surface_Level();
	if (surface == nullptr)
		return texture;

	Int pitch = 0;
	UnsignedByte *bits = (UnsignedByte *)surface->Lock(&pitch);
	if (bits != nullptr)
	{
		const size_t rowBytes = (size_t)width * 4;
		for (Int row = 0; row < height; ++row)
			memcpy(bits + (size_t)row * pitch, pixels.data() + (size_t)row * rowBytes, rowBytes);

		surface->Unlock();
	}

	REF_PTR_RELEASE(surface);
	return texture;
}

Image *createImage(TextureClass *texture, Int width, Int height)
{
	Region2D uv;
	uv.lo.x = 0.0f;
	uv.lo.y = 0.0f;
	uv.hi.x = 1.0f;
	uv.hi.y = 1.0f;

	ICoord2D size;
	size.x = width;
	size.y = height;

	Image *image = newInstance(Image);
	image->setStatus(IMAGE_STATUS_RAW_TEXTURE);
	image->setRawTextureData(texture);
	image->setUV(&uv);
	image->setTextureWidth(width);
	image->setTextureHeight(height);
	image->setImageSize(&size);
	return image;
}

const Image *loadBundleImage(const char *resourceName)
{
	CGImageRef cgImage = createBundleCGImage(resourceName);
	if (cgImage == nullptr)
		return nullptr;

	PixelBuffer pixels;
	Int width = 0;
	Int height = 0;
	const Bool decoded = decodeToBGRA(cgImage, pixels, width, height);
	CGImageRelease(cgImage);

	if (!decoded)
		return nullptr;

	TextureClass *texture = createTexture(pixels, width, height);
	if (texture == nullptr)
		return nullptr;

	return createImage(texture, width, height);
}

}  // namespace

const Image *MacOSGetBundleImage(const char *resourceName)
{
	static std::map<std::string, const Image *> theBundleImages;

	if (resourceName == nullptr)
		return nullptr;

	std::map<std::string, const Image *>::const_iterator it = theBundleImages.find(resourceName);
	if (it != theBundleImages.end())
		return it->second;

	const Image *image = loadBundleImage(resourceName);
	theBundleImages[resourceName] = image;
	return image;
}
