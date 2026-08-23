#include "MacOSAudioManager.h"
#include "MacOSVideoAudioStream.h"
#include "AdpcmWavDecoder.h"
#include "Common/AudioAffect.h"
#include "Common/AudioEventInfo.h"
#include "Common/AudioEventRTS.h"
#include "Common/AudioHandleSpecialValues.h"
#include "Common/AudioRequest.h"
#include "Common/AudioSettings.h"
#include "Common/Debug.h"
#include "Common/GameMemory.h"
#include "Common/GameSounds.h"
#include "Common/FileSystem.h"
#include "Common/file.h"
#include "Common/System/NativeFileSystem.h"
#include "Common/GlobalData.h"
#include "Common/Registry.h"
#include "../Utils/MacDebug.h"
#include <unistd.h>

extern FileSystem *TheFileSystem;

static const char* AUDIO_CACHE_DIR_FORMAT = "%sAudioCache/";

static std::string applyLanguageFallback(const std::string& originalPath) {
    std::string lowerPath = originalPath;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
    AsciiString language = GetRegistryLanguage();
    language.toLower();

    std::string searchSpeech1 = "data\\audio\\speech\\" + std::string(language.str()) + "\\";
    std::string searchSpeech2 = "data/audio/speech/" + std::string(language.str()) + "/";
    std::string searchTracks1 = "data\\audio\\tracks\\" + std::string(language.str()) + "\\";
    std::string searchTracks2 = "data/audio/tracks/" + std::string(language.str()) + "/";

    if (lowerPath.find(searchSpeech1) == 0) {
        return "Data\\Audio\\Speech\\English\\" + originalPath.substr(searchSpeech1.length());
    } else if (lowerPath.find(searchSpeech2) == 0) {
        return "Data/Audio/Speech/English/" + originalPath.substr(searchSpeech2.length());
    } else if (lowerPath.find(searchTracks1) == 0) {
        return "Data\\Audio\\Tracks\\English\\" + originalPath.substr(searchTracks1.length());
    } else if (lowerPath.find(searchTracks2) == 0) {
        return "Data/Audio/Tracks/English/" + originalPath.substr(searchTracks2.length());
    }
    return originalPath;
}

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
    if (!TheFileSystem) return false;

    std::string pathToTry = originalPath;
    if (!TheFileSystem->doesFileExist(pathToTry.c_str())) {
        std::string fallbackPath = applyLanguageFallback(pathToTry);
        if (fallbackPath != pathToTry && TheFileSystem->doesFileExist(fallbackPath.c_str())) {
            pathToTry = fallbackPath;
        } else {
            return false;
        }
    }

    File *f = TheFileSystem->openFile(pathToTry.c_str(), File::READ);
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
    delete m_videoAudioStream;
    m_videoAudioStream = nullptr;
    avbridge_shutdown();
}

void MacOSAudioManager::init() {
    AudioManager::init();

    const AudioSettings *settings = getAudioSettings();
    m_num2DSamples = (settings && settings->m_sampleCount2D > 0) ? settings->m_sampleCount2D : 4;
    m_num3DSamples = (settings && settings->m_sampleCount3D > 0) ? settings->m_sampleCount3D : 25;
    m_numStreams = (settings && settings->m_streamCount > 0) ? settings->m_streamCount : 3;
    // Shared AVAudio node pool ≈ Miles 2D + 3D + stream handles.
    m_maxSources = m_num2DSamples + m_num3DSamples + m_numStreams;
    if (m_maxSources < 8) {
        m_maxSources = 8;
    }

    if (!avbridge_init(m_maxSources)) {
        printf("MACOS AUDIO: AVAudioEngine init FAILED!\n");
        fflush(stdout);
        return;
    }

    m_sources.clear();
    m_sources.resize((size_t)m_maxSources);

    for (int i = 0; i < m_maxSources; ++i) {
        if (i < m_num2DSamples) {
            m_sources[i].poolKind = SK_2D;
        } else if (i < m_num2DSamples + m_num3DSamples) {
            m_sources[i].poolKind = SK_3D;
        } else {
            m_sources[i].poolKind = SK_Stream;
        }
        m_sources[i].kind = m_sources[i].poolKind;
    }

    printf("MACOS AUDIO: AVAudioEngine Init Success. pool=%d (2D=%d 3D=%d streams=%d).\n",
           m_maxSources, m_num2DSamples, m_num3DSamples, m_numStreams);
    fflush(stdout);
}

