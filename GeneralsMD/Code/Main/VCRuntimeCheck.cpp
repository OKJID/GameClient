// Refuses to start if the installed VC++ runtime is older than the one this build's toolset ships,
// and offers to download, verify and install the current redistributable, then relaunch the game.
// Microsoft only supports running a binary on a runtime at least as new as its toolset; older
// msvcp140.dll builds crash in std::mutex::lock (mtx_do_lock) among other things, so every loaded
// runtime DLL is checked before any user static initializer runs.
// The STL satellite DLLs (msvcp140_atomic_wait.dll etc.) are delay-loaded, so a redistributable too old to
// ship them still reaches this check; only a machine with no redistributable at all fails in the loader.
#if defined(_MSC_VER) && _MSC_VER >= 1900 && defined(_DLL)

#include <windows.h>
#include <shellapi.h>
#include <urlmon.h>
#include <wintrust.h>
#include <softpub.h>
#include <delayimp.h>
#include <cstring>
#include <cwchar>

#pragma comment(lib, "version.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")

extern "C" IMAGE_DOS_HEADER __ImageBase;

// Stamped by CMake from the toolset's redistributable; falls back to the compiler's minor version
#ifndef GO_VCREDIST_MAJOR
#define GO_VCREDIST_MAJOR 14
#define GO_VCREDIST_MINOR (_MSC_VER - 1900)
#define GO_VCREDIST_BUILD 0
#endif

namespace
{
	// Runtime DLLs whose version must be at least the toolset's; the optional ones only count when loaded
	const wchar_t* const kRuntimeDlls[] =
	{
		L"msvcp140.dll",
		L"vcruntime140.dll",
		L"msvcp140_atomic_wait.dll",
		L"msvcp140_1.dll",
		L"msvcp140_2.dll",
		L"concrt140.dll",
	};

	// Latest supported v14 redistributable, covers every 14.x toolset including VS2026
	const wchar_t kRedistUrl[] = L"https://aka.ms/vc14/vc_redist.x86.exe";
	const wchar_t kTitle[] = L"Generals Online";
	const wchar_t kAttemptedEnv[] = L"GO_VCREDIST_ATTEMPTED";

	struct Version
	{
		unsigned major;
		unsigned minor;
		unsigned build;
	};

	const Version kRequired = { GO_VCREDIST_MAJOR, GO_VCREDIST_MINOR, GO_VCREDIST_BUILD };

	bool IsOlder(const Version& a, const Version& b)
	{
		if (a.major != b.major) return a.major < b.major;
		if (a.minor != b.minor) return a.minor < b.minor;
		return a.build < b.build;
	}

	// Large enough for long paths and the relaunch command line
	wchar_t g_path[32768];
	wchar_t g_installer[MAX_PATH + 64];
	wchar_t g_detail[33024];
	wchar_t g_message[33792];
	wchar_t g_command[40000];

	// Wine/Proton ship their own runtime implementations whose version resources mean nothing here
	bool IsRunningUnderWine()
	{
		HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
		return hNtdll != nullptr && GetProcAddress(hNtdll, "wine_get_version") != nullptr;
	}

	// Support escape hatch: -skipRuntimeCheck on the command line or GO_SKIP_VCRUNTIME_CHECK set
	bool IsCheckDisabled()
	{
		const wchar_t* szCommandLine = GetCommandLineW();
		if (szCommandLine != nullptr && wcsstr(szCommandLine, L"-skipRuntimeCheck") != nullptr)
		{
			return true;
		}
		return GetEnvironmentVariableW(L"GO_SKIP_VCRUNTIME_CHECK", nullptr, 0) != 0;
	}

