#include "MacOSCrashTrail.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <mach/mach_time.h>
#include <os/lock.h>

namespace
{
    const int kTrailEntries = 32;
    const size_t kEntrySize = 256;
    const size_t kReasonSize = 1024;
    const size_t kTimestampSize = 32;
    const size_t kSupportIdSize = 32;
    const char* const kLauncherStartedAtKey = "GENERALS_LAUNCHER_STARTED_AT";
    const char* const kSupportIdKey = "GENERALS_SUPPORT_ID";

    char s_entries[kTrailEntries][kEntrySize];
    int s_markCount = 0;
    os_unfair_lock s_markLock = OS_UNFAIR_LOCK_INIT;

    char s_exceptionReason[kReasonSize];
    char s_gameStartedAt[kTimestampSize];
    char s_launcherStartedAt[kTimestampSize];
    char s_supportId[kSupportIdSize];

    time_t s_gameStartTime = 0;
    uint64_t s_startAbsoluteTicks = 0;
    uint64_t s_startContinuousTicks = 0;
    mach_timebase_info_data_t s_timebase = {};

    void formatUtc(time_t moment, char* out, size_t size)
    {
        struct tm utc;
        gmtime_r(&moment, &utc);
        snprintf(out, size, "%04d-%02d-%02d %02d:%02d:%02d UTC",
            utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday, utc.tm_hour, utc.tm_min, utc.tm_sec);
    }

    void prepareLauncherStartedAt()
    {
        const char* value = getenv(kLauncherStartedAtKey);
        const time_t launcherStartTime = value != nullptr ? (time_t)strtoll(value, nullptr, 10) : 0;
        if (launcherStartTime <= 0) {
            snprintf(s_launcherStartedAt, sizeof(s_launcherStartedAt), "not launched by the launcher");
            return;
        }

        formatUtc(launcherStartTime, s_launcherStartedAt, sizeof(s_launcherStartedAt));
    }

    void prepareSupportId()
    {
        const char* value = getenv(kSupportIdKey);
        snprintf(s_supportId, sizeof(s_supportId), "%s", value != nullptr ? value : "unknown");
    }

    uint64_t ticksToSeconds(uint64_t ticks)
    {
        if (s_timebase.denom == 0) {
            return 0;
        }

        return ticks * s_timebase.numer / s_timebase.denom / 1000000000ULL;
    }

    void replaceLineBreaks(char* text)
    {
        for (char* cursor = text; *cursor != '\0'; ++cursor) {
            if (*cursor == '\n' || *cursor == '\r') {
                *cursor = ' ';
            }
        }
    }

    void reportSessionTimes(void (*logLine)(const char*))
    {
        char line[128];
        snprintf(line, sizeof(line), "CRASH SUPPORT ID: %s", s_supportId);
        logLine(line);
        snprintf(line, sizeof(line), "CRASH LAUNCHER STARTED: %s", s_launcherStartedAt);
        logLine(line);
        snprintf(line, sizeof(line), "CRASH GAME STARTED: %s", s_gameStartedAt);
        logLine(line);

        const uint64_t awakeTicks = mach_absolute_time() - s_startAbsoluteTicks;
        const uint64_t totalTicks = mach_continuous_time() - s_startContinuousTicks;
        snprintf(line, sizeof(line), "CRASH RUN TIME: %ld s, asleep %llu s",
            (long)(time(nullptr) - s_gameStartTime), (unsigned long long)ticksToSeconds(totalTicks - awakeTicks));
        logLine(line);
    }

    void reportTrail(void (*logLine)(const char*))
    {
        const int markCount = s_markCount;
        const int firstMark = markCount > kTrailEntries ? markCount - kTrailEntries : 0;

        char line[kEntrySize + 32];
        snprintf(line, sizeof(line), "CRASH TRAIL: last %d of %d events", markCount - firstMark, markCount);
        logLine(line);

        for (int mark = firstMark; mark < markCount; ++mark) {
            snprintf(line, sizeof(line), "  %s", s_entries[mark % kTrailEntries]);
            logLine(line);
        }
    }
}

namespace MacOSCrashTrail
{
    void start()
    {
        s_gameStartTime = time(nullptr);
        s_startAbsoluteTicks = mach_absolute_time();
        s_startContinuousTicks = mach_continuous_time();
        mach_timebase_info(&s_timebase);

        formatUtc(s_gameStartTime, s_gameStartedAt, sizeof(s_gameStartedAt));
        prepareLauncherStartedAt();
        prepareSupportId();
        mark("game started");
    }

    void mark(const char* format, ...)
    {
        char text[kEntrySize];
        va_list args;
        va_start(args, format);
        vsnprintf(text, sizeof(text), format, args);
        va_end(args);
        replaceLineBreaks(text);

        const time_t now = time(nullptr);
        struct tm utc;
        gmtime_r(&now, &utc);

        os_unfair_lock_lock(&s_markLock);
        snprintf(s_entries[s_markCount % kTrailEntries], kEntrySize, "%02d:%02d:%02d +%lds %s",
            utc.tm_hour, utc.tm_min, utc.tm_sec, (long)(now - s_gameStartTime), text);
        ++s_markCount;
        os_unfair_lock_unlock(&s_markLock);
    }

    void setExceptionReason(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        vsnprintf(s_exceptionReason, sizeof(s_exceptionReason), format, args);
        va_end(args);
        replaceLineBreaks(s_exceptionReason);
    }

    void report(void (*logLine)(const char*))
    {
        if (s_exceptionReason[0] != '\0') {
            char line[kReasonSize + 32];
            snprintf(line, sizeof(line), "CRASH EXCEPTION: %s", s_exceptionReason);
            logLine(line);
        }

        reportSessionTimes(logLine);
        reportTrail(logLine);
    }
}
