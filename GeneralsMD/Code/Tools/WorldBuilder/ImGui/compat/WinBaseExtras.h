#pragma once

#define SND_SYNC 0x0000
#define SND_ASYNC 0x0001
#define SND_NODEFAULT 0x0002
#define SND_MEMORY 0x0004
#define SND_LOOP 0x0008
#define SND_NOSTOP 0x0010
#define SND_PURGE 0x0040
#define SND_FILENAME 0x00020000L

BOOL PlaySoundA(LPCSTR sound, HMODULE module, DWORD flags);
#define PlaySound PlaySoundA

HRESULT CoInitialize(void* reserved);
void CoUninitialize();

DWORD GetFullPathNameA(LPCSTR fileName, DWORD bufferLength, LPSTR buffer, LPSTR* filePart);
#define GetFullPathName GetFullPathNameA

UINT GetPrivateProfileIntA(LPCSTR section, LPCSTR key, INT defaultValue, LPCSTR fileName);
DWORD GetPrivateProfileStringA(LPCSTR section, LPCSTR key, LPCSTR defaultValue, LPSTR buffer, DWORD bufferSize, LPCSTR fileName);
BOOL WritePrivateProfileStringA(LPCSTR section, LPCSTR key, LPCSTR value, LPCSTR fileName);
#define GetPrivateProfileInt GetPrivateProfileIntA
#define GetPrivateProfileString GetPrivateProfileStringA
#define WritePrivateProfileString WritePrivateProfileStringA