	bool GetModuleVersion(HMODULE hModule, wchar_t* szPath, DWORD pathLen, Version& version)
	{
		const DWORD len = GetModuleFileNameW(hModule, szPath, pathLen);
		if (len == 0 || len >= pathLen)
		{
			return false;
		}

		DWORD dwHandle = 0;
		const DWORD dwSize = GetFileVersionInfoSizeW(szPath, &dwHandle);
		if (dwSize == 0)
		{
			return false;
		}

		void* pData = HeapAlloc(GetProcessHeap(), 0, dwSize);
		if (pData == nullptr)
		{
			return false;
		}

		bool bFound = false;
		VS_FIXEDFILEINFO* pInfo = nullptr;
		UINT uInfoLen = 0;
		if (GetFileVersionInfoW(szPath, 0, dwSize, pData)
			&& VerQueryValueW(pData, L"\\", reinterpret_cast<LPVOID*>(&pInfo), &uInfoLen)
			&& pInfo != nullptr && uInfoLen >= sizeof(VS_FIXEDFILEINFO))
		{
			version.major = HIWORD(pInfo->dwFileVersionMS);
			version.minor = LOWORD(pInfo->dwFileVersionMS);
			version.build = HIWORD(pInfo->dwFileVersionLS);
			bFound = true;
		}

		HeapFree(GetProcessHeap(), 0, pData);
		return bFound;
	}

	// Only run a downloaded installer with a valid Authenticode signature from Microsoft
	bool IsSignedByMicrosoft(const wchar_t* szFile)
	{
		WINTRUST_FILE_INFO fileInfo = {};
		fileInfo.cbStruct = sizeof(fileInfo);
		fileInfo.pcwszFilePath = szFile;

		GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;
		WINTRUST_DATA trustData = {};
		trustData.cbStruct = sizeof(trustData);
		trustData.dwUIChoice = WTD_UI_NONE;
		trustData.fdwRevocationChecks = WTD_REVOKE_NONE;
		trustData.dwUnionChoice = WTD_CHOICE_FILE;
		trustData.pFile = &fileInfo;
		trustData.dwStateAction = WTD_STATEACTION_VERIFY;

		bool bTrusted = false;
		if (WinVerifyTrust(static_cast<HWND>(INVALID_HANDLE_VALUE), &action, &trustData) == ERROR_SUCCESS)
		{
			CRYPT_PROVIDER_DATA* pProvider = WTHelperProvDataFromStateData(trustData.hWVTStateData);
			CRYPT_PROVIDER_SGNR* pSigner = pProvider != nullptr ? WTHelperGetProvSignerFromChain(pProvider, 0, FALSE, 0) : nullptr;
			CRYPT_PROVIDER_CERT* pCert = pSigner != nullptr ? WTHelperGetProvCertFromChain(pSigner, 0) : nullptr;
			if (pCert != nullptr && pCert->pCert != nullptr)
			{
				wchar_t szSigner[256];
				if (CertGetNameStringW(pCert->pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, szSigner, ARRAYSIZE(szSigner)) > 1)
				{
					bTrusted = wcscmp(szSigner, L"Microsoft Corporation") == 0;
				}
			}
		}

		trustData.dwStateAction = WTD_STATEACTION_CLOSE;
		WinVerifyTrust(static_cast<HWND>(INVALID_HANDLE_VALUE), &action, &trustData);
		return bTrusted;
	}

	bool DownloadInstaller()
	{
		wchar_t szTemp[MAX_PATH + 1];
		const DWORD tempLen = GetTempPathW(ARRAYSIZE(szTemp), szTemp);
		if (tempLen == 0 || tempLen > MAX_PATH)
		{
			return false;
		}
		swprintf_s(g_installer, L"%sGeneralsOnline_vc_redist.x86.exe", szTemp);
		DeleteFileW(g_installer);

		return SUCCEEDED(URLDownloadToFileW(nullptr, kRedistUrl, g_installer, 0, nullptr))
			&& IsSignedByMicrosoft(g_installer);
	}

