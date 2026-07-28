#pragma once


enum class ELogVerbosity
{
	LOG_DEBUG = 0,
	LOG_RELEASE = 1
};

// TODO(PS_PATH): Remove NGMP_COMPAT_OLD_CHANNEL_PROTOCOL when GO production Windows client
// updates past commit 28c93dd89 ("Cleanup of network transport and header code", May 10 2026).
// Production build as of May 25 2026 is from April 28 2026 — uses old 3-byte AC header [9,1,2]
// and sends game packets WITHOUT a channel header byte.
// grep "NGMP_COMPAT_OLD_CHANNEL_PROTOCOL" to find all affected locations.
#ifdef __APPLE__
#define NGMP_COMPAT_OLD_CHANNEL_PROTOCOL
#endif

enum class ENetworkChannel : BYTE
{
	NETWORK_CHANNEL_GAME = 0,
	NETWORK_CHANNEL_AC = 1
};

#define CHECK_MAIN_THREAD assert(std::this_thread::get_id() == NGMP_OnlineServicesManager::g_MainThreadID);
#define CHECK_WORKER_THREAD assert(std::this_thread::get_id() != NGMP_OnlineServicesManager::g_MainThreadID);

static const ELogVerbosity g_LogVerbosity =
#if _DEBUG
ELogVerbosity::LOG_DEBUG;
#else
ELogVerbosity::LOG_RELEASE;
#endif

void NetworkLog(ELogVerbosity logVerbosity, const char* fmt, ...);

std::string to_utf8(const std::wstring& wstr);
std::wstring from_utf8(const std::string& utf8_str);

int RoundUpLatencyToFrameInterval(int latency, int frameInterval);
int ConvertMSLatencyToFrames(int ms);
int ConvertMSLatencyToGenToolFrames(int ms);
// NGMP_NOTE: Plr Templates look like this:
/*
| NAME							|	GAME INDEX	|	SERVICE INDEX	|	PLAYABLE	|
-------------------------------------------------------------------------------------
Civilian								0				-1					No
Observer								1				-1					No
America									2				0					Yes
China									3				1					Yes
GLA										4				2					Yes
AmericaSuperWeaponGeneral				5				3					Yes
AmericaLaserGeneral						6				4					Yes
AmericaAirForceGeneral					7				5					Yes
ChinaTankGeneral						8				6					Yes
ChinaInfantryGeneral					9				7					Yes
ChinaNukeGeneral						10				8					Yes
ChinaNukeGeneral						11				9					Yes
GLAToxinGeneral							12				10					Yes
GLADemolitionGeneral					13				11					Yes
GLAStealthGeneral						14				12					Yes
Boss									15				-1					No
*/

static std::unordered_map<int, std::string> g_mapServiceIndexToPlayerTemplateString =
{
	{ 0, "USA" }, // NOTE: Gamespy used "America", the game uses "USA"
	{ 1, "China" },
	{ 2, "GLA" },
	{ 3, "AmericaSuperWeaponGeneral" },
	{ 4, "AmericaLaserGeneral" },
	{ 5, "AmericaAirForceGeneral" },
	{ 6, "ChinaTankGeneral" },
	{ 7, "ChinaInfantryGeneral" },
	{ 8, "ChinaNukeGeneral" },
	{ 9, "GLAToxinGeneral" },
	{ 10, "GLADemolitionGeneral" },
	{ 11, "GLAStealthGeneral" }
};

// common game engine includes
#include "Common/UnicodeString.h"
#include "Common/AsciiString.h"
// standard libs
#include <fstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>

#include "GameNetwork/GeneralsOnline/NextGenMP_defines.h"
#include "GameNetwork/GeneralsOnline/NetworkPacket.h"
#include "GameNetwork/GeneralsOnline/NetworkBitstream.h"
#include "GameNetwork/GeneralsOnline/NGMP_types.h"
#include "GameNetwork/GeneralsOnline/NGMPGame.h"

#include "NextGenTransport.h"


#if defined(GENERALS_ONLINE_BRANCH_JMARSHALL)
#include "../Console/Console.h"
#endif

#include "GameNetwork/GeneralsOnline/json.hpp"

std::string Base64Encode(const std::vector<uint8_t>& data);
std::vector<uint8_t> Base64Decode(const std::string& encodedData);

const char* GetHTTPVersionName(long curlHTTPVersion);
