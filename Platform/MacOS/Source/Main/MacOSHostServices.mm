// MacOSHostServices.mm — host queries the engine makes, shared by the game and WorldBuilder

#import <AppKit/AppKit.h>

#include "MacOSHostServices.h"

#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <sys/sysctl.h>

char MacOSCommandLineString[4096] = "";

void MacOS_StoreCommandLine(int argc, char* argv[]) {
    MacOSCommandLineString[0] = '\0';
    for (int i=0; i<argc; ++i) {
        if (i>0) strncat(MacOSCommandLineString, " ", sizeof(MacOSCommandLineString)-1 - strlen(MacOSCommandLineString));
        bool hasSpace = strchr(argv[i], ' ') != nullptr;
        if (hasSpace) strncat(MacOSCommandLineString, "\"", sizeof(MacOSCommandLineString)-1 - strlen(MacOSCommandLineString));
        strncat(MacOSCommandLineString, argv[i], sizeof(MacOSCommandLineString)-1 - strlen(MacOSCommandLineString));
        if (hasSpace) strncat(MacOSCommandLineString, "\"", sizeof(MacOSCommandLineString)-1 - strlen(MacOSCommandLineString));
    }
    printf("[DIAG] MacOSMain: Built cmd line: %s\n", MacOSCommandLineString);
    fflush(stdout);
}

extern "C" unsigned long long MacOS_GetTotalPhysicalMemory() {
    int64_t physical_memory = 0;
    size_t length = sizeof(int64_t);
    int res = sysctlbyname("hw.memsize", &physical_memory, &length, nullptr, 0);
    printf("DEBUG: MacOS_GetTotalPhysicalMemory -> res=%d, mem=%llu\n", res, (unsigned long long)physical_memory); fflush(stdout);
    return 2048ULL * 1024 * 1024; // Force it to 2GB to be safe
}

// TheSuperHackers @feature macOS: Compute 90% of main screen dimensions for
// first-time users with no saved resolution. Called from GlobalData.cpp when
// OptionPreferences has no "Resolution" key (i.e., default 800x600 is returned).
extern "C" void MacOS_GetAdaptiveResolution(int *w, int *h) {
    NSScreen *screen = [NSScreen mainScreen];
    if (!screen) return;

    NSRect frame = [screen visibleFrame];
    *w = (int)(frame.size.width * 0.9);
    *h = (int)(frame.size.height * 0.9);

    // Ensure even dimensions (Metal drawable requirement)
    *w &= ~1;
    *h &= ~1;

    printf("[MacOS] GetAdaptiveResolution: screen=%.0fx%.0f -> adaptive=%dx%d\n",
           frame.size.width, frame.size.height, *w, *h);
    fflush(stdout);
}
