#pragma once

// Vanilla Generals does not ship the Generals Online client. This header provides
// the interface surface that shared Core code compiles against; the manager never
// hands out an interface, so Core keeps to its offline paths.

#include <cstdint>
#include <string>

class PSPlayerStats;

enum class EScreenshotType
{
	SCREENSHOT_TYPE_LOADSCREEN,
};

class NGMP_OnlineServices_AuthInterface
{
public:
	int64_t GetUserID() const { return -1; }
};

class NGMP_OnlineServices_StatsInterface
{
public:
	void getPlayerStatsFromCache(int profileID, PSPlayerStats *outStats) {}
};

class NGMP_OnlineServices_LobbyInterface;
class NGMP_OnlineServices_SocialInterface;

class NGMPGame
{
public:
	bool isQMGame() const { return false; }
};

class NGMP_OnlineServicesManager
{
public:
	static NGMP_OnlineServicesManager* GetInstance() { return nullptr; }

	template <typename T>
	static T* GetInterface() { return nullptr; }

	void Tick() {}
	void CaptureScreenshotForProbe(EScreenshotType type, const std::string& uri) {}
};
