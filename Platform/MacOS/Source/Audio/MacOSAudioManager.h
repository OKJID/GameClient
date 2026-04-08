#pragma once

#include "Common/GameAudio.h"
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <set>

#pragma pack(push, 1)
// Standard RIFF WAV header struct for simple loading
struct WAVHeader {
    char riff[4];
    uint32_t fileSize;
    char wave[4];
    char fmt[4];
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char data[4];
    uint32_t dataSize;
};
#pragma pack(pop)

// Represents a loaded audio file in the OpenAL buffer pool
struct AudioBufferCacheEntry {
    AsciiString path;
    ALuint bufferID;
    UnsignedInt refCount; // Though we just load once normally
    bool valid;
};

// Represents a hardware/software channel (Voice) in the OpenAL engine
struct PlayingAudio {
    ALuint sourceID;
    Bool isPlaying;
    AudioEventRTS *eventRTS;
    AudioHandle handle;
    int priority; 
};

class MacOSAudioManager : public AudioManager {
public:
  MacOSAudioManager();
  virtual ~MacOSAudioManager();

  // SubsystemInterface overrides
  virtual void init() override;
  virtual void reset() override;
  virtual void update() override;

  // AudioManager overrides
  virtual void stopAudio(AudioAffect which) override;
  virtual void pauseAudio(AudioAffect which) override;
  virtual void resumeAudio(AudioAffect which) override;
  virtual void pauseAmbient(Bool shouldPause) override;
  virtual void killAudioEventImmediately(AudioHandle audioEvent) override;

  virtual void nextMusicTrack() override;
  virtual void prevMusicTrack() override;
  virtual Bool isMusicPlaying() const override;
  virtual Bool isMusicAlreadyLoaded() const override;
  virtual Bool hasMusicTrackCompleted(const AsciiString &trackName, Int numberOfTimes) const override;
  virtual AsciiString getMusicTrackName() const override;

  virtual void openDevice() override;
  virtual void closeDevice() override;
  virtual void *getDevice() override;

  virtual void notifyOfAudioCompletion(UnsignedInt audioCompleted, UnsignedInt flags) override;

  virtual UnsignedInt getProviderCount() const override;
  virtual AsciiString getProviderName(UnsignedInt providerNum) const override;
  virtual UnsignedInt getProviderIndex(AsciiString providerName) const override;
  virtual void selectProvider(UnsignedInt providerNdx) override;
  virtual void unselectProvider() override;
  virtual UnsignedInt getSelectedProvider() const override;

  virtual void setSpeakerType(UnsignedInt speakerType) override;
  virtual UnsignedInt getSpeakerType() override;

  virtual UnsignedInt getNum2DSamples() const override;
  virtual UnsignedInt getNum3DSamples() const override;
  virtual UnsignedInt getNumStreams() const override;

  virtual Bool doesViolateLimit(AudioEventRTS *event) const override;
  virtual Bool isPlayingLowerPriority(AudioEventRTS *event) const override;
  virtual Bool isPlayingAlready(AudioEventRTS *event) const override;
  virtual Bool isObjectPlayingVoice(UnsignedInt objID) const override;

  virtual void adjustVolumeOfPlayingAudio(AsciiString eventName, Real newVolume) override;
  virtual void removePlayingAudio(AsciiString eventName) override;
  virtual void removeAllDisabledAudio() override;

  virtual Bool has3DSensitiveStreamsPlaying() const override;
  virtual void *getHandleForBink() override;
  virtual void releaseHandleForBink() override;

  virtual Bool isCurrentlyPlaying(AudioHandle handle) override;
  virtual void friend_forcePlayAudioEventRTS(const AudioEventRTS *eventToPlay) override;

  virtual void setPreferredProvider(AsciiString providerNdx) override;
  virtual void setPreferredSpeaker(AsciiString speakerType) override;

  virtual Real getFileLengthMS(AsciiString strToLoad) const override;
  virtual void closeAnySamplesUsingFile(const void *fileToClose) override;

  virtual void setDeviceListenerPosition() override;

#if defined(RTS_DEBUG)
  virtual void audioDebugDisplay(DebugDisplayInterface *dd, void *userData, FILE *fp = nullptr) override;
#endif

protected:
  void processRequestList();

  // Internal Core Methods
  ALuint loadAudioFileIntoBuffer(const AsciiString& path, bool forceMono = false);
  void stopSourceAndFree(PlayingAudio &pa);
  PlayingAudio* findFreeSource(int priorityToDemand);

private:
  ALCdevice *m_device;
  ALCcontext *m_context;

  static const int MAX_SOURCES = 64;
  std::vector<PlayingAudio> m_sources;
  std::unordered_map<std::string, AudioBufferCacheEntry> m_bufferCache;
};
