# macOS Port — File System

## Windows Flow (Reference)

On Windows, the `generals.exe` binary lives inside the Zero Hour folder. CWD at launch = ZH folder.

```
C:\EA Games\
├── Command and Conquer Generals Zero Hour\
│   ├── generals.exe          ← CWD = here
│   ├── *.big                 ← ZH big files
│   └── Data/Scripts/SkirmishScripts.scb
└── Command and Conquer Generals\          ← Registry InstallPath
    ├── *.big                 ← Base big files
    └── Data/...
```

`Win32BIGFileSystem::init()`:
1. `loadBigFilesFromDirectory("", "*.big")` → CWD = ZH
2. `GetStringFromGeneralsRegistry("", "InstallPath")` → path to Base from registry
3. `loadBigFilesFromDirectory(installPath, "*.big")` → Base

**Priority (first loaded wins):** `Loose CWD > BIG CWD > BIG Base`

## macOS: Mirroring Windows via `chdir()`

On macOS the binary lives inside an `.app` bundle or the build directory — **not** in the game data folder. To make the shared engine code (`StdBIGFileSystem::init`) work identically to Windows without any `#ifdef`, the Windows environment is recreated at startup.

### `GENERALS_INSTALL_PATH` Variable

```bash
export GENERALS_INSTALL_PATH="/Users/okji/dev/games/Command and Conquer - Generals"
```

Points to the **root** directory containing subdirectories for each game mode.

### User Data Folder (Replays, Maps, Cache)

User data (custom maps, `MapCache.ini` cache, replays, saves) is stored in the home directory. The path is derived from the `UserDataLeafName` registry key (set in `GlobalData::BuildUserDataPathFromRegistry()`).
The `SHGetSpecialFolderPath` shim with `CSIDL_PERSONAL` flag resolves to the home directory root (`~/`) on macOS.

Example path: `~/Command and Conquer Generals Zero Hour Data/`

Custom maps go into the `Maps/` subfolder. Cache files (`MapCache.ini`, `MapCacheGO.ini`) are auto-generated there.

> **Important:** The engine internally constructs this path with Windows-style backslashes `\`, which are translated to `/` only when reaching native macOS APIs through `MacOSLocalFileSystem` and the `fopen`/`CreateDirectory` interceptors.

### Data Structure

```
Command and Conquer - Generals/
├── Command and Conquer Generals Zero Hour/   ← ZH (marker: INIZH.big)
│   ├── *.big
│   └── Data/Scripts/SkirmishScripts.scb
└── Command and Conquer Generals/             ← Base (marker: INI.big, no INIZH.big)
    ├── *.big
    └── Data/...
```

> Subdirectory names are NOT hardcoded. The detector scans contents and looks for markers.

### Initialization (MacOSGameEngine::init)

Before calling `GameEngine::init()`:

1. Read `GENERALS_INSTALL_PATH`
2. `DetectGameModes(rootPath)` scans subdirectories:
   - **ZH** = contains `INIZH.big`
   - **Base** = contains `INI.big`, but NOT `INIZH.big`
3. `chdir(zhPath)` — CWD is now ZH, identical to Windows
4. `basePath` is saved for `registry.cpp`

After this, `GameEngine::init()` → `StdBIGFileSystem::init()` works **identically to Windows**:
- `loadBigFilesFromDirectory("", "*.big")` → CWD = ZH ✅
- `GetStringFromGeneralsRegistry("", "InstallPath")` → `basePath` ✅
- `loadBigFilesFromDirectory(basePath, "*.big")` → Base ✅

## Architecture

| Component | Class | Purpose |
|-----------|-------|---------|
| `TheLocalFileSystem` | `MacOSLocalFileSystem` | Loose files on disk + slash normalization + case-insensitive lookup |
| `TheArchiveFileSystem` | `StdBIGFileSystem` | Files inside `.big` archives |

When `TheFileSystem->openFile(path)` is called:
1. First searches for a **loose file** via `TheLocalFileSystem->openFile`
2. If not found — searches in **`.big` archives** via `TheArchiveFileSystem->openFile`

> Loose files **always** take priority over `.big` archives.

## Slash Boundary (Normalization Layer)

The SAGE engine is hardcoded for Windows paths (`\` and case-insensitive). `MacOSLocalFileSystem` isolates this from POSIX:

**Inbound (to OS):** Methods `openFile`, `doesFileExist`, `getFileInfo` pass paths through `resolveWithSearchPaths`:
- `\` → `/`
- Case-insensitive matching via `strcasecmp`

**Outbound (to SAGE engine):** `getFileListInDirectory` converts `/` → `\` in returned paths.

> No `#ifdef __APPLE__` inside `StdLocalFileSystem.cpp`, `MapUtil.cpp`, or `INIMapCache.cpp`.
> Shared code operates as if it were on Windows.

## NativeFileSystem Facade

The engine sometimes bypasses the virtual file system and calls `fopen` or `CreateDirectory` directly with Windows-style backslash paths. On macOS, these calls fail silently or create files/directories with literal backslashes in their names.

The platform header `Platform/MacOS/Include/windows.h` provides interceptors:

