#import "AVAudioBridge.h"

// Restore Byte typedef required by AudioToolbox, which was undefined by metal_prefix.h
#include <MacTypes.h>
#ifndef Byte
typedef unsigned char Byte;
#endif

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <os/lock.h>

#pragma mark - Internal Types

static bool gEngineStarted = false;

struct AVBridgePlayerSlot {
    AVAudioPlayerNode *node;
    int bufferID;
    bool active;
    bool is3D;
};

struct AVBridgeBufferEntry {
    AVAudioPCMBuffer *buffer;
    AVAudioFormat *format;
    int refCount;
};

#pragma mark - Global State

static AVAudioEngine *gEngine = nil;
static AVAudioEnvironmentNode *gEnvNode = nil;
static AVAudioMixerNode *gMixer2D = nil;

static int gMaxNodes = 0;
static AVBridgePlayerSlot *gSlots = nullptr;
static int gNextBufferID = 1;

static NSMutableDictionary<NSNumber*, NSValue*> *gBufferMap = nil;

static os_unfair_lock gLock = OS_UNFAIR_LOCK_INIT;

#pragma mark - WAV Parser

struct WavParseResult {
    const uint8_t *pcmStart;
    uint32_t pcmBytes;
    uint16_t channels;
    uint32_t sampleRate;
    uint16_t bitsPerSample;
};

static uint16_t wav_u16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static uint32_t wav_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool wav_parse(const uint8_t *data, size_t len, WavParseResult *out) {
    if (len < 44) return false;
    if (memcmp(data, "RIFF", 4) != 0) return false;
    if (memcmp(data + 8, "WAVE", 4) != 0) return false;

    const uint8_t *fmtChunk = nullptr;
    uint32_t fmtSize = 0;
    const uint8_t *dataChunk = nullptr;
    uint32_t dataSize = 0;

    size_t pos = 12;
    while (pos + 8 <= len) {
        uint32_t chunkSize = wav_u32(data + pos + 4);
        if (pos + 8 + chunkSize > len) break;

        if (memcmp(data + pos, "fmt ", 4) == 0) {
            fmtChunk = data + pos + 8;
            fmtSize = chunkSize;
        } else if (memcmp(data + pos, "data", 4) == 0) {
            dataChunk = data + pos + 8;
            dataSize = chunkSize;
        }

        pos += 8 + chunkSize;
        if (pos & 1) pos++;
    }

    if (!fmtChunk || fmtSize < 16 || !dataChunk || dataSize == 0) return false;

    uint16_t audioFmt = wav_u16(fmtChunk);
    if (audioFmt != 1) return false;

    out->channels      = wav_u16(fmtChunk + 2);
    out->sampleRate    = wav_u32(fmtChunk + 4);
    out->bitsPerSample = wav_u16(fmtChunk + 14);
    out->pcmStart      = dataChunk;
    out->pcmBytes      = dataSize;

    if (out->channels == 0 || out->channels > 2) return false;
    if (out->bitsPerSample != 8 && out->bitsPerSample != 16) return false;
    if (out->sampleRate == 0 || out->sampleRate > 96000) return false;
    if (out->pcmBytes > 50 * 1024 * 1024) return false;

    return true;
}

#pragma mark - Internal Helpers

static AVAudioPCMBuffer *createPCMBuffer(const uint8_t *pcmData, uint32_t pcmBytes,
                                          uint32_t sampleRate, uint16_t channels,
                                          uint16_t bitsPerSample) {
    AVAudioCommonFormat commonFmt;
    uint32_t bytesPerSample;

    if (bitsPerSample == 16) {
        commonFmt = AVAudioPCMFormatInt16;
        bytesPerSample = 2;
    } else {
        commonFmt = AVAudioOtherFormat;
        bytesPerSample = 1;
    }

    if (commonFmt == AVAudioOtherFormat) {
        AVAudioFormat *fmt16 = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatInt16
                                                               sampleRate:sampleRate
                                                                 channels:channels
                                                              interleaved:YES];
        uint32_t numSamples = pcmBytes / channels;
        AVAudioPCMBuffer *buf = [[AVAudioPCMBuffer alloc] initWithPCMFormat:fmt16
                                                             frameCapacity:numSamples];
        buf.frameLength = numSamples;

        int16_t *dst = buf.int16ChannelData[0];
        for (uint32_t i = 0; i < pcmBytes; i++) {
            dst[i] = (int16_t)((pcmData[i] - 128) << 8);
        }
        return buf;
    }

    AVAudioFormat *fmt = [[AVAudioFormat alloc] initWithCommonFormat:commonFmt
                                                         sampleRate:sampleRate
                                                           channels:channels
                                                        interleaved:YES];
    if (!fmt) return nil;

    uint32_t framesTotal = pcmBytes / (bytesPerSample * channels);
    AVAudioPCMBuffer *buf = [[AVAudioPCMBuffer alloc] initWithPCMFormat:fmt
                                                         frameCapacity:framesTotal];
    if (!buf) return nil;

    buf.frameLength = framesTotal;
    memcpy(buf.int16ChannelData[0], pcmData, pcmBytes);
    return buf;
}

