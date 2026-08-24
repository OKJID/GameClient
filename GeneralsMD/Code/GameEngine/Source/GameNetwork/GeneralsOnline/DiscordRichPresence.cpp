#include "GameNetwork/GeneralsOnline/DiscordRichPresence.h"

#include "Common/PlayerTemplate.h"
#include "GameClient/MapUtil.h"
#include "GameNetwork/GameInfo.h"
#include "GameNetwork/GeneralsOnline/NGMPGame.h"
#include "GameNetwork/GeneralsOnline/OnlineServices_LobbyInterface.h"

#ifdef __APPLE__
#include "GameNetwork/GeneralsOnline/DiscordRichPresence_Apple.h"
#endif

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>

namespace {
constexpr auto PRESENCE_UPDATE_INTERVAL = std::chrono::seconds(2);
constexpr const char *DISCORD_APPLICATION_ID = "1354979004507226294";
constexpr const char *LOBBY_DETAILS = "In Lobby / In Room";
constexpr const char *LOBBY_STATE = "Waiting match";
constexpr const char *DEFAULT_IMAGE_KEY = "generals_online";
constexpr const char *DEFAULT_IMAGE_TEXT = "Generals Online";

struct GeneralAsset {
  const char *side;
  const char *generalImage;
  const char *factionImage;
};

constexpr GeneralAsset GENERAL_ASSETS[] = {
    {"America", "usa_general", "usa"},
    {"AmericaAirForceGeneral", "usa_air_force", "usa"},
    {"AmericaLaserGeneral", "usa_laser", "usa"},
    {"AmericaSuperWeaponGeneral", "usa_superweapon", "usa"},
    {"China", "china_general", "china"},
    {"ChinaInfantryGeneral", "china_infantry", "china"},
    {"ChinaNukeGeneral", "china_nuke", "china"},
    {"ChinaTankGeneral", "china_tank", "china"},
    {"GLA", "gla_general", "gla"},
    {"GLADemolitionGeneral", "gla_demolition", "gla"},
    {"GLAStealthGeneral", "gla_stealth", "gla"},
    {"GLAToxinGeneral", "gla_toxin", "gla"},
};

const GeneralAsset *FindGeneralAsset(const AsciiString &side) {
  for (const GeneralAsset &asset : GENERAL_ASSETS) {
    if (side.compareNoCase(asset.side) == 0) {
      return &asset;
    }
  }

  return nullptr;
}

std::string GetMapDisplayName(const AsciiString &mapPath) {
  if (TheMapCache != nullptr) {
    const MapMetaData *mapData = TheMapCache->findMap(mapPath);
    if (mapData != nullptr && !mapData->m_displayName.isEmpty()) {
      return to_utf8(mapData->m_displayName.str());
    }
  }

  std::string name = mapPath.str();
  const size_t separator = name.find_last_of("\\/");
  if (separator != std::string::npos) {
    name.erase(0, separator + 1);
  }

  if (name.size() >= 4) {
    std::string extension = name.substr(name.size() - 4);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char character) {
                     return static_cast<char>(std::tolower(character));
                   });
    if (extension == ".map") {
      name.resize(name.size() - 4);
    }
  }

  return name.empty() ? "Unknown Map" : name;
}

std::string ClampDiscordString(std::string value) {
  constexpr size_t MAX_DISCORD_STRING_BYTES = 127;
  if (value.size() <= MAX_DISCORD_STRING_BYTES) {
    return value;
  }

  size_t cutoff = MAX_DISCORD_STRING_BYTES;
  while (cutoff > 0 &&
         (static_cast<unsigned char>(value[cutoff]) & 0xC0) == 0x80) {
    --cutoff;
  }
  value.resize(cutoff);
  return value;
}

template <typename T> T ResolveFunction(HMODULE module, const char *name) {
  return reinterpret_cast<T>(GetProcAddress(module, name));
}
} // namespace

GeneralsOnlineDiscordRPC::~GeneralsOnlineDiscordRPC() { Shutdown(); }

