#include "MacOSAudioManager.h"
#include "Common/AudioAffect.h"
#include "Common/AudioEventInfo.h"
#include "Common/AudioEventRTS.h"
#include "Common/AudioHandleSpecialValues.h"
#include "Common/AudioRequest.h"
#include "Common/Debug.h"
#include "Common/GameMemory.h"
#include "Common/FileSystem.h"
#include "Common/file.h"
#include "Common/System/NativeFileSystem.h"
#include "Common/GlobalData.h"
#include "../Utils/MacDebug.h"
#include <unistd.h>

extern FileSystem *TheFileSystem;

static const char* AUDIO_CACHE_DIR_FORMAT = "%sAudioCache/";

#pragma mark - WAV Loading from Engine FileSystem

struct WavParseResult {
    const uint8_t *pcmStart;
    uint32_t pcmBytes;
    uint16_t channels;
    uint32_t sampleRate;
    uint16_t bitsPerSample;
};

static uint16_t wav_read_u16(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static uint32_t wav_read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool parseWavHeader(const uint8_t *data, size_t len, WavParseResult *out) {
    if (len < 44) return false;
    if (memcmp(data, "RIFF", 4) != 0 || memcmp(data + 8, "WAVE", 4) != 0) return false;

    const uint8_t *fmtChunk = nullptr;
    uint32_t fmtSize = 0;
    const uint8_t *dataChunk = nullptr;
    uint32_t dataSize = 0;

    size_t pos = 12;
    while (pos + 8 <= len) {
        uint32_t chunkSize = wav_read_u32(data + pos + 4);
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
    if (wav_read_u16(fmtChunk) != 1) return false;

    out->channels = wav_read_u16(fmtChunk + 2);
    out->sampleRate = wav_read_u32(fmtChunk + 4);
    out->bitsPerSample = wav_read_u16(fmtChunk + 14);
    out->pcmStart = dataChunk;
    out->pcmBytes = dataSize;

    if (out->channels == 0 || out->channels > 2) return false;
    if (out->bitsPerSample != 8 && out->bitsPerSample != 16) return false;
    if (out->sampleRate == 0 || out->sampleRate > 96000) return false;
    if (out->pcmBytes > 50 * 1024 * 1024) return false;

    return true;
}

static bool loadWavFromDisk(const std::string &pathStr, uint8_t **outData, size_t *outSize) {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    std::string fullPath = std::string(cwd) + "/" + pathStr;

    FILE *f = fopen(fullPath.c_str(), "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fileSize <= 0 || fileSize > 50 * 1024 * 1024) { fclose(f); return false; }

    uint8_t *buf = (uint8_t*)malloc(fileSize);
    if (!buf) { fclose(f); return false; }

    size_t rd = fread(buf, 1, fileSize, f);
    fclose(f);

    if ((long)rd != fileSize) { free(buf); return false; }

    *outData = buf;
    *outSize = (size_t)fileSize;
    return true;
}

static bool loadWavFromBig(const std::string &originalPath, uint8_t **outData, size_t *outSize) {
    if (!TheFileSystem || !TheFileSystem->doesFileExist(originalPath.c_str())) return false;

    File *f = TheFileSystem->openFile(originalPath.c_str(), File::READ);
    if (!f) return false;

    size_t fileSize = f->size();
    if (fileSize == 0 || fileSize > 50 * 1024 * 1024) { f->close(); return false; }

    uint8_t *buf = (uint8_t*)malloc(fileSize);
    if (!buf) { f->close(); return false; }

    Int bytesRead = f->read(buf, (Int)fileSize);
    f->close();

    if (bytesRead <= 0 || (size_t)bytesRead != fileSize) { free(buf); return false; }

    *outData = buf;
    *outSize = fileSize;
    return true;
}

#pragma mark - MacOSAudioManager Lifecycle

MacOSAudioManager::MacOSAudioManager() {}

MacOSAudioManager::~MacOSAudioManager() {
    avbridge_shutdown();
}

void MacOSAudioManager::init() {
    AudioManager::init();

    if (!avbridge_init(MAX_SOURCES)) {
        printf("MACOS AUDIO: AVAudioEngine init FAILED!\n");
        fflush(stdout);
        return;
    }

    for (int i = 0; i < MAX_SOURCES; ++i) {
        PlayingAudio pa;
        pa.playerID = -1;
        pa.isPlaying = FALSE;
        pa.eventRTS = nullptr;
        pa.handle = 0;
        pa.priority = 0;
        m_sources.push_back(pa);
    }

    printf("MACOS AUDIO: AVAudioEngine Init Success. %d source slots.\n", MAX_SOURCES);
    fflush(stdout);
}

void MacOSAudioManager::reset() {
    int activeCount = 0;
    for (auto &pa : m_sources) {
        if (pa.isPlaying) activeCount++;
    }
    DEBUG_AUDIO_MAC(("MacOSAudioManager::reset() called. %d sources were active.", activeCount));

    AudioManager::reset();
    avbridge_stopAll();
    for (auto &pa : m_sources) {
        pa.playerID = -1;
        pa.isPlaying = FALSE;
        if (pa.eventRTS) { delete pa.eventRTS; pa.eventRTS = nullptr; }
        pa.handle = 0;
    }

    DEBUG_AUDIO_MAC(("MacOSAudioManager::reset() completed. Buffer cache has %zu entries.", m_bufferCache.size()));
}

void MacOSAudioManager::update() {
    AudioManager::update();

    if (m_audioRequests.size() > 0) {
        DEBUG_AUDIO_MAC(("processRequestList running with %zu requests", m_audioRequests.size()));
    }

    setDeviceListenerPosition();
    processRequestList();

    for (auto &pa : m_sources) {
        if (!pa.isPlaying) continue;
        if (pa.playerID < 0) continue;

        if (!avbridge_isPlaying(pa.playerID)) {
            stopSourceAndFree(pa);
        }
    }
}

#pragma mark - Source Management

void MacOSAudioManager::stopSourceAndFree(PlayingAudio &pa) {
    if (pa.playerID >= 0) {
        avbridge_stop(pa.playerID);
    }
    pa.playerID = -1;
    pa.isPlaying = FALSE;
    pa.handle = 0;
    if (pa.eventRTS) {
        delete pa.eventRTS;
        pa.eventRTS = nullptr;
    }
}

PlayingAudio* MacOSAudioManager::findFreeSource(int priorityToDemand) {
    PlayingAudio *lowestPriorityPlaying = nullptr;
    int lowestPri = 999999;

    for (auto &pa : m_sources) {
        if (!pa.isPlaying) return &pa;

        if (pa.priority < lowestPri) {
            lowestPri = pa.priority;
            lowestPriorityPlaying = &pa;
        }
    }

    if (priorityToDemand > lowestPri && lowestPriorityPlaying) {
        stopSourceAndFree(*lowestPriorityPlaying);
        return lowestPriorityPlaying;
    }
    return nullptr;
}

#pragma mark - Buffer Loading

int MacOSAudioManager::loadAudioBuffer(const AsciiString& path, bool forceMono) {
    std::string originalPath = path.str();
    std::string pathStr = NativeFileSystem::get_safe_path(originalPath);

    std::string cacheKey = originalPath + (forceMono ? "_mono" : "_stereo");
    auto hit = m_bufferCache.find(cacheKey);
    if (hit != m_bufferCache.end()) {
        if (hit->second <= 0) return 0; // Previously failed to load/parse
        return hit->second;
    }

    uint8_t *fileData = nullptr;
    size_t fileSize = 0;

    bool loaded = loadWavFromDisk(pathStr, &fileData, &fileSize);
    if (!loaded) {
        loaded = loadWavFromBig(originalPath, &fileData, &fileSize);
    }
    if (!loaded || !fileData) {
        // Cache the failure so we don't spam disk/network
        m_bufferCache[cacheKey] = -1;
        return 0;
    }

    WavParseResult wav;
    if (!parseWavHeader(fileData, fileSize, &wav)) {
        // Log once, then cache the failure to avoid thread stutter
        DEBUG_AUDIO_MAC(("loadAudioBuffer: WAV parse failed for %s (not PCM WAV)", pathStr.c_str()));
        m_bufferCache[cacheKey] = -1;
        free(fileData);
        return 0;
    }

    uint16_t outChannels = wav.channels;
    const uint8_t *pcmData = wav.pcmStart;
    uint32_t pcmBytes = wav.pcmBytes;
    uint8_t *monoData = nullptr;

    if (forceMono && wav.channels == 2 && wav.bitsPerSample == 16) {
        uint32_t numSamples = wav.pcmBytes / 4;
        monoData = (uint8_t*)malloc(numSamples * 2);
        const int16_t *src = (const int16_t*)wav.pcmStart;
        int16_t *dst = (int16_t*)monoData;
        for (uint32_t i = 0; i < numSamples; i++) {
            dst[i] = (int16_t)(((int32_t)src[i*2] + (int32_t)src[i*2+1]) / 2);
        }
        pcmData = monoData;
        pcmBytes = numSamples * 2;
        outChannels = 1;
    }

    int bufID = avbridge_loadBuffer(pcmData, pcmBytes, wav.sampleRate, outChannels, wav.bitsPerSample);

    if (monoData) free(monoData);
    free(fileData);

    if (bufID <= 0) {
        m_bufferCache[cacheKey] = -1;
        return 0;
    }

    DEBUG_AUDIO_MAC(("loadAudioBuffer: OK %s -> bridge buf=%d ch=%d rate=%u",
        pathStr.c_str(), bufID, outChannels, wav.sampleRate));

    m_bufferCache[cacheKey] = bufID;
    return bufID;
}

std::string MacOSAudioManager::getPhysicalPathForStream(const std::string& vfsPath) {
    std::string safePath = NativeFileSystem::get_safe_path(vfsPath);
    if (NativeFileSystem::exists(safePath)) {
        return safePath;
    }

    if (!TheFileSystem || !TheFileSystem->doesFileExist(vfsPath.c_str())) {
        return "";
    }

    File *f = TheFileSystem->openFile(vfsPath.c_str(), File::READ);
    if (!f) return "";

    size_t fileSize = f->size();
    if (fileSize == 0 || fileSize > 50 * 1024 * 1024) { f->close(); return ""; }

    uint8_t *buf = (uint8_t*)malloc(fileSize);
    if (!buf) { f->close(); return ""; }

    Int bytesRead = f->read(buf, (Int)fileSize);
    f->close();

    if (bytesRead <= 0 || (size_t)bytesRead != fileSize) { free(buf); return ""; }

    AsciiString cachePath;
    cachePath.format(AUDIO_CACHE_DIR_FORMAT, TheGlobalData->getPath_UserData().str());
    cachePath.concat(vfsPath.c_str());

    std::string safeMacPath = NativeFileSystem::get_safe_path(cachePath.str());

    bool writeNeeded = true;
    if (NativeFileSystem::exists(safeMacPath)) {
        if (NativeFileSystem::file_size(safeMacPath) == fileSize) {
            writeNeeded = false;
        }
    }

    if (writeNeeded) {
        File *outFile = TheFileSystem->openFile(cachePath.str(), File::WRITE);
        if (outFile) {
            outFile->write(buf, (Int)fileSize);
            outFile->close();
        } else {
            free(buf);
            return "";
        }
    }

    free(buf);
    return safeMacPath;
}

#pragma mark - Request Processing

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
                    playAudioEvent(req->m_pendingEvent);
                    req->m_pendingEvent = nullptr;
                }
                break;
            }
            case AR_Stop: {
                for (auto &pa : m_sources) {
                    if (pa.isPlaying && pa.handle == req->m_handleToInteractOn) {
                        stopSourceAndFree(pa);
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

#pragma mark - Play Audio Event (3D Game Sounds)

void MacOSAudioManager::playAudioEvent(AudioEventRTS *eventToPlay) {
    if (!eventToPlay) return;

    AudioEventRTS *event = eventToPlay;
    event->generateFilename();
    AsciiString filename = event->getFilename();
    if (filename.isEmpty()) {
        DEBUG_AUDIO_MAC(("playAudioEvent: Filename is empty. Deleting event."));
        delete event;
        return;
    }

    int priority = 50;
    const AudioEventInfo *info = event->getAudioEventInfo();
    if (info) priority = info->m_priority;

    bool isStream = (info && (info->m_soundType == AT_Music || info->m_soundType == AT_Streaming));
    int bufID = 0;

    if (!isStream) {
        bool isPos = (event->getPosition() != nullptr && event->isPositionalAudio());
        bufID = loadAudioBuffer(filename, isPos);
        if (bufID <= 0) {
            delete event;
            return;
        }
    }

    PlayingAudio *pa = findFreeSource(priority);
    if (!pa) {
        DEBUG_AUDIO_MAC(("playAudioEvent: No free source for %s (pri %d). Deleting event.", filename.str(), priority));
        delete event;
        return;
    }

    float baseVol = 1.0f;
    if (info) {
        if (info->m_soundType == AT_Music) baseVol = getVolume(AudioAffect_Music);
        else if (info->m_soundType == AT_Streaming) baseVol = getVolume(AudioAffect_Speech);
        else baseVol = getVolume(AudioAffect_Sound);
    } else {
        baseVol = getVolume(AudioAffect_Sound);
    }

    float gain = event->getVolume() * baseVol;
    float pitch = event->getPitchShift() > 0 ? event->getPitchShift() : 1.0f;

    int playerID = -1;
    if (isStream) {
        std::string physicalPath = getPhysicalPathForStream(filename.str());
        if (!physicalPath.empty()) {
            playerID = avbridge_playStream(physicalPath.c_str(), gain, pitch, false);
        } else {
            DEBUG_AUDIO_MAC(("playAudioEvent: Failed to extract stream %s", filename.str()));
        }
    } else {
        const Coord3D *pos = event->getPosition();
        if (pos && event->isPositionalAudio()) {
            playerID = avbridge_play3D(bufID, gain, pitch,
                                       pos->x, pos->y, pos->z,
                                       500.0f, 50.0f);
        } else {
            playerID = avbridge_play(bufID, gain, pitch, false);
        }
    }

    if (playerID < 0) {
        DEBUG_AUDIO_MAC(("playAudioEvent: avbridge_play failed for %s", filename.str()));
        delete event;
        return;
    }

    DEBUG_AUDIO_MAC(("playAudioEvent: PLAYING %s! playerID=%d, Volume=%.2f", filename.str(), playerID, gain));

    pa->playerID = playerID;
    pa->isPlaying = TRUE;
    pa->eventRTS = event;
    pa->handle = event->getPlayingHandle();
    pa->priority = priority;
}

#pragma mark - Force Play (2D UI/Lobby Sounds)

void MacOSAudioManager::friend_forcePlayAudioEventRTS(const AudioEventRTS *eventToPlay) {
    if (!eventToPlay) return;

    AudioEventRTS eventCopy = *eventToPlay;
    eventCopy.generateFilename();
    AsciiString filename = eventCopy.getFilename();
    if (filename.isEmpty()) return;

    float baseVol = 1.0f;
    const AudioEventInfo *info = eventCopy.getAudioEventInfo();

    bool isStream = (info && (info->m_soundType == AT_Music || info->m_soundType == AT_Streaming));
    int bufID = 0;

    if (!isStream) {
        bufID = loadAudioBuffer(filename, false);
        if (bufID <= 0) return;
    }
    if (info) {
        if (info->m_soundType == AT_Music) baseVol = getVolume(AudioAffect_Music);
        else if (info->m_soundType == AT_Streaming) baseVol = getVolume(AudioAffect_Speech);
        else baseVol = getVolume(AudioAffect_Sound);
    } else {
        baseVol = getVolume(AudioAffect_Sound);
    }

    float gain = eventCopy.getVolume() * baseVol;
    float pitch = eventCopy.getPitchShift() > 0 ? eventCopy.getPitchShift() : 1.0f;

    if (isStream) {
        std::string pathStr = NativeFileSystem::get_safe_path(filename.str());
        avbridge_playStream(pathStr.c_str(), gain, pitch, false);
    } else {
        avbridge_play(bufID, gain, pitch, false);
    }
}

#pragma mark - Listener

void MacOSAudioManager::setDeviceListenerPosition() {
    avbridge_setListenerPosition(
        m_listenerPosition.x, m_listenerPosition.y, m_listenerPosition.z,
        m_listenerOrientation.x, m_listenerOrientation.y, m_listenerOrientation.z,
        0.0f, 0.0f, 1.0f
    );
}

#pragma mark - Query

Bool MacOSAudioManager::isCurrentlyPlaying(AudioHandle handle) {
    if (handle == 0) return FALSE;
    for (auto &pa : m_sources) {
        if (pa.isPlaying && pa.handle == handle) {
            return avbridge_isPlaying(pa.playerID) ? TRUE : FALSE;
        }
    }
    return FALSE;
}

#pragma mark - Global Controls

void MacOSAudioManager::stopAudio(AudioAffect which) {
    for (auto &pa : m_sources) {
        if (pa.isPlaying) stopSourceAndFree(pa);
    }
}

void MacOSAudioManager::pauseAudio(AudioAffect which) {
    avbridge_pauseAll();
}

void MacOSAudioManager::resumeAudio(AudioAffect which) {
    avbridge_resumeAll();
}

void MacOSAudioManager::pauseAmbient(Bool shouldPause) {}

void MacOSAudioManager::killAudioEventImmediately(AudioHandle audioEvent) {
    for (auto &pa : m_sources) {
        if (pa.isPlaying && pa.handle == audioEvent) {
            stopSourceAndFree(pa);
        }
    }
}

#pragma mark - Stubs

void MacOSAudioManager::nextMusicTrack() {
    AsciiString trackName = getMusicTrackName();
    TheAudio->removeAudioEvent(AHSV_StopTheMusic);
    trackName = TheAudio->nextTrackName(trackName);
    AudioEventRTS newTrack(trackName);
    TheAudio->addAudioEvent(&newTrack);
}
void MacOSAudioManager::prevMusicTrack() {
    AsciiString trackName = getMusicTrackName();
    TheAudio->removeAudioEvent(AHSV_StopTheMusic);
    trackName = TheAudio->prevTrackName(trackName);
    AudioEventRTS newTrack(trackName);
    TheAudio->addAudioEvent(&newTrack);
}
Bool MacOSAudioManager::isMusicPlaying() const {
    for (auto &pa : m_sources) {
        if (pa.isPlaying && pa.eventRTS && pa.eventRTS->getAudioEventInfo()) {
            if (pa.eventRTS->getAudioEventInfo()->m_soundType == AT_Music) return TRUE;
        }
    }
    return FALSE;
}
Bool MacOSAudioManager::isMusicAlreadyLoaded() const { return TRUE; }
Bool MacOSAudioManager::hasMusicTrackCompleted(const AsciiString &trackName, Int numberOfTimes) const { return FALSE; }
AsciiString MacOSAudioManager::getMusicTrackName() const {
    for (auto &pa : m_sources) {
        if (pa.isPlaying && pa.eventRTS && pa.eventRTS->getAudioEventInfo()) {
            if (pa.eventRTS->getAudioEventInfo()->m_soundType == AT_Music) {
                return pa.eventRTS->getEventName();
            }
        }
    }
    return AsciiString("");
}
void MacOSAudioManager::openDevice() {}
void MacOSAudioManager::closeDevice() {}
void *MacOSAudioManager::getDevice() { return nullptr; }
void MacOSAudioManager::notifyOfAudioCompletion(UnsignedInt audioCompleted, UnsignedInt flags) {}
UnsignedInt MacOSAudioManager::getProviderCount() const { return 1; }
AsciiString MacOSAudioManager::getProviderName(UnsignedInt providerNum) const { return "MacOS AVAudioEngine"; }
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
