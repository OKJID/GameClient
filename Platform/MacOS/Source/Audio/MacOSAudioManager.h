#pragma once

#include "Common/GameAudio.h"
#include "AVAudioBridge.h"
#include <vector>
#include <string>
#include <unordered_map>

struct PlayingAudio {
    int playerID = -1;
    Bool isPlaying = FALSE;
    AudioEventRTS *eventRTS = nullptr;
    AudioHandle handle = 0;
    int priority = 0;
    Bool is3D = FALSE;
    Bool counted = FALSE;
};

class MacOSVideoAudioStream;

class MacOSAudioManager : public AudioManager {
public:
  MacOSAudioManager();
  virtual ~MacOSAudioManager();

  virtual void init() override;
  virtual void reset() override;
  virtual void update() override;

  virtual void stopAudio(AudioAffect which) override;
  virtual void pauseAudio(AudioAffect which) override;
  virtual void resumeAudio(AudioAffect which) override;
  virtual void pauseAmbient(Bool shouldPause) override;
  virtual void killAudioEventImmediately(AudioHandle audioEvent) override;

  virtual void nextMusicTrack() override;
  virtual void prevMusicTrack() override;
  virtual Bool isMusicPlaying() const override;
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
  void processRequestList() override;
  void playAudioEvent(AudioEventRTS *eventToPlay);

  int startPlayback(AudioEventRTS *eventToPlay, Bool &isPositional);
  Bool shouldLoopSeamlessly(const AudioEventRTS *event) const;
  Bool restartCurrentPortion(PlayingAudio &pa);
  Bool startNextLoop(PlayingAudio &pa);
  void advancePlayingAudio(PlayingAudio &pa);

  Bool isAffectedBy(const PlayingAudio &pa, AudioAffect which) const;

  Real measureFileLengthMS(const std::string &path) const;

  int loadAudioBuffer(const AsciiString& path, bool forceMono = false);
  std::string getPhysicalPathForStream(const std::string& vfsPath) const;
  void stopSourceAndFree(PlayingAudio &pa);
  PlayingAudio* findFreeSource(int priorityToDemand);
  PlayingAudio* findSourceByHandle(AudioHandle handle);
  void notifySampleStart(PlayingAudio &pa);
  void notifySampleCompletion(PlayingAudio &pa);

private:
  int m_maxSources = 0;
  int m_num2DSamples = 0;
  int m_num3DSamples = 0;
  int m_numStreams = 0;
  std::vector<PlayingAudio> m_sources;
  std::unordered_map<std::string, int> m_bufferCache;
  mutable std::unordered_map<std::string, Real> m_fileLengthCache;
  MacOSVideoAudioStream* m_videoAudioStream = nullptr;
};