void MacOSAudioManager::reset() {
    int activeCount = 0;
    for (auto &pa : m_sources) {
        if (pa.isPlaying) activeCount++;
    }
    DEBUG_AUDIO_MAC(("MacOSAudioManager::reset() called. %d sources were active.", activeCount));

    AudioManager::reset();
    for (auto &pa : m_sources) {
        if (pa.isPlaying) {
            stopSourceAndFree(pa);
        }
    }
    avbridge_stopAll();

    DEBUG_AUDIO_MAC(("MacOSAudioManager::reset() completed. Buffer cache has %zu entries.", m_bufferCache.size()));
}

void MacOSAudioManager::update() {
    AudioManager::update();

    if (m_audioRequests.size() > 0) {
        DEBUG_AUDIO_MAC(("processRequestList running with %zu requests", m_audioRequests.size()));
    }

    setDeviceListenerPosition();
    avbridge_serviceLoops();
    processRequestList();

    for (auto &pa : m_sources) {
        if (!pa.isPlaying) continue;
        if (pa.playerID < 0) continue;

        if (!avbridge_isPlaying(pa.playerID)) {
            advancePlayingAudio(pa);
        }
    }
}

#pragma mark - Source Management

void MacOSAudioManager::notifySampleStart(PlayingAudio &pa) {
    if (!m_sound || pa.counted) {
        return;
    }
    // Miles only counts sample handles, not music/speech streams.
    if (pa.eventRTS && pa.eventRTS->getAudioEventInfo()) {
        const AudioType st = pa.eventRTS->getAudioEventInfo()->m_soundType;
        if (st == AT_Music || st == AT_Streaming) {
            return;
        }
    }
    pa.counted = TRUE;
}

void MacOSAudioManager::notifySampleCompletion(PlayingAudio &pa) {
    if (!m_sound || !pa.counted) {
        return;
    }
    pa.counted = FALSE;
}

UnsignedInt MacOSAudioManager::getNumAvailable2DSamples() const {
    int used = 0;
    for (const PlayingAudio &pa : m_sources) {
        if (pa.counted && !pa.is3D) {
            ++used;
        }
    }
    return (UnsignedInt)((m_num2DSamples > used) ? (m_num2DSamples - used) : 0);
}

UnsignedInt MacOSAudioManager::getNumAvailable3DSamples() const {
    int used = 0;
    for (const PlayingAudio &pa : m_sources) {
        if (pa.counted && pa.is3D) {
            ++used;
        }
    }
    return (UnsignedInt)((m_num3DSamples > used) ? (m_num3DSamples - used) : 0);
}

void MacOSAudioManager::stopSourceAndFree(PlayingAudio &pa) {
    notifySampleCompletion(pa);
    if (pa.playerID >= 0) {
        avbridge_stop(pa.playerID);
    }
    pa.playerID = -1;
    pa.isPlaying = FALSE;
    pa.handle = 0;
    pa.priority = 0;
    pa.is3D = FALSE;
    if (pa.eventRTS) {
        delete pa.eventRTS;
        pa.eventRTS = nullptr;
    }
}

PlayingAudio* MacOSAudioManager::findSourceByHandle(AudioHandle handle) {
    if (handle == 0) {
        return nullptr;
    }
    for (auto &pa : m_sources) {
        if (pa.isPlaying && pa.handle == handle) {
            return &pa;
        }
    }
    return nullptr;
}

