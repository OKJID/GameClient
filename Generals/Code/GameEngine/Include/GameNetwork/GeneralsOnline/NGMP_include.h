#pragma once

// Vanilla Generals does not ship the Generals Online client, but the shared network
// code in Core logs through NetworkLog and converts latency to frames regardless of
// which transport is active. This header carries that game-agnostic surface.

#include <string>

#include "GameNetwork/GeneralsOnline/NextGenMP_defines.h"
#include "GameNetwork/GeneralsOnline/NGMP_interfaces.h"

enum class ELogVerbosity
{
	LOG_DEBUG = 0,
	LOG_RELEASE = 1
};

enum class ENetworkChannel : BYTE
{
	NETWORK_CHANNEL_GAME = 0,
	NETWORK_CHANNEL_AC = 1
};

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