	// The arguments after argv[0], parsed with the same quoting rules the CRT uses for argv[0]
	const wchar_t* GetArgumentsAfterProgram()
	{
		const wchar_t* p = GetCommandLineW();
		if (p == nullptr)
		{
			return L"";
		}
		if (*p == L'"')
		{
			++p;
			while (*p != L'\0' && *p != L'"')
				++p;
			if (*p == L'"')
				++p;
		}
		else
		{
			while (*p != L'\0' && *p != L' ' && *p != L'\t')
				++p;
		}
		while (*p == L' ' || *p == L'\t')
			++p;
		return p;
	}

	// cmd runs the installer after this process exits (it holds the old DLLs open), then restarts the game
	bool LaunchInstallerAndRelaunch(bool& bWillRelaunch)
	{
		wchar_t szCmd[MAX_PATH + 16];
		const UINT sysLen = GetSystemDirectoryW(szCmd, MAX_PATH);
		if (sysLen == 0 || sysLen >= MAX_PATH)
		{
			return false;
		}
		wcscat_s(szCmd, L"\\cmd.exe");

		// relaunch the exe by its full path, quoted, with the original arguments;
		// cmd metacharacters in either would be interpreted, so skip the relaunch then
		const DWORD exeLen = GetModuleFileNameW(nullptr, g_path, ARRAYSIZE(g_path));
		const wchar_t* szArguments = GetArgumentsAfterProgram();
		bWillRelaunch = exeLen != 0 && exeLen < ARRAYSIZE(g_path)
			&& wcspbrk(g_path, L"&|<>^%\"") == nullptr && wcspbrk(szArguments, L"&|<>^%") == nullptr;

		if (bWillRelaunch)
		{
			swprintf_s(g_command, L"\"%s\" /d /s /c \"start \"\" /wait \"%s\" /install /passive /norestart & set %s=1& start \"\" \"%s\" %s\"",
				szCmd, g_installer, kAttemptedEnv, g_path, szArguments);
		}
		else
		{
			swprintf_s(g_command, L"\"%s\" /d /s /c \"start \"\" /wait \"%s\" /install /passive /norestart\"",
				szCmd, g_installer);
		}

		STARTUPINFOW startupInfo = {};
		startupInfo.cb = sizeof(startupInfo);
		PROCESS_INFORMATION processInfo = {};
		if (!CreateProcessW(szCmd, g_command, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo))
		{
			return false;
		}
		CloseHandle(processInfo.hThread);
		CloseHandle(processInfo.hProcess);
		return true;
	}

	[[noreturn]] void OfferDownloadPageAndExit(const wchar_t* szMessage)
	{
		if (MessageBoxW(nullptr, szMessage, kTitle, MB_YESNO | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST) == IDYES)
		{
			ShellExecuteW(nullptr, L"open", kRedistUrl, nullptr, nullptr, SW_SHOWNORMAL);
		}
		ExitProcess(1);
	}