```cpp
// CreateDirectory interceptor — normalizes backslashes before mkdir
inline BOOL MacOS_CreateDirectory(LPCSTR lpPathName, void*) {
    std::string safePath(lpPathName);
    std::replace(safePath.begin(), safePath.end(), '\\', '/');
    return mkdir(safePath.c_str(), 0755) == 0 || errno == EEXIST;
}
#define CreateDirectory MacOS_CreateDirectory

// fopen interceptor — normalizes backslashes before opening
inline FILE* MacOS_fopen(const char* filename, const char* mode) {
    std::string safePath(filename);
    std::replace(safePath.begin(), safePath.end(), '\\', '/');
    return fopen(safePath.c_str(), mode);
}
```

This fixes `MapCache.ini` read/write, user data directory creation, and replay file operations without modifying any shared engine code.

## addSearchPath (macOS-specific)

`MacOSLocalFileSystem` has `addSearchPath` / `resolveWithSearchPaths` — absent in `Win32LocalFileSystem`. Required because:
- macOS file systems can be case-sensitive
- Loose files are searched first in CWD, then in search paths (ZH, Base)

Search paths are registered in `MacOSLocalFileSystem::init()`.

### Loose File Priority

| # | Where | Example |
|---|-------|---------|
| 1 | CWD (= ZH after chdir) | `./Data/Scripts/foo.scb` |
| 2 | ZH path (search path) | `.../Zero Hour/Data/Scripts/foo.scb` |
| 3 | Base path (search path) | `.../Generals/Data/Scripts/foo.scb` |

### Overall Priority for `openFile`

```
Loose CWD > Loose ZH > Loose Base > BIG CWD > BIG Base
```

## registry.cpp (`#ifdef __APPLE__`)

On Windows, `GetStringFromGeneralsRegistry("", "InstallPath")` returns the Base path from the registry.

On macOS, `getStringFromRegistry` is wrapped with `#ifdef __APPLE__`. For the `InstallPath` key, it returns `basePath` discovered by the detector at startup.

## Core / Platform Duality

Two `StdLocalFileSystem` implementations exist in the project:

| File | Location |
|------|----------|
| `Core/GameEngineDevice/.../StdLocalFileSystem.h/.cpp` | Core (base class) |
| `Platform/MacOS/.../MacOSLocalFileSystem.h/.mm` | Platform (inherits StdLocalFileSystem) |

`MacOSGameEngine::createLocalFileSystem()` returns `NEW MacOSLocalFileSystem`.

> `MacOSLocalFileSystem` inherits `StdLocalFileSystem` and overrides all methods
> with slash normalization and search path logic.

## Case-Insensitivity in .big Archives (ArchiveFileSystem)

The `std::multimap<AsciiString, ArchiveFile*>` used for the virtual file tree relies on `AsciiString::operator<`, which is **strictly case-sensitive** (`strcmp`).

To guarantee Windows-compatible file lookup inside `.big` archives (critical for `MapCache.ini`):
1. During `.big` scanning in `ArchiveFileSystem::loadIntoDirectoryTree`, extracted filenames are **forced to lowercase** (`lowerToken.toLower()`) before insertion into `m_files`
2. Removing `.toLower()` at this step critically breaks `find()` in Clang libc++, since queries often arrive in different case
3. Loading order (shadowing): ZH `.big` files are loaded first, then Base. The `overwrite = FALSE` parameter inserts duplicate keys at the end, preserving correct caching priority

## INIZH.big Duplicate Filtering

In `loadBigFilesFromDirectory`, `INIZH.big` from the `Data/INI/` subdirectory is skipped because many digital distribution versions contain a duplicate, causing CRC conflicts.

## Critical Loose Files

These files are **not packed** into `.big` archives and are found via CWD / search paths:

| File | Purpose |
|------|---------|
| `Data/Scripts/SkirmishScripts.scb` | AI scripts for skirmish (building, attacks) |
| `Data/Scripts/MultiplayerScripts.scb` | Multiplayer scripts |
| `Data/INI/*.ini` | Unit, weapon, building configuration |

> **Caution:** If `SkirmishScripts.scb` is not found, the AI bot in skirmish **does not build or attack**.

## Map Abstraction (VFS, UI, and Network Transfer)

Map loading (`.map`) relies on two independent systems: the cache parser (`INIMapCache.cpp`) and the user interface (`MapUtil.cpp` / GUI codes).

**1. Parsing and the m_isOfficial Flag (Network Transfer):**
During parsing (`MapCache.ini` / `MapCacheGO.ini` creation):
- `loadMapsFromDisk("Maps\\", TRUE)` scans the virtual root (loose files + `.big` contents). These maps get `m_isOfficial = true`.
- `loadMapsFromDisk(UserPath, FALSE)` scans `~/Command and Conquer Generals Zero Hour Data/Maps/`. These maps get `m_isOfficial = false`.

> The `m_isOfficial` flag directly controls map transfer in multiplayer (see `GUIUtil.cpp`: `willTransfer = !mapData->m_isOfficial;`). If a community map is packed into a `.big` archive, it is marked as official and **never transferred** to other players.

**2. UI Rendering (Ignores the Flag):**
The interface lists (Official/Custom tabs in Skirmish/LAN/WOL) do not use `m_isOfficial` for filtering. They use path prefix matching exclusively:
```cpp
const Bool mapOk = mapName.startsWithNoCase(mapDir.str())
```
- Official tab shows everything starting with `"maps\\"`.
- Custom tab shows everything starting with the user data path.
