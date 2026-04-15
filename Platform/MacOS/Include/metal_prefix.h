#pragma once
//
// metal_prefix.h — Include FIRST in every .mm file
//
// Apple frameworks define types that conflict with engine types:
//   WideChar (union from IntlResources.h vs wchar_t from BaseType.h)
//   RGBColor (struct from Quickdraw.h vs struct from BaseType.h)
//   Byte     (UInt8 from MacTypes.h vs char from BaseTypeCore.h)
//   ChunkHeader (from AIFF.h vs from chunkio.h)
//
// Strategy: include Apple frameworks first, then rename conflicting
// types before engine headers see them.
//

#ifdef __APPLE__

#define __AIFF__
#define Byte AppleByte
#define RGBColor AppleRGBColor
#define WideChar AppleWideChar

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <AppKit/AppKit.h>
#endif

#undef Byte
#undef RGBColor
#undef WideChar

#ifndef BOOL_DEFINED
#define BOOL_DEFINED
#endif

#include <Utility/CppMacros.h>

#endif
