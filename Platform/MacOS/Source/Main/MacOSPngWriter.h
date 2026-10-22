// MacOSPngWriter.h — encodes an RGBA pixel buffer as PNG bytes

#pragma once

#include <vector>

bool MacOS_EncodeRgbaPng( const unsigned char* rgba, int width, int height, std::vector<unsigned char>& png );
