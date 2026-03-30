#include "MacOSAudioManager.h"
#include "Common/AudioAffect.h"
#include "Common/AudioEventInfo.h"
#include "Common/AudioEventRTS.h"
#include "Common/AudioRequest.h"
#include "Common/Debug.h"
#include "Common/GameMemory.h"

MacOSAudioManager::MacOSAudioManager() {}
MacOSAudioManager::~MacOSAudioManager() {}

void MacOSAudioManager::init() {
  AudioManager::init();
}

void MacOSAudioManager::reset() {
  AudioManager::reset();
}

void MacOSAudioManager::update() {
  AudioManager::update();
  processRequestList();
}

void MacOSAudioManager::processRequestList() {
  for (auto it = m_audioRequests.begin(); it != m_audioRequests.end();) {
    AudioRequest *req = *it;
    if (req) {
      if (req->m_request == AR_Play && req->m_usePendingEvent && req->m_pendingEvent) {
        delete req->m_pendingEvent; // Cleanup to prevent memory leak
        req->m_pendingEvent = nullptr;
      }
      deleteInstance(req);
    }
    it = m_audioRequests.erase(it);
  }
}

void MacOSAudioManager::stopAudio(AudioAffect which) {}
void MacOSAudioManager::pauseAudio(AudioAffect which) {}
void MacOSAudioManager::resumeAudio(AudioAffect which) {}
void MacOSAudioManager::pauseAmbient(Bool shouldPause) {}
void MacOSAudioManager::killAudioEventImmediately(AudioHandle audioEvent) {}
void MacOSAudioManager::nextMusicTrack() {}
void MacOSAudioManager::prevMusicTrack() {}
Bool MacOSAudioManager::isMusicPlaying() const { return FALSE; }
Bool MacOSAudioManager::isMusicAlreadyLoaded() const { return TRUE; }  // Prevents quit on empty missing music folder
Bool MacOSAudioManager::hasMusicTrackCompleted(const AsciiString &trackName, Int numberOfTimes) const { return FALSE; }
AsciiString MacOSAudioManager::getMusicTrackName() const { return ""; }
void MacOSAudioManager::openDevice() {}
void MacOSAudioManager::closeDevice() {}
void *MacOSAudioManager::getDevice() { return nullptr; }
void MacOSAudioManager::notifyOfAudioCompletion(UnsignedInt audioCompleted, UnsignedInt flags) {}
UnsignedInt MacOSAudioManager::getProviderCount() const { return 1; }
AsciiString MacOSAudioManager::getProviderName(UnsignedInt providerNum) const { return "MacOS Stub Audio"; }
UnsignedInt MacOSAudioManager::getProviderIndex(AsciiString providerName) const { return 0; }
void MacOSAudioManager::selectProvider(UnsignedInt providerNdx) {}
void MacOSAudioManager::unselectProvider() {}
UnsignedInt MacOSAudioManager::getSelectedProvider() const { return 0; }
void MacOSAudioManager::setSpeakerType(UnsignedInt speakerType) {}
UnsignedInt MacOSAudioManager::getSpeakerType() { return 0; }
UnsignedInt MacOSAudioManager::getNum2DSamples() const { return 64; }
UnsignedInt MacOSAudioManager::getNum3DSamples() const { return 64; }
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
Bool MacOSAudioManager::isCurrentlyPlaying(AudioHandle handle) { return FALSE; } // Very important for EVA queue
void MacOSAudioManager::friend_forcePlayAudioEventRTS(const AudioEventRTS *eventToPlay) {}
void MacOSAudioManager::setPreferredProvider(AsciiString providerNdx) {}
void MacOSAudioManager::setPreferredSpeaker(AsciiString speakerType) {}
Real MacOSAudioManager::getFileLengthMS(AsciiString strToLoad) const { return 0.0f; }
void MacOSAudioManager::closeAnySamplesUsingFile(const void *fileToClose) {}
void MacOSAudioManager::setDeviceListenerPosition() {}

#if defined(RTS_DEBUG)
void MacOSAudioManager::audioDebugDisplay(DebugDisplayInterface *dd, void *userData, FILE *fp) {}
#endif
