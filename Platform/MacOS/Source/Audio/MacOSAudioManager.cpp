#include "MacOSAudioManager.h"
#include "Common/AudioAffect.h"
#include "Common/AudioEventInfo.h"
#include "Common/AudioEventRTS.h"
#include "Common/AudioRequest.h"
#include "Common/Debug.h"
#include "Common/GameMemory.h"
#include "Common/FileSystem.h"
#include "Common/file.h"
#include "../Utils/MacDebug.h"
#include <unistd.h>

#define Byte MacByte
#define RGBColor MacRGBColor
#define BOOL MacBOOL
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#undef Byte
#undef RGBColor
#undef BOOL

extern FileSystem *TheFileSystem;

static CFURLRef CreateTempAudioFileURL(const std::string& pathStr, const std::string& originalPath) {
    if (pathStr.empty()) return nullptr;

    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    std::string fullPath = std::string(cwd) + "/" + pathStr;
    
    // Check if it exists on disk loosely
    FILE *chk = fopen(fullPath.c_str(), "rb");
    if (chk) {
        fclose(chk);
        DEBUG_AUDIO_MAC(("Found loosely on disk: %s", fullPath.c_str()));
        CFStringRef cfPath = CFStringCreateWithCString(kCFAllocatorDefault, fullPath.c_str(), kCFStringEncodingUTF8);
        CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfPath, kCFURLPOSIXPathStyle, false);
        CFRelease(cfPath);
        return url;
    }

    // Try extracting from .big via TheFileSystem using the original engine path (Slash Boundary contract)
    if (TheFileSystem && TheFileSystem->doesFileExist(originalPath.c_str())) {
        File *f = TheFileSystem->openFile(originalPath.c_str(), File::READ);
        if (f) {
            size_t fileSize = f->size();
            char *buffer = static_cast<char*>(f->readEntireAndClose());
            if (buffer && fileSize > 0) {
                // Find filename only
                size_t slashPos = pathStr.find_last_of("\\/");
                std::string fileName = (slashPos != std::string::npos) ? pathStr.substr(slashPos + 1) : pathStr;
                
                char tmpFolder[1024];
                if (confstr(_CS_DARWIN_USER_TEMP_DIR, tmpFolder, sizeof(tmpFolder)) == 0) {
                    strcpy(tmpFolder, "/tmp/");
                }
                std::string tempPath = std::string(tmpFolder) + fileName;
                FILE *out = fopen(tempPath.c_str(), "wb");
                if (out) {
                    fwrite(buffer, 1, fileSize, out);
                    fclose(out);
                    DEBUG_AUDIO_MAC(("Extracted from Big to %s (source: %s)", tempPath.c_str(), originalPath.c_str()));
                } else {
                    DEBUG_AUDIO_MAC(("FAILED to write to %s", tempPath.c_str()));
                }
                delete[] buffer;
                
                CFStringRef cfPath = CFStringCreateWithCString(kCFAllocatorDefault, tempPath.c_str(), kCFStringEncodingUTF8);
                CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfPath, kCFURLPOSIXPathStyle, false);
                CFRelease(cfPath);
                return url;
            } else {
                DEBUG_AUDIO_MAC(("File read error from TheFileSystem: %s", originalPath.c_str()));
            }
        }
    } else {
        DEBUG_AUDIO_MAC(("TheFileSystem->doesFileExist FAILED for '%s'", originalPath.c_str()));
    }
    return nullptr;
}

MacOSAudioManager::MacOSAudioManager() : m_device(nullptr), m_context(nullptr) {}

MacOSAudioManager::~MacOSAudioManager() {
    for (auto &pa : m_sources) {
        if (pa.sourceID != 0) {
            alDeleteSources(1, &pa.sourceID);
        }
    }
    m_sources.clear();

    for (auto &kv : m_bufferCache) {
        alDeleteBuffers(1, &kv.second.bufferID);
    }
    m_bufferCache.clear();

    if (m_context) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(m_context);
        m_context = nullptr;
    }
    if (m_device) {
        alcCloseDevice(m_device);
        m_device = nullptr;
    }
}

