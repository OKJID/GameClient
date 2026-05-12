#pragma once

#include "Common/IVideoAudioStream.h"

class MacOSVideoAudioStream : public IVideoAudioStream {
public:
    MacOSVideoAudioStream();
    ~MacOSVideoAudioStream() override;

    void reset() override;
    void play() override;
    void update() override;
    void bufferData(uint8_t* data, int size, int channels, int bitsPerSample, int sampleRate) override;

private:
    void* m_playerNode;
    bool  m_attached;
    int   m_lastChannels;
    int   m_lastSampleRate;
};
