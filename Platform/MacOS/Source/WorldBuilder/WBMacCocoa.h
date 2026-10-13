#pragma once

#define __INTLRESOURCES__
#define __FINDER__
#define __AIFF__

#define Byte MacByte
#define RGBColor MacRGBColor
#define BOOL MacBOOL
#define Polygon MacQuickdrawPolygon
#import <Cocoa/Cocoa.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#undef Byte
#undef RGBColor
#undef BOOL
#undef Polygon

#include "WWLib/bittype.h"