static int findFreeSlot(void) {
    for (int i = 0; i < gMaxNodes; i++) {
        if (!gSlots[i].active) return i;
    }
    return -1;
}

static void detachAndReattach(int slotIdx, bool to3D, AVAudioFormat *format) {
    AVAudioPlayerNode *node = gSlots[slotIdx].node;

    if ([node engine] != nil) {
        [gEngine disconnectNodeOutput:node];
    }

    if (to3D) {
        [gEngine connect:node to:gEnvNode format:format];
    } else {
        [gEngine connect:node to:gMixer2D format:format];
    }
}

static void ensure_engine_inited(void) {
    if (gEngine) return;

    gEngine = [[AVAudioEngine alloc] init];
    gEnvNode = [[AVAudioEnvironmentNode alloc] init];
    gEnvNode.distanceAttenuationParameters.distanceAttenuationModel = AVAudioEnvironmentDistanceAttenuationModelInverse;
    gEnvNode.distanceAttenuationParameters.referenceDistance = 300.0f;
    gEnvNode.distanceAttenuationParameters.maximumDistance = 2000.0f;
    gEnvNode.distanceAttenuationParameters.rolloffFactor = 1.0f;
    gMixer2D = [[AVAudioMixerNode alloc] init];

    [gEngine attachNode:gEnvNode];
    [gEngine attachNode:gMixer2D];

    AVAudioMixerNode *mainMixer = [gEngine mainMixerNode];
    [gEngine connect:gEnvNode to:mainMixer format:nil];
    [gEngine connect:gMixer2D to:mainMixer format:nil];

    gEnvNode.listenerPosition = AVAudioMake3DPoint(0, 0, 0);
    gEnvNode.listenerVectorOrientation = AVAudioMake3DVectorOrientation(
        AVAudioMake3DVector(0, 1, 0),
        AVAudioMake3DVector(0, 0, 1)
    );

    for (int i = 0; i < gMaxNodes; i++) {
        AVAudioPlayerNode *node = [[AVAudioPlayerNode alloc] init];
        [gEngine attachNode:node];
        gSlots[i].node = node;
        gSlots[i].active = false;
        gSlots[i].bufferID = 0;
        gSlots[i].is3D = false;
    }

    NSError *error = nil;
    if (![gEngine startAndReturnError:&error]) {
        printf("AVAudioBridge: AVAudioEngine start FAILED: %s\n", error.localizedDescription.UTF8String);
        fflush(stdout);
    } else {
        gEngineStarted = true;
        printf("AVAudioBridge: AVAudioEngine started safely. %d player nodes.\n", gMaxNodes);
        fflush(stdout);
    }
}

#pragma mark - Public API: Init / Shutdown

bool avbridge_init(int maxNodes) {
    if (gSlots) return true;

    gMaxNodes = maxNodes;
    gSlots = (AVBridgePlayerSlot *)calloc(maxNodes, sizeof(AVBridgePlayerSlot));
    gBufferMap = [NSMutableDictionary dictionary];

    printf("avbridge_init: Data structures initialized. Engine creation deferred.\n");
    fflush(stdout);
    return true;
}

void avbridge_shutdown(void) {
    if (!gEngine) return;

    [gEngine stop];

    for (int i = 0; i < gMaxNodes; i++) {
        [gSlots[i].node stop];
    }

    gBufferMap = nil;
    gEnvNode = nil;
    gMixer2D = nil;
    gEngine = nil;

    free(gSlots);
    gSlots = nullptr;
    gMaxNodes = 0;

    printf("avbridge_shutdown: done\n");
    fflush(stdout);
}

#pragma mark - Public API: Buffer Management

int avbridge_loadBuffer(const uint8_t *pcmData, uint32_t pcmBytes,
                        uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample) {
    AVAudioPCMBuffer *buf = createPCMBuffer(pcmData, pcmBytes, sampleRate, channels, bitsPerSample);
    if (!buf) return 0;

    os_unfair_lock_lock(&gLock);
    int id = gNextBufferID++;

    AVBridgeBufferEntry *entry = (AVBridgeBufferEntry *)calloc(1, sizeof(AVBridgeBufferEntry));
    entry->buffer = buf;
    entry->format = buf.format;
    entry->refCount = 1;

    gBufferMap[@(id)] = [NSValue valueWithPointer:entry];
    os_unfair_lock_unlock(&gLock);

    return id;
}

void avbridge_unloadBuffer(int bufferID) {
    os_unfair_lock_lock(&gLock);
    NSValue *val = gBufferMap[@(bufferID)];
    if (val) {
        AVBridgeBufferEntry *entry = (AVBridgeBufferEntry *)[val pointerValue];
        entry->refCount--;
        if (entry->refCount <= 0) {
            free(entry);
            [gBufferMap removeObjectForKey:@(bufferID)];
        }
    }
    os_unfair_lock_unlock(&gLock);
}

static AVBridgeBufferEntry *getBufferEntry(int bufferID) {
    NSValue *val = gBufferMap[@(bufferID)];
    if (!val) return nullptr;
    return (AVBridgeBufferEntry *)[val pointerValue];
}

