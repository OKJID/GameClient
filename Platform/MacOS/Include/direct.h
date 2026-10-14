#pragma once
#ifdef __APPLE__
#include <windows.h>
#include <sys/stat.h>
inline int _mkdir(const char* path) {
    char native[MAX_PATH];
    strncpy(native, path, MAX_PATH - 1);
    native[MAX_PATH - 1] = '\0';
    ConvertToNativePath(native);
    return mkdir(native, 0755);
}
#define _chdir chdir
#define _getcwd getcwd
#endif
