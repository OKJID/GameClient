#include <cstdio>
#include <cstdlib>

// #define DEBUG_AUDIO_MAC_FLAG

#ifdef DEBUG_AUDIO_MAC_FLAG
#define DEBUG_AUDIO_MAC(m)                                                     \
  do {                                                                         \
    printf("[DEBUG_AUDIO_MAC] ");                                              \
    printf m;                                                                  \
    printf("\n");                                                              \
    fflush(stdout);                                                            \
  } while (0)
#else
#define DEBUG_AUDIO_MAC(m) ((void)0)
#endif