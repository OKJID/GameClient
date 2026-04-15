#pragma once
#ifdef __APPLE__
#include <windows.h>
#include <sys/stat.h>
#define _mkdir(path) mkdir(path, 0755)
#define _chdir chdir
#define _getcwd getcwd
#endif