bool GeneralsOnlineDiscordRPC::Initialize() {
  Shutdown();

#ifdef __APPLE__
  m_initialize = DiscordIpc::Initialize;
  m_shutdown = DiscordIpc::Shutdown;
  m_runCallbacks = DiscordIpc::RunCallbacks;
  m_updatePresence = DiscordIpc::UpdatePresence;
  m_clearPresence = DiscordIpc::ClearPresence;
#else

  char executablePath[MAX_PATH] = {};
  const DWORD pathLength =
      GetModuleFileNameA(nullptr, executablePath, MAX_PATH);
  if (pathLength == 0 || pathLength == MAX_PATH) {
    NetworkLog(ELogVerbosity::LOG_RELEASE,
               "[DiscordRPC] Failed to locate the game executable");
    return false;
  }

  std::filesystem::path libraryPath(executablePath);
  libraryPath.replace_filename("discord-rpc.dll");

  HMODULE module = LoadLibraryA(libraryPath.string().c_str());
  if (module == nullptr) {
    NetworkLog(ELogVerbosity::LOG_RELEASE,
               "[DiscordRPC] Failed to load %s (error %lu)",
               libraryPath.string().c_str(), GetLastError());
    return false;
  }

  m_module = module;
  m_initialize =
      ResolveFunction<InitializeFunction>(module, "Discord_Initialize");
  m_shutdown = ResolveFunction<ShutdownFunction>(module, "Discord_Shutdown");
  m_runCallbacks =
      ResolveFunction<RunCallbacksFunction>(module, "Discord_RunCallbacks");
  m_updatePresence =
      ResolveFunction<UpdatePresenceFunction>(module, "Discord_UpdatePresence");
  m_clearPresence =
      ResolveFunction<ClearPresenceFunction>(module, "Discord_ClearPresence");

  if (m_initialize == nullptr || m_shutdown == nullptr ||
      m_runCallbacks == nullptr || m_updatePresence == nullptr ||
      m_clearPresence == nullptr) {
    NetworkLog(
        ELogVerbosity::LOG_RELEASE,
        "[DiscordRPC] The loaded library does not expose the required API");
    FreeLibrary(module);
    m_module = nullptr;
    ResetFunctions();
    return false;
  }
#endif

  DiscordEventHandlers handlers = {};
  m_initialize(DISCORD_APPLICATION_ID, &handlers, 0, nullptr);
  m_initialized = true;
  m_lastPresence.clear();
  m_nextPresenceUpdate = (std::chrono::steady_clock::time_point::min)();
  NetworkLog(ELogVerbosity::LOG_RELEASE,
             "[DiscordRPC] Rich Presence initialized");
  return true;
}

void GeneralsOnlineDiscordRPC::Shutdown() {
  if (m_initialized) {
    m_clearPresence();
    m_shutdown();
    m_initialized = false;
  }

  if (m_module != nullptr) {
    FreeLibrary(static_cast<HMODULE>(m_module));
    m_module = nullptr;
  }

  ResetFunctions();
  m_lastPresence.clear();
}

void GeneralsOnlineDiscordRPC::Tick(
    const NGMP_OnlineServices_LobbyInterface *lobbyInterface) {
  if (!m_initialized) {
    return;
  }

  m_runCallbacks();

  const auto now = std::chrono::steady_clock::now();
  if (now < m_nextPresenceUpdate) {
    return;
  }
  m_nextPresenceUpdate = now + PRESENCE_UPDATE_INTERVAL;

  const PresenceData data = BuildPresence(lobbyInterface);
  const std::string fingerprint = data.Fingerprint();
  if (fingerprint == m_lastPresence) {
    return;
  }

  UpdatePresence(data);
  m_lastPresence = fingerprint;
}

