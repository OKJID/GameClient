#include "MacOSVideoAudioStream.h"
#include "AVAudioBridge.h"

#include <MacTypes.h>
#ifndef Byte
typedef unsigned char Byte;
#endif

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>

MacOSVideoAudioStream::MacOSVideoAudioStream()
    : m_playerNode(nullptr)
    , m_attached(false)
    , m_lastChannels(0)
    , m_lastSampleRate(0)
{
    AVAudioPlayerNode* node = [[AVAudioPlayerNode alloc] init];
    m_playerNode = (__bridge_retained void*)node;
}

MacOSVideoAudioStream::~MacOSVideoAudioStream()
{
    reset();

    AVAudioPlayerNode* node = (__bridge_transfer AVAudioPlayerNode*)m_playerNode;
    m_playerNode = nullptr;

    AVAudioEngine* engine = (__bridge AVAudioEngine*)avbridge_get_engine();
    if (engine && node) {
        [node stop];
        [engine detachNode:node];
    }
}

void MacOSVideoAudioStream::reset()
{
    AVAudioPlayerNode* node = (__bridge AVAudioPlayerNode*)m_playerNode;
    if (!node) return;

    [node stop];
    m_attached = false;
    m_lastChannels = 0;
    m_lastSampleRate = 0;
}

void MacOSVideoAudioStream::play()
{
    AVAudioPlayerNode* node = (__bridge AVAudioPlayerNode*)m_playerNode;
    if (!node || !m_attached) return;

    if (!node.isPlaying) {
        [node play];
    }
}

void MacOSVideoAudioStream::update()
{
}

void MacOSVideoAudioStream::bufferData(uint8_t* data, int size, int channels, int bitsPerSample, int sampleRate)
{
    if (!data || size <= 0 || channels <= 0 || bitsPerSample <= 0 || sampleRate <= 0) return;

    int bytesPerSample = bitsPerSample / 8;
    if (bytesPerSample <= 0) return;

    uint32_t framesTotal = (uint32_t)(size / (bytesPerSample * channels));
    if (framesTotal == 0 || framesTotal > 1000000) return;

    AVAudioEngine* engine = (__bridge AVAudioEngine*)avbridge_get_engine();
    AVAudioMixerNode* mixer = (__bridge AVAudioMixerNode*)avbridge_get_mixer2D();
    AVAudioPlayerNode* node = (__bridge AVAudioPlayerNode*)m_playerNode;
    if (!engine || !mixer || !node) return;

    AVAudioFormat* fmt = [[AVAudioFormat alloc]
        initStandardFormatWithSampleRate:(double)sampleRate
                                channels:(AVAudioChannelCount)channels];
    if (!fmt) return;

    bool formatChanged = (channels != m_lastChannels || sampleRate != m_lastSampleRate);

    if (!m_attached || formatChanged) {
        [node stop];

        if ([node engine] != nil) {
            [engine disconnectNodeOutput:node];
        }

        if ([node engine] == nil) {
            [engine attachNode:node];
        }

        [engine connect:node to:mixer format:fmt];
        m_attached = true;
        m_lastChannels = channels;
        m_lastSampleRate = sampleRate;

        if (!engine.isRunning) {
            NSError* err = nil;
            [engine startAndReturnError:&err];
        }

        [node play];
    }

    AVAudioPCMBuffer* buf = [[AVAudioPCMBuffer alloc] initWithPCMFormat:fmt frameCapacity:framesTotal];
    if (!buf) return;

    buf.frameLength = framesTotal;

    if (bitsPerSample == 32) {
        const float* src = (const float*)data;
        for (int ch = 0; ch < channels; ch++) {
            float* dst = buf.floatChannelData[ch];
            for (uint32_t f = 0; f < framesTotal; f++) {
                dst[f] = src[f * channels + ch];
            }
        }
    } else if (bitsPerSample == 16) {
        const int16_t* src = (const int16_t*)data;
        for (int ch = 0; ch < channels; ch++) {
            float* dst = buf.floatChannelData[ch];
            for (uint32_t f = 0; f < framesTotal; f++) {
                dst[f] = (float)src[f * channels + ch] / 32768.0f;
            }
        }
    } else {
        for (int ch = 0; ch < channels; ch++) {
            float* dst = buf.floatChannelData[ch];
            for (uint32_t f = 0; f < framesTotal; f++) {
                dst[f] = ((float)data[f * channels + ch] - 128.0f) / 128.0f;
            }
        }
    }

    [node scheduleBuffer:buf completionHandler:nil];
}

