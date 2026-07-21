#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "GameNetwork/GeneralsOnline/NGMP_include.h"

#include <codecvt>
#include <cstdarg>
#include <locale>

// Vanilla has no Generals Online session to attach a network log to, so the shared
// network code logs through the engine's own debug log instead of a separate file.

std::string to_utf8(const std::wstring& wstr)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(wstr);
}

std::wstring from_utf8(const std::string& utf8_str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.from_bytes(utf8_str);
}

void NetworkLog(ELogVerbosity logVerbosity, const char* fmt, ...)
{
	if (logVerbosity < g_LogVerbosity)
	{
		return;
	}

	char buffer[2048];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	DEBUG_LOG(("%s", buffer));
}

int RoundUpLatencyToFrameInterval(int latency, int frameInterval)
{
	if (frameInterval == 0)
		return latency;

	int remainder = latency % frameInterval;
	if (remainder == 0)
		return latency;

	return latency + frameInterval - remainder;
}

int ConvertMSLatencyToFrames(int ms)
{
	ms = RoundUpLatencyToFrameInterval(ms, 1000 / GENERALS_ONLINE_HIGH_FPS_LIMIT);
	return (int)ceil((ms / 1000.f) * (float)GENERALS_ONLINE_HIGH_FPS_LIMIT);
}

int ConvertMSLatencyToGenToolFrames(int ms)
{
	return (int)ceil((float)ConvertMSLatencyToFrames(ms) / (float)GENERALS_ONLINE_HIGH_FPS_FRAME_MULTIPLIER);
}