GeneralsOnlineDiscordRPC::PresenceData GeneralsOnlineDiscordRPC::BuildPresence(
    const NGMP_OnlineServices_LobbyInterface *lobbyInterface) const {
  PresenceData data;
  data.details = LOBBY_DETAILS;
  data.state = LOBBY_STATE;
  data.largeImageKey = DEFAULT_IMAGE_KEY;
  data.largeImageText = DEFAULT_IMAGE_TEXT;

  const GameInfo *game =
      TheGameInfo != nullptr && TheGameInfo->isGameInProgress() ? TheGameInfo
                                                                : nullptr;
  if (game == nullptr) {
    return data;
  }

  const LobbyEntry *lobby = nullptr;
  if (game == TheNGMPGame && lobbyInterface != nullptr &&
      lobbyInterface->IsInLobby()) {
    lobby = &lobbyInterface->GetCurrentLobby();
  }

  if (lobby != nullptr) {
    data.state = lobby->name;
    data.details = lobby->map_name;
    data.partyId = std::to_string(lobby->lobbyID);
    data.partySize = lobby->current_players;
    data.partyMax = lobby->max_players;
    data.smallImageText =
        std::format("{}$, {}, {}", lobby->starting_cash,
                    lobby->limit_superweapons ? "Limit SW" : "No SW Limit",
                    lobby->track_stats ? "Record Stats" : "No Stats");
  } else {
    data.state = game == TheNGMPGame ? to_utf8(TheNGMPGame->getGameName().str())
                                     : "In Match";
    data.details = GetMapDisplayName(game->getMap());
    data.partySize = game->getNumPlayers();
    data.partyMax = game->getMaxPlayers();
    data.smallImageText = std::format(
        "{}$, {}, {}", game->getStartingCash().countMoney(),
        game->getSuperweaponRestriction() ? "Limit SW" : "No SW Limit",
        game->getUseStats() ? "Record Stats" : "No Stats");
  }

  data.state = ClampDiscordString(data.state.empty() ? "In Match" : data.state);
  data.details = ClampDiscordString(
      data.details.empty() ? GetMapDisplayName(game->getMap()) : data.details);
  data.smallImageText = ClampDiscordString(data.smallImageText);
  data.partyMax = (std::max)(data.partyMax, 0);
  data.partySize = std::clamp(data.partySize, 0, data.partyMax);

  const Int localSlotIndex = game->getLocalSlotNum();
  const GameSlot *localSlot =
      localSlotIndex >= 0 ? game->getConstSlot(localSlotIndex) : nullptr;
  if (localSlot == nullptr ||
      localSlot->getPlayerTemplate() == PLAYERTEMPLATE_OBSERVER) {
    return data;
  }

  const Int templateIndex = localSlot->getPlayerTemplate();
  if (templateIndex < 0 ||
      templateIndex >= ThePlayerTemplateStore->getPlayerTemplateCount()) {
    return data;
  }

  const PlayerTemplate *playerTemplate =
      ThePlayerTemplateStore->getNthPlayerTemplate(templateIndex);
  if (playerTemplate == nullptr) {
    return data;
  }

  data.largeImageText =
      ClampDiscordString(to_utf8(playerTemplate->getDisplayName().str()));
  if (const GeneralAsset *asset = FindGeneralAsset(playerTemplate->getSide())) {
    data.largeImageKey = asset->generalImage;
    data.smallImageKey = asset->factionImage;
  }

  return data;
}

void GeneralsOnlineDiscordRPC::UpdatePresence(const PresenceData &data) {
  DiscordRichPresence presence = {};
  presence.state = data.state.c_str();
  presence.details = data.details.c_str();
  presence.largeImageKey = data.largeImageKey.c_str();
  presence.largeImageText = data.largeImageText.c_str();
  presence.smallImageKey =
      data.smallImageKey.empty() ? nullptr : data.smallImageKey.c_str();
  presence.smallImageText =
      data.smallImageText.empty() ? nullptr : data.smallImageText.c_str();
  presence.partyId = data.partyId.empty() ? nullptr : data.partyId.c_str();
  presence.partySize = data.partySize;
  presence.partyMax = data.partyMax;
  m_updatePresence(&presence);
}

void GeneralsOnlineDiscordRPC::ResetFunctions() {
  m_initialize = nullptr;
  m_shutdown = nullptr;
  m_runCallbacks = nullptr;
  m_updatePresence = nullptr;
  m_clearPresence = nullptr;
}

std::string GeneralsOnlineDiscordRPC::PresenceData::Fingerprint() const {
  return std::format("{}\n{}\n{}\n{}\n{}\n{}\n{}\n{}\n{}", state, details,
                     largeImageKey, largeImageText, smallImageKey,
                     smallImageText, partyId, partySize, partyMax);
}