	// installed == nullptr means the DLL is missing entirely
	[[noreturn]] void HandleOutdatedRuntime(const wchar_t* szDll, const Version* installed)
	{
		if (installed != nullptr)
		{
			swprintf_s(g_detail, L"%s is version %u.%u.%u:\n%s", szDll, installed->major, installed->minor, installed->build, g_path);
		}
		else
		{
			swprintf_s(g_detail, L"%s is missing.", szDll);
		}

		// already installed once and still outdated: the new files only take effect after a Windows restart
		if (GetEnvironmentVariableW(kAttemptedEnv, nullptr, 0) != 0)
		{
			swprintf_s(g_message,
				L"The Microsoft Visual C++ Redistributable was installed, but the game still can't use it "
				L"(needs %u.%u.%u or newer):\n%s\n\n"
				L"Restart Windows and start the game again.\n\n"
				L"If this keeps happening, install it manually. Open the download page?",
				kRequired.major, kRequired.minor, kRequired.build, g_detail);
			OfferDownloadPageAndExit(g_message);
		}

		swprintf_s(g_message,
			L"Generals Online needs the Microsoft Visual C++ Redistributable (x86) %u.%u.%u or newer.\n\n"
			L"%s\n\n"
			L"Install it now? The official installer is downloaded from Microsoft, "
			L"and the game restarts when it's done.",
			kRequired.major, kRequired.minor, kRequired.build, g_detail);

		if (MessageBoxW(nullptr, g_message, kTitle, MB_YESNO | MB_ICONWARNING | MB_SETFOREGROUND | MB_TOPMOST) != IDYES)
		{
			ExitProcess(1);
		}

		bool bWillRelaunch = false;
		if (DownloadInstaller() && LaunchInstallerAndRelaunch(bWillRelaunch))
		{
			if (!bWillRelaunch)
			{
				MessageBoxW(nullptr, L"The installer is starting. Start the game again once it has finished.", kTitle, MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
			}
			ExitProcess(0);
		}

		OfferDownloadPageAndExit(L"The Visual C++ Redistributable could not be downloaded or verified.\n\nOpen the download page to install it manually?");
	}

	bool IsRuntimeDllName(const char* szName)
	{
		return _strnicmp(szName, "msvcp140", 8) == 0
			|| _strnicmp(szName, "vcruntime140", 12) == 0
			|| _strnicmp(szName, "concrt140", 9) == 0;
	}

	// Runtime satellites are delay-loaded so old redistributables don't stop the loader; load them now so a
	// missing or outdated one is handled here instead of throwing on its first call mid-game
	void CheckDelayLoadedRuntimes()
	{
		const BYTE* pBase = reinterpret_cast<const BYTE*>(&__ImageBase);
		const IMAGE_NT_HEADERS* pNtHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(pBase + __ImageBase.e_lfanew);
		const IMAGE_DATA_DIRECTORY& delayDir = pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT];
		if (delayDir.VirtualAddress == 0 || delayDir.Size == 0)
		{
			return;
		}

		for (const ImgDelayDescr* pDescr = reinterpret_cast<const ImgDelayDescr*>(pBase + delayDir.VirtualAddress);
			pDescr->rvaDLLName != 0; ++pDescr)
		{
			// every descriptor the current linker emits uses RVAs
			if ((pDescr->grAttrs & dlattrRva) == 0)
			{
				continue;
			}

			const char* szName = reinterpret_cast<const char*>(pBase + pDescr->rvaDLLName);
			if (!IsRuntimeDllName(szName))
			{
				continue;
			}

			wchar_t szWideName[64];
			if (MultiByteToWideChar(CP_ACP, 0, szName, -1, szWideName, ARRAYSIZE(szWideName)) == 0)
			{
				continue;
			}

			HMODULE hModule = LoadLibraryW(szWideName);
			if (hModule == nullptr)
			{
				HandleOutdatedRuntime(szWideName, nullptr);
			}

			Version installed = {};
			if (GetModuleVersion(hModule, g_path, ARRAYSIZE(g_path), installed) && IsOlder(installed, kRequired))
			{
				HandleOutdatedRuntime(szWideName, &installed);
			}
		}
	}

	void CheckVCRuntimeVersion()
	{
		if (IsCheckDisabled() || IsRunningUnderWine())
		{
			return;
		}

		for (size_t i = 0; i < ARRAYSIZE(kRuntimeDlls); ++i)
		{
			HMODULE hModule = GetModuleHandleW(kRuntimeDlls[i]);
			Version installed = {};
			if (hModule == nullptr || !GetModuleVersion(hModule, g_path, ARRAYSIZE(g_path), installed))
			{
				continue;
			}

			if (IsOlder(installed, kRequired))
			{
				HandleOutdatedRuntime(kRuntimeDlls[i], &installed);
			}
		}

		CheckDelayLoadedRuntimes();
	}

	struct VCRuntimeChecker
	{
		VCRuntimeChecker() { CheckVCRuntimeVersion(); }
	};

#pragma warning(push)
#pragma warning(disable: 4073)
#pragma init_seg(lib)
	VCRuntimeChecker g_vcRuntimeChecker;
#pragma warning(pop)
}

#endif // _MSC_VER >= 1900 && _DLL