void MacOSAudioManager::init() {
    AudioManager::init();

    m_device = alcOpenDevice(nullptr); // default device
    if (!m_device) {
        fprintf(stderr, "MACOS AUDIO: Failed to open OpenAL device\n");
        return;
    }

    m_context = alcCreateContext(m_device, nullptr);
    if (!m_context || !alcMakeContextCurrent(m_context)) {
        fprintf(stderr, "MACOS AUDIO: Failed to create or set OpenAL context\n");
        return;
    }

    // Pre-allocate N sources (Miles flow)
    for (int i = 0; i < MAX_SOURCES; ++i) {
        ALuint sid = 0;
        alGenSources(1, &sid);
        if (alGetError() == AL_NO_ERROR) {
            PlayingAudio pa;
            pa.sourceID = sid;
            pa.isPlaying = FALSE;
            pa.eventRTS = nullptr;
            pa.handle = 0;
            pa.priority = 0;
            m_sources.push_back(pa);
        }
    }
    fprintf(stderr, "MACOS AUDIO: OpenAL Init Success. Preallocated %zu sources.\n", m_sources.size());
}

void MacOSAudioManager::reset() {
    AudioManager::reset();
    for (auto &pa : m_sources) {
        alSourceStop(pa.sourceID);
        alSourcei(pa.sourceID, AL_BUFFER, 0);
        pa.isPlaying = FALSE;
        pa.eventRTS = nullptr;
        pa.handle = 0;
    }
}

void MacOSAudioManager::update() {
    AudioManager::update();
    
    if (m_audioRequests.size() > 0) {
        DEBUG_AUDIO_MAC(("processRequestList running with %zu requests", m_audioRequests.size()));
    }
    
    setDeviceListenerPosition();
    processRequestList();

    // Check playing status
    for (auto &pa : m_sources) {
        if (pa.isPlaying) {
            ALint state;
            alGetSourcei(pa.sourceID, AL_SOURCE_STATE, &state);
            if (state == AL_STOPPED) {
                stopSourceAndFree(pa);
            }
        }
    }
}

void MacOSAudioManager::stopSourceAndFree(PlayingAudio &pa) {
    if (pa.sourceID != 0) {
        alSourceStop(pa.sourceID);
        alSourcei(pa.sourceID, AL_BUFFER, 0);
    }
    pa.isPlaying = FALSE;
    pa.handle = 0;
    // Don't delete eventRTS here if handled closely, though if AR_Play handled it,
    // we take ownership. If taking ownership, we must delete.
    if (pa.eventRTS) {
        delete pa.eventRTS;
        pa.eventRTS = nullptr;
    }
}

PlayingAudio* MacOSAudioManager::findFreeSource(int priorityToDemand) {
    PlayingAudio *lowestPriorityPlaying = nullptr;
    int lowestPri = 999999;

    for (auto &pa : m_sources) {
        if (!pa.isPlaying) {
            return &pa;
        }
        if (pa.priority < lowestPri) {
            lowestPri = pa.priority;
            lowestPriorityPlaying = &pa;
        }
    }

    // Kill lowest priority if demanding higher
    if (priorityToDemand > lowestPri && lowestPriorityPlaying) {
        stopSourceAndFree(*lowestPriorityPlaying);
        return lowestPriorityPlaying;
    }
    return nullptr;
}

