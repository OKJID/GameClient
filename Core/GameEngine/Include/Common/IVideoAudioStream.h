#pragma once

#include <stdint.h>

class IVideoAudioStream
{
public:
    virtual ~IVideoAudioStream() {}

    virtual void reset() = 0;
    virtual void play() = 0;
    virtual void update() = 0;
    virtual void bufferData(uint8_t* data, int size, int channels, int bitsPerSample, int sampleRate) = 0;
};
