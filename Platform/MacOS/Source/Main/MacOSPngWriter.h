// MacOSPngWriter.h — writes an RGBA pixel buffer as a PNG file

#pragma once

bool MacOS_WriteRgbaPng( const char* path, const unsigned char* rgba, int width, int height );