#pragma mark - Public API: Playback

int avbridge_play(int bufferID, float gain, float pitch, bool loop) {
    ensure_engine_inited();
    if (!gEngineStarted) return -1;

    AVBridgeBufferEntry *entry = getBufferEntry(bufferID);
    if (!entry) return -1;

    os_unfair_lock_lock(&gLock);
    int idx = findFreeSlot();
    if (idx < 0) {
        os_unfair_lock_unlock(&gLock);
        return -1;
    }
    gSlots[idx].active = true;
    gSlots[idx].bufferID = bufferID;
    gSlots[idx].is3D = false;
    os_unfair_lock_unlock(&gLock);

    detachAndReattach(idx, false, entry->format);

    AVAudioPlayerNode *node = gSlots[idx].node;
    node.volume = gain;
    node.rate = pitch;

    AVAudioPlayerNodeBufferOptions opts = loop ? AVAudioPlayerNodeBufferLoops : 0;
    [node scheduleBuffer:entry->buffer atTime:nil options:opts completionHandler:^{
        os_unfair_lock_lock(&gLock);
        gSlots[idx].active = false;
        gSlots[idx].bufferID = 0;
        os_unfair_lock_unlock(&gLock);
    }];
    [node play];

    return idx;
}

int avbridge_play3D(int bufferID, float gain, float pitch,
                    float x, float y, float z, float maxDist, float refDist) {
    ensure_engine_inited();
    if (!gEngineStarted) return -1;

    AVBridgeBufferEntry *entry = getBufferEntry(bufferID);
    if (!entry) return -1;

    os_unfair_lock_lock(&gLock);
    int idx = findFreeSlot();
    if (idx < 0) {
        os_unfair_lock_unlock(&gLock);
        return -1;
    }
    gSlots[idx].active = true;
    gSlots[idx].bufferID = bufferID;
    gSlots[idx].is3D = true;
    os_unfair_lock_unlock(&gLock);

    detachAndReattach(idx, true, entry->format);

    AVAudioPlayerNode *node = gSlots[idx].node;
    node.volume = gain;
    node.rate = pitch;
    node.position = AVAudioMake3DPoint(x, y, z);
    node.renderingAlgorithm = AVAudio3DMixingRenderingAlgorithmEqualPowerPanning;

    if (refDist > 0) node.reverbBlend = 0;

    [node scheduleBuffer:entry->buffer atTime:nil options:0 completionHandler:^{
        os_unfair_lock_lock(&gLock);
        gSlots[idx].active = false;
        gSlots[idx].bufferID = 0;
        os_unfair_lock_unlock(&gLock);
    }];
    [node play];

    return idx;
}

#pragma mark - Public API: Control

void avbridge_stop(int playerID) {
    if (playerID < 0 || playerID >= gMaxNodes) return;
    if (!gSlots[playerID].active) return;

    [gSlots[playerID].node stop];

    os_unfair_lock_lock(&gLock);
    gSlots[playerID].active = false;
    gSlots[playerID].bufferID = 0;
    os_unfair_lock_unlock(&gLock);
}

void avbridge_stopAll(void) {
    for (int i = 0; i < gMaxNodes; i++) {
        if (gSlots[i].active) {
            [gSlots[i].node stop];
            gSlots[i].active = false;
            gSlots[i].bufferID = 0;
        }
    }
}

bool avbridge_isPlaying(int playerID) {
    if (playerID < 0 || playerID >= gMaxNodes) return false;
    return gSlots[playerID].active && gSlots[playerID].node.isPlaying;
}

void avbridge_setVolume(int playerID, float gain) {
    if (playerID < 0 || playerID >= gMaxNodes) return;
    gSlots[playerID].node.volume = gain;
}

void avbridge_setPitch(int playerID, float pitch) {
    if (playerID < 0 || playerID >= gMaxNodes) return;
    gSlots[playerID].node.rate = pitch;
}

void avbridge_pause(int playerID) {
    if (playerID < 0 || playerID >= gMaxNodes) return;
    [gSlots[playerID].node pause];
}

void avbridge_resume(int playerID) {
    if (playerID < 0 || playerID >= gMaxNodes) return;
    [gSlots[playerID].node play];
}

void avbridge_pauseAll(void) {
    [gEngine pause];
}

void avbridge_resumeAll(void) {
    NSError *err = nil;
    [gEngine startAndReturnError:&err];
}

#pragma mark - Public API: Listener

void avbridge_setListenerPosition(float x, float y, float z,
                                  float fwdX, float fwdY, float fwdZ,
                                  float upX, float upY, float upZ) {
    if (!gEnvNode) return; // Silent discard if engine not yet started
    gEnvNode.listenerPosition = AVAudioMake3DPoint(x, y, z);
    gEnvNode.listenerVectorOrientation = AVAudioMake3DVectorOrientation(
        AVAudioMake3DVector(fwdX, fwdY, fwdZ),
        AVAudioMake3DVector(upX, upY, upZ)
    );
}

#pragma mark - Public API: Cleanup (no-op, handled by completion callbacks)

void avbridge_cleanup(void) {
}
