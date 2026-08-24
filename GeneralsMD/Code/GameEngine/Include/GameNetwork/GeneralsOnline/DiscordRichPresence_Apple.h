#pragma once

#ifdef __APPLE__

#include "GameNetwork/GeneralsOnline/Vendor/DiscordRPC/discord_rpc.h"

namespace DiscordIpc {

void Initialize(const char *applicationId, DiscordEventHandlers *handlers,
                int autoRegister, const char *optionalSteamId);
void Shutdown();
void RunCallbacks();
void UpdatePresence(const DiscordRichPresence *presence);
void ClearPresence();

} // namespace DiscordIpc

#endif
