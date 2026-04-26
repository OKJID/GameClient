#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

bool    avbridge_init(int maxNodes);
void    avbridge_shutdown(void);

int     avbridge_loadBuffer(const uint8_t *pcmData, uint32_t pcmBytes,
                            uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample);
void    avbridge_unloadBuffer(int bufferID);

int     avbridge_play(int bufferID, float gain, float pitch, bool loop);
int     avbridge_play3D(int bufferID, float gain, float pitch,
                        float x, float y, float z, float maxDist, float refDist);
int     avbridge_playStream(const char* filepath, float gain, float pitch, bool loop);
void    avbridge_stop(int playerID);
void    avbridge_stopAll(void);
bool    avbridge_isPlaying(int playerID);

void    avbridge_setListenerPosition(float x, float y, float z,
                                     float fwdX, float fwdY, float fwdZ,
                                     float upX, float upY, float upZ);

void    avbridge_setVolume(int playerID, float gain);
void    avbridge_setPitch(int playerID, float pitch);
void    avbridge_pause(int playerID);
void    avbridge_resume(int playerID);
void    avbridge_pauseAll(void);
void    avbridge_resumeAll(void);

void    avbridge_cleanup(void);

#ifdef __cplusplus
}
#endif
