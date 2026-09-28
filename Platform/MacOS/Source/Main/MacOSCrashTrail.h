#pragma once

namespace MacOSCrashTrail
{
    void start();
    void mark(const char* format, ...) __attribute__((format(printf, 1, 2)));
    void setExceptionReason(const char* format, ...) __attribute__((format(printf, 1, 2)));
    void report(void (*logLine)(const char*));
}
