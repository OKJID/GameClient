#pragma once
#ifdef __APPLE__

#include <windows.h>

typedef DWORD* LPDWORD;
typedef DWORD* PDWORD;

typedef struct _IMAGEHLP_SYMBOL {
    DWORD SizeOfStruct;
    DWORD Address;
    DWORD Size;
    DWORD Flags;
    DWORD MaxNameLength;
    char Name[1];
} IMAGEHLP_SYMBOL, *PIMAGEHLP_SYMBOL;

typedef struct _IMAGEHLP_LINE {
    DWORD SizeOfStruct;
    LPVOID Key;
    DWORD LineNumber;
    char* FileName;
    DWORD Address;
} IMAGEHLP_LINE, *PIMAGEHLP_LINE;

typedef struct _tagSTACKFRAME {
    DWORD AddrPC;
    DWORD AddrReturn;
    DWORD AddrFrame;
    DWORD AddrStack;
    LPVOID FuncTableEntry;
    DWORD Params[4];
    BOOL Far;
    BOOL Virtual;
    DWORD Reserved[3];
} STACKFRAME, *LPSTACKFRAME;

typedef BOOL (*PREAD_PROCESS_MEMORY_ROUTINE)(HANDLE, DWORD, LPVOID, DWORD, LPDWORD);
typedef LPVOID (*PFUNCTION_TABLE_ACCESS_ROUTINE)(HANDLE, DWORD);
typedef DWORD (*PGET_MODULE_BASE_ROUTINE)(HANDLE, DWORD);
typedef DWORD (*PTRANSLATE_ADDRESS_ROUTINE)(HANDLE, HANDLE, LPVOID);

// TODO(PS_PATH): MINIDUMP types for DbgHelpLoader — no-op on macOS
typedef int MINIDUMP_TYPE;
typedef void* PMINIDUMP_EXCEPTION_INFORMATION;
typedef void* PMINIDUMP_USER_STREAM_INFORMATION;
typedef void* PMINIDUMP_CALLBACK_INFORMATION;
typedef struct _EXCEPTION_POINTERS { int ExceptionPointers; } EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;

#define MiniDumpNormal 0

// TODO(PS_PATH): crash dump stub — implement macOS crash reporting
inline BOOL MiniDumpWriteDump(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
    PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION,
    PMINIDUMP_CALLBACK_INFORMATION) { return FALSE; }

#endif