ALuint MacOSAudioManager::loadAudioFileIntoBuffer(const AsciiString& path, bool forceMono) {
    std::string originalPath = path.str();
    std::string pathStr = originalPath;
    for (size_t i = 0; i < pathStr.length(); ++i) {
        if (pathStr[i] == '\\') pathStr[i] = '/';
    }

    std::string cacheKey = originalPath + (forceMono ? "_mono" : "_stereo");
    auto hit = m_bufferCache.find(cacheKey); // Cache by path and format
    if (hit != m_bufferCache.end()) {
        return hit->second.bufferID;
    }

    CFURLRef url = CreateTempAudioFileURL(pathStr, originalPath);
    if (!url) {
        DEBUG_AUDIO_MAC(("loadAudio: CreateTempAudioFileURL failed for %s", pathStr.c_str()));
        return 0;
    }

    ExtAudioFileRef audioFile = nullptr;
    OSStatus err = ExtAudioFileOpenURL(url, &audioFile);
    CFRelease(url);
    if (err != noErr || !audioFile) {
        DEBUG_AUDIO_MAC(("loadAudio: ExtAudioFileOpenURL failed with err %d for %s", (int)err, pathStr.c_str()));
        return 0;
    }

    AudioStreamBasicDescription fileFormat;
    UInt32 size = sizeof(fileFormat);
    ExtAudioFileGetProperty(audioFile, kExtAudioFileProperty_FileDataFormat, &size, &fileFormat);

    AudioStreamBasicDescription clientFormat;
    memset(&clientFormat, 0, sizeof(clientFormat));
    clientFormat.mSampleRate = fileFormat.mSampleRate;
    clientFormat.mFormatID = kAudioFormatLinearPCM;
    clientFormat.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    clientFormat.mBitsPerChannel = 16;
    clientFormat.mChannelsPerFrame = forceMono ? 1 : fileFormat.mChannelsPerFrame;
    clientFormat.mFramesPerPacket = 1;
    clientFormat.mBytesPerFrame = (clientFormat.mBitsPerChannel / 8) * clientFormat.mChannelsPerFrame;
    clientFormat.mBytesPerPacket = clientFormat.mBytesPerFrame;

    ExtAudioFileSetProperty(audioFile, kExtAudioFileProperty_ClientDataFormat, sizeof(clientFormat), &clientFormat);

    SInt64 numFrames = 0;
    size = sizeof(numFrames);
    ExtAudioFileGetProperty(audioFile, kExtAudioFileProperty_FileLengthFrames, &size, &numFrames);

    UInt32 totalBytes = numFrames * clientFormat.mBytesPerFrame;
    uint8_t *audioData = new uint8_t[totalBytes];

    AudioBufferList bufList;
    bufList.mNumberBuffers = 1;
    bufList.mBuffers[0].mNumberChannels = clientFormat.mChannelsPerFrame;
    bufList.mBuffers[0].mDataByteSize = totalBytes;
    bufList.mBuffers[0].mData = audioData;

    UInt32 framesToRead = (UInt32)numFrames;
    ExtAudioFileRead(audioFile, &framesToRead, &bufList);
    ExtAudioFileDispose(audioFile);

    ALuint bufferID = 0;
    alGenBuffers(1, &bufferID);
    
    ALenum format = (clientFormat.mChannelsPerFrame == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    alBufferData(bufferID, format, audioData, totalBytes, clientFormat.mSampleRate);
    delete[] audioData;

    if (alGetError() != AL_NO_ERROR) {
        DEBUG_AUDIO_MAC(("loadAudio: alBufferData failed for %s", pathStr.c_str()));
        alDeleteBuffers(1, &bufferID);
        return 0;
    }
    
    DEBUG_AUDIO_MAC(("loadAudio: Successfully loaded %s into OpenAL buffer %d (frames=%lld, bytes=%u)", pathStr.c_str(), bufferID, numFrames, (unsigned)totalBytes));

    AudioBufferCacheEntry entry;
    entry.path = path;
    entry.bufferID = bufferID;
    entry.refCount = 1;
    entry.valid = true;
    m_bufferCache[cacheKey] = entry;

    return bufferID;
}

void MacOSAudioManager::processRequestList() {
    for (auto it = m_audioRequests.begin(); it != m_audioRequests.end();) {
        AudioRequest *req = *it;
        if (!req) {
            it = m_audioRequests.erase(it);
            continue;
        }

        switch (req->m_request) {
            case AR_Play: {
                if (req->m_usePendingEvent && req->m_pendingEvent) {
                    friend_forcePlayAudioEventRTS(req->m_pendingEvent);
                    req->m_pendingEvent = nullptr;
                }
                break;
            }
            case AR_Stop: {
                for (auto &pa : m_sources) {
                    if (pa.isPlaying && pa.handle == req->m_handleToInteractOn) {
                        alSourceStop(pa.sourceID);
                        pa.isPlaying = FALSE;
                        pa.handle = 0;
                        if (pa.eventRTS) {
                            delete pa.eventRTS;
                            pa.eventRTS = nullptr;
                        }
                    }
                }
                break;
            }
            case AR_Pause:
                break;
        }

        deleteInstance(req);
        it = m_audioRequests.erase(it);
    }
}

void MacOSAudioManager::friend_forcePlayAudioEventRTS(const AudioEventRTS *eventToPlay) {
    if (!eventToPlay) return;

    AudioEventRTS *event = const_cast<AudioEventRTS*>(eventToPlay);
    event->generateFilename();
    AsciiString filename = event->getFilename();
    if (filename.isEmpty()) {
        DEBUG_AUDIO_MAC(("forcePlay: Filename is empty. Deleting event."));
        delete event;
        return;
    }

    bool isPos = (event->getPosition() != nullptr && event->isPositionalAudio());
    ALuint buffer = loadAudioFileIntoBuffer(filename, isPos);
    if (!buffer) {
        DEBUG_AUDIO_MAC(("forcePlay: buffer failed to load for %s. Deleting event.", filename.str()));
        delete event;
        return;
    }

    int priority = 50; 
    const AudioEventInfo *info = event->getAudioEventInfo();
    if (info) priority = info->m_priority;

    PlayingAudio *pa = findFreeSource(priority);
    if (!pa) {
        DEBUG_AUDIO_MAC(("forcePlay: No free source for %s (priority %d). Deleting event.", filename.str(), priority));
        delete event;
        return;
    }

    // Setup source
    alSourcei(pa->sourceID, AL_BUFFER, buffer);
    
    // Volume & Pitch
    float baseVol = 1.0f;
    if (info) {
        if (info->m_soundType == AT_Music) baseVol = getVolume(AudioAffect_Music);
        else if (info->m_soundType == AT_Streaming) baseVol = getVolume(AudioAffect_Speech);
        else baseVol = getVolume(AudioAffect_Sound);
    } else {
        baseVol = getVolume(AudioAffect_Sound);
    }
    
    alSourcef(pa->sourceID, AL_GAIN, event->getVolume() * baseVol);
    alSourcef(pa->sourceID, AL_PITCH, event->getPitchShift() > 0 ? event->getPitchShift() : 1.0f);

    // 3D Audio Pos
    const Coord3D *pos = event->getPosition();
    if (pos && event->isPositionalAudio()) {
        alSourcei(pa->sourceID, AL_SOURCE_RELATIVE, AL_FALSE);
        alSource3f(pa->sourceID, AL_POSITION, pos->x, pos->y, pos->z);
        // Default max dist
        alSourcef(pa->sourceID, AL_MAX_DISTANCE, 500.0f);
        alSourcef(pa->sourceID, AL_REFERENCE_DISTANCE, 50.0f);
    } else {
        alSourcei(pa->sourceID, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(pa->sourceID, AL_POSITION, 0, 0, 0);
    }

    alSourcePlay(pa->sourceID);

    DEBUG_AUDIO_MAC(("forcePlay: PLAYING %s! SourceID=%d, Volume=%.2f, Pitch=%.2f", filename.str(), pa->sourceID, event->getVolume() * baseVol, event->getPitchShift() > 0 ? event->getPitchShift() : 1.0f));

    pa->isPlaying = TRUE;
    pa->eventRTS = event;
    pa->handle = event->getPlayingHandle();
    pa->priority = priority;
}

void MacOSAudioManager::setDeviceListenerPosition() {
    ALfloat pos[] = { m_listenerPosition.x, m_listenerPosition.y, m_listenerPosition.z };
    alListenerfv(AL_POSITION, pos);

    // Orientation expects 6 floats: "forward" vector, then "up" vector.
    // C&C Generals uses Z as UP. m_listenerOrientation usually dictates forward vector.
    ALfloat ori[] = { m_listenerOrientation.x, m_listenerOrientation.y, m_listenerOrientation.z, 0.0f, 0.0f, 1.0f };
    alListenerfv(AL_ORIENTATION, ori);
}

Bool MacOSAudioManager::isCurrentlyPlaying(AudioHandle handle) {
    if (handle == 0) return FALSE;
    for (auto &pa : m_sources) {
        if (pa.isPlaying && pa.handle == handle) {
            ALint state;
            alGetSourcei(pa.sourceID, AL_SOURCE_STATE, &state);
            return (state == AL_PLAYING) ? TRUE : FALSE;
        }
    }
    return FALSE;
}

void MacOSAudioManager::stopAudio(AudioAffect which) {
    for (auto &pa : m_sources) {
        if (pa.isPlaying) {
            alSourceStop(pa.sourceID);
            pa.isPlaying = FALSE;
            pa.handle = 0;
            if (pa.eventRTS) { delete pa.eventRTS; pa.eventRTS = nullptr; }
        }
    }
}
void MacOSAudioManager::pauseAudio(AudioAffect which) {
    for (auto &pa : m_sources) {
        if (pa.isPlaying) alSourcePause(pa.sourceID);
    }
}
void MacOSAudioManager::resumeAudio(AudioAffect which) {
    for (auto &pa : m_sources) {
        if (pa.isPlaying) {
            ALint state;
            alGetSourcei(pa.sourceID, AL_SOURCE_STATE, &state);
            if (state == AL_PAUSED) alSourcePlay(pa.sourceID);
        }
    }
}

void MacOSAudioManager::pauseAmbient(Bool shouldPause) {}
void MacOSAudioManager::killAudioEventImmediately(AudioHandle audioEvent) {
    for (auto &pa : m_sources) {
        if (pa.isPlaying && pa.handle == audioEvent) {
            stopSourceAndFree(pa);
        }
    }
}
void MacOSAudioManager::nextMusicTrack() {}
void MacOSAudioManager::prevMusicTrack() {}
Bool MacOSAudioManager::isMusicPlaying() const { return FALSE; }
Bool MacOSAudioManager::isMusicAlreadyLoaded() const { return TRUE; }
Bool MacOSAudioManager::hasMusicTrackCompleted(const AsciiString &trackName, Int numberOfTimes) const { return FALSE; }
AsciiString MacOSAudioManager::getMusicTrackName() const { return ""; }
void MacOSAudioManager::openDevice() {}
void MacOSAudioManager::closeDevice() {}
void *MacOSAudioManager::getDevice() { return nullptr; }
void MacOSAudioManager::notifyOfAudioCompletion(UnsignedInt audioCompleted, UnsignedInt flags) {}
UnsignedInt MacOSAudioManager::getProviderCount() const { return 1; }
AsciiString MacOSAudioManager::getProviderName(UnsignedInt providerNum) const { return "MacOS OpenAL Audio"; }
UnsignedInt MacOSAudioManager::getProviderIndex(AsciiString providerName) const { return 0; }
void MacOSAudioManager::selectProvider(UnsignedInt providerNdx) {}
void MacOSAudioManager::unselectProvider() {}
UnsignedInt MacOSAudioManager::getSelectedProvider() const { return 0; }
void MacOSAudioManager::setSpeakerType(UnsignedInt speakerType) {}
UnsignedInt MacOSAudioManager::getSpeakerType() { return 0; }
UnsignedInt MacOSAudioManager::getNum2DSamples() const { return MAX_SOURCES; }
UnsignedInt MacOSAudioManager::getNum3DSamples() const { return MAX_SOURCES; }
UnsignedInt MacOSAudioManager::getNumStreams() const { return 8; }
Bool MacOSAudioManager::doesViolateLimit(AudioEventRTS *event) const { return FALSE; }
Bool MacOSAudioManager::isPlayingLowerPriority(AudioEventRTS *event) const { return FALSE; }
Bool MacOSAudioManager::isPlayingAlready(AudioEventRTS *event) const { return FALSE; }
Bool MacOSAudioManager::isObjectPlayingVoice(UnsignedInt objID) const { return FALSE; }
void MacOSAudioManager::adjustVolumeOfPlayingAudio(AsciiString eventName, Real newVolume) {}
void MacOSAudioManager::removePlayingAudio(AsciiString eventName) {}
void MacOSAudioManager::removeAllDisabledAudio() {}
Bool MacOSAudioManager::has3DSensitiveStreamsPlaying() const { return FALSE; }
void *MacOSAudioManager::getHandleForBink() { return nullptr; }
void MacOSAudioManager::releaseHandleForBink() {}
void MacOSAudioManager::setPreferredProvider(AsciiString providerNdx) {}
void MacOSAudioManager::setPreferredSpeaker(AsciiString speakerType) {}
Real MacOSAudioManager::getFileLengthMS(AsciiString strToLoad) const { return 0.0f; }
void MacOSAudioManager::closeAnySamplesUsingFile(const void *fileToClose) {}

#if defined(RTS_DEBUG)
void MacOSAudioManager::audioDebugDisplay(DebugDisplayInterface *dd, void *userData, FILE *fp) {}
#endif
