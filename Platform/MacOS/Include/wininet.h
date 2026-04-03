#pragma once
#ifdef __APPLE__

// TODO(PS_PATH): Implement HTTP upload via NSURLSession/curl for StatsUploader
#include <windows.h>

typedef void* HINTERNET;

#define INTERNET_OPEN_TYPE_DIRECT 1
#define INTERNET_FLAG_RELOAD 0x80000000
#define INTERNET_FLAG_NO_CACHE_WRITE 0x04000000
#define HTTP_QUERY_STATUS_CODE 19
#define HTTP_QUERY_FLAG_NUMBER 0x20000000

inline HINTERNET InternetOpen(LPCSTR, DWORD, LPCSTR, LPCSTR, DWORD) { return nullptr; }
inline HINTERNET InternetConnect(HINTERNET, LPCSTR, WORD, LPCSTR, LPCSTR, DWORD, DWORD, DWORD_PTR) { return nullptr; }
inline HINTERNET HttpOpenRequest(HINTERNET, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPCSTR*, DWORD, DWORD_PTR) { return nullptr; }
inline BOOL HttpSendRequest(HINTERNET, LPCSTR, DWORD, LPVOID, DWORD) { return FALSE; }
inline BOOL HttpQueryInfo(HINTERNET, DWORD, LPVOID, LPDWORD, LPDWORD) { return FALSE; }
inline BOOL InternetCloseHandle(HINTERNET) { return TRUE; }

#endif
