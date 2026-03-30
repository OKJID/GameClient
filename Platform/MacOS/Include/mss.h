#pragma once
#ifdef __APPLE__

typedef void* HSAMPLE;
typedef void* HDIGDRIVER;
typedef void* H3DPOBJECT;
typedef void* H3DSAMPLE;
typedef void* HPROVIDER;
typedef void* HTIMER;
typedef void* HSTREAM;
typedef void* HMDIDRIVER;
typedef void* HSEQUENCE;

typedef int S32;
typedef unsigned int U32;
typedef float F32;

typedef struct {
    unsigned short wFormatTag;
    unsigned short nChannels;
    unsigned long nSamplesPerSec;
    unsigned long nAvgBytesPerSec;
    unsigned short nBlockAlign;
    unsigned short wBitsPerSample;
    unsigned short cbSize;
} WAVEFORMATEX, *LPWAVEFORMATEX;

typedef WAVEFORMATEX WAVEFORMAT, *LPWAVEFORMAT;
typedef WAVEFORMATEX* PWAVEFORMATEX;

#define WAVE_FORMAT_PCM 1

typedef void (*AILTIMERCB)(unsigned int);
typedef void (*AILSAMPLECB)(HSAMPLE);

#define AIL_QUICK_DONT_USE_WAVEOUT 8
#define AILCALLBACK

inline int AIL_startup() { return 0; }
inline void AIL_shutdown() {}
inline void AIL_set_preference(U32, S32) {}
inline HTIMER AIL_register_timer(AILTIMERCB) { return nullptr; }
inline void AIL_set_timer_period(HTIMER, U32) {}
inline void AIL_start_timer(HTIMER) {}
inline void AIL_stop_timer(HTIMER) {}
inline void AIL_release_timer_handle(HTIMER) {}

#endif
