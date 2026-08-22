#pragma once

#include "GameNetwork/GeneralsOnline/Vendor/DiscordRPC/discord_rpc.h"

#include <chrono>
#include <string>

class NGMP_OnlineServices_LobbyInterface;

// TheSuperHackers @feature ultimate2000 19/07/2026 Adds Discord Rich Presence
// for Generals Online lobbies and matches.
class GeneralsOnlineDiscordRPC {
public:
  GeneralsOnlineDiscordRPC() = default;
  ~GeneralsOnlineDiscordRPC();

  bool Initialize();
  void Shutdown();
  void Tick(const NGMP_OnlineServices_LobbyInterface *lobbyInterface);

private:
  using InitializeFunction = void (*)(const char *, DiscordEventHandlers *, int,
                                      const char *);
  using ShutdownFunction = void (*)();
  using RunCallbacksFunction = void (*)();
  using UpdatePresenceFunction = void (*)(const DiscordRichPresence *);
  using ClearPresenceFunction = void (*)();

  struct PresenceData {
    std::string state;
    std::string details;
    std::string largeImageKey;
    std::string largeImageText;
    std::string smallImageKey;
    std::string smallImageText;
    std::string partyId;
    int partySize = 0;
    int partyMax = 0;

    std::string Fingerprint() const;
  };

  PresenceData
  BuildPresence(const NGMP_OnlineServices_LobbyInterface *lobbyInterface) const;
  void UpdatePresence(const PresenceData &data);
  void ResetFunctions();

  void *m_module = nullptr;
  InitializeFunction m_initialize = nullptr;
  ShutdownFunction m_shutdown = nullptr;
  RunCallbacksFunction m_runCallbacks = nullptr;
  UpdatePresenceFunction m_updatePresence = nullptr;
  ClearPresenceFunction m_clearPresence = nullptr;
  bool m_initialized = false;
  std::string m_lastPresence;
  std::chrono::steady_clock::time_point m_nextPresenceUpdate;
};