PlayingAudio* MacOSAudioManager::findFreeSource(int priorityToDemand, SourceKind kind) {
    PlayingAudio *lowestPriorityPlaying = nullptr;
    int lowestPri = 999999;

    for (auto &pa : m_sources) {
        if (pa.poolKind != kind) continue;
        if (!pa.isPlaying) return &pa;

        if (pa.priority < lowestPri) {
            lowestPri = pa.priority;
            lowestPriorityPlaying = &pa;
        }
    }

    // Miles caps 2D and 3D handles but never streams — "streams are basically
    // free" — so music and speech borrow any idle slot rather than being turned
    // away while a firefight owns the effect pools.
    if (kind == SK_Stream) {
        for (auto &pa : m_sources) {
            if (!pa.isPlaying) return &pa;
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
        if (hit->second <= 0) return -1; // Previously failed to load/parse
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
    uint8_t *decodedPcm = nullptr;
    uint32_t decodedBytes = 0;
    uint32_t decodedRate = 0;
    uint16_t decodedChannels = 0;

    uint16_t outChannels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 16;
    const uint8_t *pcmData = nullptr;
    uint32_t pcmBytes = 0;
    uint8_t *monoData = nullptr;

    if (parseWavHeader(fileData, fileSize, &wav)) {
        outChannels = wav.channels;
        sampleRate = wav.sampleRate;
        bitsPerSample = wav.bitsPerSample;
        pcmData = wav.pcmStart;
        pcmBytes = wav.pcmBytes;
    } else if (AdpcmWav_DecodeImaToPcm16(fileData, fileSize, &decodedPcm, &decodedBytes,
                                         &decodedRate, &decodedChannels)) {
        outChannels = decodedChannels;
        sampleRate = decodedRate;
        bitsPerSample = 16;
        pcmData = decodedPcm;
        pcmBytes = decodedBytes;
        DEBUG_AUDIO_MAC(("loadAudioBuffer: IMA ADPCM decoded %s -> pcm bytes=%u ch=%u rate=%u",
            pathStr.c_str(), decodedBytes, decodedChannels, decodedRate));
    } else {
        DEBUG_AUDIO_MAC(("loadAudioBuffer: WAV parse/decode failed for %s", pathStr.c_str()));
        m_bufferCache[cacheKey] = -1;
        free(fileData);
        return 0;
    }

    if (forceMono && outChannels == 2 && bitsPerSample == 16) {
        uint32_t numFrames = pcmBytes / 4;
        monoData = (uint8_t*)malloc(numFrames * 2);
        const int16_t *src = (const int16_t*)pcmData;
        int16_t *dst = (int16_t*)monoData;
        for (uint32_t i = 0; i < numFrames; i++) {
            dst[i] = (int16_t)(((int32_t)src[i*2] + (int32_t)src[i*2+1]) / 2);
        }
        pcmData = monoData;
        pcmBytes = numFrames * 2;
        outChannels = 1;
    }

    int bufID = avbridge_loadBuffer(pcmData, pcmBytes, sampleRate, outChannels, bitsPerSample);

    if (monoData) free(monoData);
    if (decodedPcm) free(decodedPcm);
    free(fileData);

    if (bufID <= 0) {
        m_bufferCache[cacheKey] = -1;
        return 0;
    }

    DEBUG_AUDIO_MAC(("loadAudioBuffer: OK %s -> bridge buf=%d ch=%d rate=%u",
        pathStr.c_str(), bufID, outChannels, sampleRate));

    m_bufferCache[cacheKey] = bufID;
    return bufID;
}

std::string MacOSAudioManager::getPhysicalPathForStream(const std::string& vfsPath) const {
    std::string safePath = NativeFileSystem::get_safe_path(vfsPath);
    if (NativeFileSystem::exists(safePath)) {
        return safePath;
    }

    if (!TheFileSystem) return "";

    std::string pathToTry = vfsPath;
    if (!TheFileSystem->doesFileExist(pathToTry.c_str())) {
        std::string fallbackPath = applyLanguageFallback(pathToTry);
        if (fallbackPath != pathToTry && TheFileSystem->doesFileExist(fallbackPath.c_str())) {
            pathToTry = fallbackPath;
        } else {
            return "";
        }
    }

    File *f = TheFileSystem->openFile(pathToTry.c_str(), File::READ);
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

        // Events carry a per-play delay (INI "Delay min max"), which also spaces
        // out every repeat of a looping sound. Hold the request until it runs out.
        if (req->m_usePendingEvent && req->m_pendingEvent &&
            req->m_pendingEvent->getDelay() >= MSEC_PER_LOGICFRAME_REAL) {
            req->m_pendingEvent->decrementDelay(MSEC_PER_LOGICFRAME_REAL);
            ++it;
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

// Only music repeats as one unbroken buffer. Looping sound effects list several
// variants (INI "Sounds = a b c d") and carry attack/decay parts, so each repeat
// has to go back through generateFilename to pick the next one — replaying a
// single buffer forever is what makes an engine loop sound mechanical.
Bool MacOSAudioManager::shouldLoopSeamlessly(const AudioEventRTS *event) const {
    const AudioEventInfo *info = event ? event->getAudioEventInfo() : nullptr;
    return (info && info->m_soundType == AT_Music) ? TRUE : FALSE;
}

static AsciiString filenameForCurrentPortion(AudioEventRTS *event) {
    switch (event->getNextPlayPortion()) {
        case PP_Attack: return event->getAttackFilename();
        case PP_Sound:  return event->getFilename();
        case PP_Decay:  return event->getDecayFilename();
        default:        return AsciiString::TheEmptyString;
    }
}

SourceKind MacOSAudioManager::sourceKindFor(AudioEventRTS *event) const {
    const AudioEventInfo *info = event ? event->getAudioEventInfo() : nullptr;
    if (!info) {
        return SK_2D;
    }
    if (info->m_soundType == AT_Music || info->m_soundType == AT_Streaming) {
        return SK_Stream;
    }
    return (event->getPosition() != nullptr && event->isPositionalAudio()) ? SK_3D : SK_2D;
}

int MacOSAudioManager::startPlayback(AudioEventRTS *eventToPlay, SourceKind kind) {
    const AudioEventInfo *info = eventToPlay->getAudioEventInfo();
    const AsciiString filename = filenameForCurrentPortion(eventToPlay);
    if (!info || filename.isEmpty()) {
        return -1;
    }

    const Bool loop = shouldLoopSeamlessly(eventToPlay);
    const float pitch = eventToPlay->getPitchShift() > 0 ? eventToPlay->getPitchShift() : 1.0f;

    float baseVol = getVolume(AudioAffect_Sound);
    if (info->m_soundType == AT_Music) {
        baseVol = getVolume(AudioAffect_Music);
    } else if (info->m_soundType == AT_Streaming) {
        baseVol = getVolume(AudioAffect_Speech);
    }
    const float gain = eventToPlay->getVolume() * baseVol;

    if (kind == SK_Stream) {
        const std::string physicalPath = getPhysicalPathForStream(filename.str());
        if (physicalPath.empty()) {
            DEBUG_AUDIO_MAC(("startPlayback: Failed to extract stream %s", filename.str()));
            return -1;
        }
        return avbridge_playStream(physicalPath.c_str(), gain, pitch, loop != FALSE);
    }

    const int bufID = loadAudioBuffer(filename, kind == SK_3D);
    if (bufID <= 0) {
        return -1;
    }

    if (kind == SK_3D) {
        const Coord3D *pos = eventToPlay->getPosition();
        return avbridge_play3D(bufID, gain, pitch,
                               pos->x, pos->y, pos->z,
                               500.0f, 50.0f, loop != FALSE);
    }
    return avbridge_play(bufID, gain, pitch, loop != FALSE);
}

void MacOSAudioManager::playAudioEvent(AudioEventRTS *eventToPlay) {
    if (!eventToPlay) return;

    AudioEventRTS *event = eventToPlay;
    const AudioEventInfo *info = event->getAudioEventInfo();
    const int priority = info ? info->m_priority : 50;

    if (!info || filenameForCurrentPortion(event).isEmpty()) {
        DEBUG_AUDIO_MAC(("playAudioEvent: no playable data for '%s'. Deleting event.",
            event->getEventName().str()));
        delete event;
        return;
    }

    const AudioHandle killHandle = event->getHandleToKill();
    if (killHandle != 0) {
        if (PlayingAudio *victim = findSourceByHandle(killHandle)) {
            stopSourceAndFree(*victim);
        }
    }

    const SourceKind kind = sourceKindFor(event);

    PlayingAudio *pa = findFreeSource(priority, kind);
    if (!pa) {
        DEBUG_AUDIO_MAC(("playAudioEvent: No free source for %s (pri %d). Deleting event.",
            event->getFilename().str(), priority));
        delete event;
        return;
    }

    const int playerID = startPlayback(event, kind);
    if (playerID < 0) {
        DEBUG_AUDIO_MAC(("playAudioEvent: playback failed for %s", event->getFilename().str()));
        delete event;
        return;
    }

    DEBUG_AUDIO_MAC(("playAudioEvent: PLAYING %s! event=%s playerID=%d looping=%d",
        event->getFilename().str(), event->getEventName().str(), playerID,
        shouldLoopSeamlessly(event) ? 1 : 0));

    pa->playerID = playerID;
    pa->isPlaying = TRUE;
    pa->eventRTS = event;
    pa->handle = event->getPlayingHandle();
    pa->priority = priority;
    pa->kind = kind;
    pa->is3D = (kind == SK_3D) ? TRUE : FALSE;
    pa->counted = FALSE;
    notifySampleStart(*pa);
}

Bool MacOSAudioManager::restartCurrentPortion(PlayingAudio &pa) {
    const int playerID = startPlayback(pa.eventRTS, pa.kind);
    if (playerID < 0) {
        return FALSE;
    }

    pa.playerID = playerID;
    return TRUE;
}

Bool MacOSAudioManager::startNextLoop(PlayingAudio &pa) {
    AudioEventRTS *event = pa.eventRTS;
    event->generateFilename();

    // generateFilename rolls a fresh delay for this repeat. A spaced-out repeat
    // goes back through the request queue, which counts the delay down, instead
    // of restarting the sample right away.
    if (event->getDelay() > MSEC_PER_LOGICFRAME_REAL) {
        AudioRequest *req = allocateAudioRequest(TRUE);
        req->m_pendingEvent = event;
        appendAudioRequest(req);

        pa.eventRTS = nullptr;
        stopSourceAndFree(pa);
        return TRUE;
    }

    return restartCurrentPortion(pa);
}

void MacOSAudioManager::advancePlayingAudio(PlayingAudio &pa) {
    AudioEventRTS *event = pa.eventRTS;
    const AudioEventInfo *info = event ? event->getAudioEventInfo() : nullptr;
    if (!info) {
        stopSourceAndFree(pa);
        return;
    }


    if (BitIsSet(info->m_control, AC_LOOP)) {
        if (event->getNextPlayPortion() == PP_Attack) {
            event->setNextPlayPortion(PP_Sound);
        }
        if (event->getNextPlayPortion() == PP_Sound) {
            event->decreaseLoopCount();
            if (event->hasMoreLoops() && startNextLoop(pa)) {
                DEBUG_AUDIO_MAC(("LOOP RESTART: event=%s file=%s playerID=%d",
                    event->getEventName().str(), event->getFilename().str(), pa.playerID));
                return;
            }
        }
    }

    event->advanceNextPlayPortion();
    if (event->getNextPlayPortion() != PP_Done && restartCurrentPortion(pa)) {
        DEBUG_AUDIO_MAC(("advancePlayingAudio: next portion of %s", event->getEventName().str()));
        return;
    }

    stopSourceAndFree(pa);
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

Bool MacOSAudioManager::isAffectedBy(const PlayingAudio &pa, AudioAffect which) const {
    const AudioEventInfo *info = pa.eventRTS ? pa.eventRTS->getAudioEventInfo() : nullptr;
    if (!info) {
        return BitIsSet(which, AudioAffect_Sound);
    }

    if (info->m_soundType == AT_Music) {
        return BitIsSet(which, AudioAffect_Music);
    }
    if (info->m_soundType == AT_Streaming) {
        return BitIsSet(which, AudioAffect_Speech);
    }
    return BitIsSet(which, pa.is3D ? AudioAffect_Sound3D : AudioAffect_Sound);
}

void MacOSAudioManager::stopAudio(AudioAffect which) {
    for (auto &pa : m_sources) {
        if (pa.isPlaying && isAffectedBy(pa, which)) {
            stopSourceAndFree(pa);
        }
    }
}

void MacOSAudioManager::pauseAudio(AudioAffect which) {
    for (auto &pa : m_sources) {
        if (pa.isPlaying && pa.playerID >= 0 && isAffectedBy(pa, which)) {
            avbridge_pause(pa.playerID);
        }
    }
}

void MacOSAudioManager::resumeAudio(AudioAffect which) {
    for (auto &pa : m_sources) {
        if (pa.isPlaying && pa.playerID >= 0 && isAffectedBy(pa, which)) {
            avbridge_resume(pa.playerID);
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

#pragma mark - Stubs

AsciiString MacOSAudioManager::nextMusicTrack() {
    AsciiString trackName = getMusicTrackName();
    TheAudio->removeAudioEvent(AHSV_StopTheMusic);
    trackName = TheAudio->nextTrackName(trackName);
    AudioEventRTS newTrack(trackName);
    TheAudio->addAudioEvent(&newTrack);
    return trackName;
}
AsciiString MacOSAudioManager::prevMusicTrack() {
    AsciiString trackName = getMusicTrackName();
    TheAudio->removeAudioEvent(AHSV_StopTheMusic);
    trackName = TheAudio->prevTrackName(trackName);
    AudioEventRTS newTrack(trackName);
    TheAudio->addAudioEvent(&newTrack);
    return trackName;
}
Bool MacOSAudioManager::isMusicPlaying() const {
    for (auto &pa : m_sources) {
        if (pa.isPlaying && pa.eventRTS && pa.eventRTS->getAudioEventInfo()) {
            if (pa.eventRTS->getAudioEventInfo()->m_soundType == AT_Music) return TRUE;
        }
    }
    return FALSE;
}
Bool MacOSAudioManager::hasMusicTrackCompleted(const AsciiString &trackName, Int numberOfTimes) const {
    for (const auto &pa : m_sources) {
        if (!pa.isPlaying || pa.playerID < 0 || !pa.eventRTS || !pa.eventRTS->getAudioEventInfo()) {
            continue;
        }
        if (pa.eventRTS->getAudioEventInfo()->m_soundType != AT_Music) {
            continue;
        }
        if (pa.eventRTS->getEventName() != trackName) {
            continue;
        }
        if (avbridge_getLoopCount(pa.playerID) >= numberOfTimes) {
            return TRUE;
        }
    }
    return FALSE;
}
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
UnsignedInt MacOSAudioManager::getNum2DSamples() const { return (UnsignedInt)m_num2DSamples; }
UnsignedInt MacOSAudioManager::getNum3DSamples() const { return (UnsignedInt)m_num3DSamples; }
UnsignedInt MacOSAudioManager::getNumStreams() const { return (UnsignedInt)m_numStreams; }

Bool MacOSAudioManager::doesViolateLimit(AudioEventRTS *event) const {
    if (!event || !event->getAudioEventInfo()) {
        return FALSE;
    }

    const Int limit = event->getAudioEventInfo()->m_limit;
    if (limit == 0) {
        return FALSE;
    }

    Int totalCount = 0;
    Int totalRequestCount = 0;
    const Bool wantPositional = event->isPositionalAudio();

    for (const auto &pa : m_sources) {
        if (!pa.isPlaying || !pa.eventRTS) {
            continue;
        }
        if (pa.eventRTS->getEventName() != event->getEventName()) {
            continue;
        }
        const Bool playingPos = pa.is3D ? TRUE : FALSE;
        if (playingPos != wantPositional) {
            continue;
        }
        if (totalCount == 0) {
            event->setHandleToKill(pa.eventRTS->getPlayingHandle());
        }
        ++totalCount;
    }

    for (AudioRequest *req : m_audioRequests) {
        if (!req || !req->m_usePendingEvent || !req->m_pendingEvent) {
            continue;
        }
        if (req->m_pendingEvent->getEventName() == event->getEventName()) {
            ++totalRequestCount;
            ++totalCount;
        }
    }

    if (event->getAudioEventInfo()->m_control & AC_INTERRUPT) {
        if (totalRequestCount < limit) {
            const Int totalPlayingCount = totalCount - totalRequestCount;
            if (totalRequestCount + totalPlayingCount < limit) {
                event->setHandleToKill(0);
                return FALSE;
            }
            return FALSE;
        }
    }

    if (totalCount < limit) {
        event->setHandleToKill(0);
        return FALSE;
    }

    return TRUE;
}

Bool MacOSAudioManager::isPlayingLowerPriority(AudioEventRTS *event) const {
    if (!event || !event->getAudioEventInfo()) {
        return FALSE;
    }

    const AudioPriority priority = event->getAudioEventInfo()->m_priority;
    if (priority == AP_LOWEST) {
        return FALSE;
    }

    const Bool wantPositional = event->isPositionalAudio();
    for (const auto &pa : m_sources) {
        if (!pa.isPlaying || !pa.eventRTS || !pa.eventRTS->getAudioEventInfo()) {
            continue;
        }
        const Bool playingPos = pa.is3D ? TRUE : FALSE;
        if (playingPos != wantPositional) {
            continue;
        }
        if (pa.eventRTS->getAudioEventInfo()->m_priority < priority) {
            return TRUE;
        }
    }
    return FALSE;
}

Bool MacOSAudioManager::isPlayingAlready(AudioEventRTS *event) const {
    if (!event) {
        return FALSE;
    }
    const Bool wantPositional = event->isPositionalAudio();
    for (const auto &pa : m_sources) {
        if (!pa.isPlaying || !pa.eventRTS) {
            continue;
        }
        if (pa.eventRTS->getEventName() != event->getEventName()) {
            continue;
        }
        const Bool playingPos = pa.is3D ? TRUE : FALSE;
        if (playingPos == wantPositional) {
            return TRUE;
        }
    }
    return FALSE;
}

Bool MacOSAudioManager::isObjectPlayingVoice(UnsignedInt objID) const {
    if (objID == 0) {
        return FALSE;
    }
    for (const auto &pa : m_sources) {
        if (!pa.isPlaying || !pa.eventRTS || !pa.eventRTS->getAudioEventInfo()) {
            continue;
        }
        if (pa.eventRTS->getObjectID() != objID) {
            continue;
        }
        if (pa.eventRTS->getAudioEventInfo()->m_type & ST_VOICE) {
            return TRUE;
        }
    }
    return FALSE;
}

void MacOSAudioManager::adjustVolumeOfPlayingAudio(AsciiString eventName, Real newVolume) {}
void MacOSAudioManager::removePlayingAudio(AsciiString eventName) {}
void MacOSAudioManager::removeAllDisabledAudio() {}
Bool MacOSAudioManager::has3DSensitiveStreamsPlaying() const { return FALSE; }
void *MacOSAudioManager::getHandleForBink() {
    if (!m_videoAudioStream) {
        m_videoAudioStream = new MacOSVideoAudioStream();
    }
    return m_videoAudioStream;
}

void MacOSAudioManager::releaseHandleForBink() {
    if (m_videoAudioStream) {
        m_videoAudioStream->reset();
    }
}
void MacOSAudioManager::setPreferredProvider(AsciiString providerNdx) {}
void MacOSAudioManager::setPreferredSpeaker(AsciiString speakerType) {}
Real MacOSAudioManager::measureFileLengthMS(const std::string &path) const {
    uint8_t *fileData = nullptr;
    size_t fileSize = 0;

    bool loaded = loadWavFromDisk(NativeFileSystem::get_safe_path(path), &fileData, &fileSize);
    if (!loaded) {
        loaded = loadWavFromBig(path, &fileData, &fileSize);
    }

    if (loaded && fileData) {
        Real lengthMS = 0.0f;
        WavParseResult wav;
        float adpcmMS = 0.0f;

        if (parseWavHeader(fileData, fileSize, &wav)) {
            const uint32_t bytesPerFrame = wav.channels * (wav.bitsPerSample / 8);
            if (bytesPerFrame > 0 && wav.sampleRate > 0) {
                lengthMS = (Real)((double)wav.pcmBytes * 1000.0 / (double)(bytesPerFrame * wav.sampleRate));
            }
        } else if (AdpcmWav_GetDurationMS(fileData, fileSize, &adpcmMS)) {
            lengthMS = (Real)adpcmMS;
        }

        free(fileData);
        if (lengthMS > 0.0f) {
            return lengthMS;
        }
    }

    const std::string physicalPath = getPhysicalPathForStream(path);
    if (physicalPath.empty()) {
        return 0.0f;
    }
    return (Real)avbridge_getFileDurationMS(physicalPath.c_str());
}

Real MacOSAudioManager::getFileLengthMS(AsciiString strToLoad) const {
    if (strToLoad.isEmpty()) {
        return 0.0f;
    }

    const std::string key = strToLoad.str();
    auto hit = m_fileLengthCache.find(key);
    if (hit != m_fileLengthCache.end()) {
        return hit->second;
    }

    const Real lengthMS = measureFileLengthMS(key);
    m_fileLengthCache[key] = lengthMS;

    DEBUG_AUDIO_MAC(("getFileLengthMS: %s -> %.1f ms", key.c_str(), lengthMS));
    return lengthMS;
}
void MacOSAudioManager::closeAnySamplesUsingFile(const void *fileToClose) {}

#if defined(RTS_DEBUG)
void MacOSAudioManager::audioDebugDisplay(DebugDisplayInterface *dd, void *userData, FILE *fp) {}
#endif
