#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DiscordRichPresence {
  const char *state;
  const char *details;
  int64_t startTimestamp;
  int64_t endTimestamp;
  const char *largeImageKey;
  const char *largeImageText;
  const char *smallImageKey;
  const char *smallImageText;
  const char *partyId;
  int partySize;
  int partyMax;
  const char *matchSecret;
  const char *joinSecret;
  const char *spectateSecret;
  int8_t instance;
} DiscordRichPresence;

typedef struct DiscordUser {
  const char *userId;
  const char *username;
  const char *discriminator;
  const char *avatar;
} DiscordUser;

typedef struct DiscordEventHandlers {
  void (*ready)(const DiscordUser *request);
  void (*disconnected)(int errorCode, const char *message);
  void (*errored)(int errorCode, const char *message);
  void (*joinGame)(const char *joinSecret);
  void (*spectateGame)(const char *spectateSecret);
  void (*joinRequest)(const DiscordUser *request);
} DiscordEventHandlers;

#ifdef __cplusplus
}
#endif
