# macOS Port — Developer Guide

---

## Core Rules

1. **Shared code** (`Core/`, `GeneralsMD/Code/`) — modify only under `#ifdef __APPLE__`, never `#ifdef _WIN32`
2. **Platform code** — freely modify in `Platform/MacOS/`
3. **Mirror the Windows flow** — replicate tested Windows behavior, not workarounds
4. **Build and run** — always via `sh build_run_mac.sh`
5. **Logging** — `printf` + `fflush(stdout)`. Never `fprintf(stderr)` (stdout is redirected to game.log)
6. **DLOG_RFLOW(level, fmt, ...)** — categorized logs for Metal backend
7. **Mark workarounds** with `TODO(PS_PATH):` and a description
8. **Scope** — Zero Hour only (`GeneralsMD/`). Base Generals is not supported
9. **Attribution** — all shared code changes carry `// TheSuperHackers @keyword` comments

---

## Architecture

### Strategy: Hybrid A+B

`DX8Wrapper` retains its original class name and API:
- `#ifndef __APPLE__` → original DX8 implementation (Windows)
- `#ifdef __APPLE__` → Metal implementation in `dx8wrapper_metal.mm`

All WW3D2 consumer code (152 files) remains **unmodified**.

### Components

| Subsystem | Files | Purpose |
|:---|:---|:---|
| **Metal Backend** | `MetalDevice8.mm` (~2900 lines) + 5 .h/.mm pairs | DX8 → Metal bridge |
| **DX8Wrapper Metal** | `dx8wrapper_metal.mm` (~1700 lines) | Static class with render state cache |
| **Entry Point** | `MacOSMain.mm` | NSApplication, GameMain, CreateGameEngine |
| **Game Engine** | `MacOSGameEngine.mm` | Subsystem factory |
| **Input** | `MacOSKeyboard.cpp`, `MacOSMouse.cpp` | NSEvent → game input |
| **Audio** | `MacOSAudioManager.cpp` + `AVAudioBridge.mm` | AVAudioEngine backend (2D/3D, 64-source pool) |
| **File System** | `MacOSLocalFileSystem.mm` | Path normalization + case-insensitive lookup |
| **Display** | `MacOSDisplayManager.mm` | Resolution enumeration and switching |
| **Shaders** | `MacOSShaders.metal` | FFP emulation (vertex + fragment) |
| **Compat Headers** | `Include/windows.h`, `d3d8*.h`, etc. | Win32/D3D type stubs + path interceptors |

### GameEngine Factory

```cpp
class MacOSGameEngine : public GameEngine {
    GameLogic*         createGameLogic()         → W3DGameLogic
    GameClient*        createGameClient()        → W3DGameClient
    ModuleFactory*     createModuleFactory()     → W3DModuleFactory
    LocalFileSystem*   createLocalFileSystem()   → MacOSLocalFileSystem
    ArchiveFileSystem* createArchiveFileSystem() → StdBIGFileSystem
    AudioManager*      createAudioManager()      → MacOSAudioManager  // AVAudioEngine
    Network*           createNetwork()           → NetworkInterface::createNetwork
    WebBrowser*        createWebBrowser()        → nullptr
};
```

---

## Pitfalls

### 1. DX8Wrapper: Deferred State Application

`Set_Transform(WORLD/VIEW)` does NOT call `D3DDevice->SetTransform` immediately. Matrices are stored in `render_state.world/view` and applied in `Apply_Render_State_Changes()` before each `Draw()`.

**Critical:** If a function like `Set_World_Identity()` is an empty stub, `render_state.world` remains zeroed → black screen.

### 2. D3D → Metal Matrix Convention

D3D stores matrices in row-major order. `memcpy` into Metal `float4x4` (column-major) effectively **transposes** the matrix. The shader uses `P * V * W * pos`, which is equivalent to D3D's `pos * W * V * P`.

### 3. NSApplication + dispatch_async

`[NSApp run]` starts the event loop. `dispatch_async(main_queue)` enqueues the game loop. The game loop blocks the main queue (infinite loop). `serviceWindowsOS()` manually pumps events via `[NSApp nextEventMatchingMask:]`. `[CATransaction flush]` is required for window updates.

### 4. File System Scans CWD

The game launches from the source code root. `StdLocalFileSystem` scans `.` = thousands of files. The working directory must be set to the ZH data folder via `chdir()` in `MacOSGameEngine::init()`.

### 5. FVF Stride vs Offset

`GetPSO` uses stride from the calling code, NOT computed from FVF flags. C++ structs may have padding that differs from the sum of FVF attribute sizes.

### 6. Half-Pixel Offset (DX8 vs Metal)

DX8 requires a -0.5px geometry bias for pixel-perfect 2D texel sampling. Metal handles pixel centers correctly at +0.5. The bias is disabled on macOS via `#ifndef __APPLE__` in `render2d.cpp` to prevent shearing artifacts in UI and fonts.

---

## 64-bit Compatibility Fixes

### Struct Padding (LP64 Data Model)

On macOS (LP64), `long` is 8 bytes and `void*` is 8 bytes, unlike Windows ILP32 where both are 4 bytes. Several binary file parsers relied on exact struct sizes matching file format headers:

| File Format | Struct | Problem | Fix |
|:---|:---|:---|:---|
| DDS | `LegacyDDSURFACEDESC2` | `void* Surface` → 132 bytes instead of 124 | Replace with `unsigned int` under `#ifdef __APPLE__` |
| TGA | `TGA2Footer`, `TGA2Extension` | `long` offsets → 34 bytes instead of 26 | Replace with `int` under `#ifdef __APPLE__` |

### Enum Bitmask Undefined Behavior

The death/veterancy system uses `1 << enumValue` bitmask mapping. When `enumValue == 0` (DEATH_NORMAL, LEVEL_REGULAR), the bitmask is `1 << 0 = 1`, which maps correctly on 32-bit but causes UB on 64-bit due to sign extension in `BitFlags<>` template. Fixed with explicit bitmask tables in `Damage.h` and `GameCommon.h`.

### Drawable Lifetime

`Drawable::drawUIText` could access parent `Object` or `Owner` pointers after the parent was destroyed during the same frame. Fixed with null checks guarded by `#ifdef __APPLE__`.

---

## Multiplayer Architecture

### Deterministic Math (Cross-Platform Lockstep)

The game uses lockstep networking — all clients compute game logic in parallel. Any floating-point divergence causes a desync.

**Problem:** Windows x86 uses 80-bit FPU (`fsin`, `fcos` asm), macOS ARM uses 32-bit NEON/SSE via `sinf()`/`cosf()`. Results differ at the least significant bit.

**Solution:** All trigonometric functions in `WWMath` are replaced with Sun's `fdlibm` (Freely Distributable LIBM) — a pure C implementation that produces bit-identical IEEE 754 results on all platforms.

| Function | Old (Windows) | Old (macOS) | New (both) |
|:---|:---|:---|:---|
| Sin, Cos | x87 `fsin`/`fcos` asm | `sinf()`/`cosf()` | `fdlibm_sin()`/`fdlibm_cos()` |
| Sqrt | x87 `fsqrt` asm | `sqrt()` | `fdlibm_sqrt()` |
| Acos, Asin, Atan, Atan2 | system libm | system libm | `fdlibm_*()` |
| Inv_Sqrt | asm Newton-Raphson | `1.0f/sqrt()` | Portable Quake `0x5f3759df` hack |
| Fast_Sin, Fast_Cos | LUT tables | LUT tables | Unchanged (already deterministic) |

### CRC Verification

The engine computes CRC checksums every `NET_CRC_INTERVAL` frames (100 in release, 1 in debug). Each player's CRC is compared — any mismatch triggers a desync error.

The macOS client reports its executable CRC via a server-side version manifest that provides Windows-compatible CRC values, ensuring the P2P handshake succeeds.

### Online Services (Generals Online)

~85% of the NGMP (Next-Gen Multiplayer) code is pure C++/STL/libcurl and works cross-platform without changes. Platform-specific replacements:

| Win32 API | macOS Replacement | Location |
|:---|:---|:---|
| `ShellExecuteA("open", url)` | `system("open <url>")` | Auth, Init |
| `CryptProtectData/Unprotect` | File-based storage | Auth |
| `GetModuleFileName` | `_NSGetExecutablePath` | Init |
| `LoadLibraryA/GetProcAddress` | `dlopen/dlsym` | Steam init |

---

## Map System

### Map Discovery Chain

1. `MapCache::updateCache` checks for `Maps\\MapCache.ini` via `TheFileSystem`
2. If found (inside `.big` archives), all official maps are loaded from cache — no disk scan
3. Custom maps are scanned from `~/Command and Conquer Generals Zero Hour Data/Maps/`
4. The `m_isOfficial` flag controls P2P map transfer: official maps are never transferred

### Lobby Map Resolution (Generals Online)

The server sends a raw map name. The client resolves it:
1. **Official maps**: prepends `maps\` prefix
2. **Custom maps**: searches `TheMapCache` by filename (`strcasecmp`) → returns full VFS path
3. **Not found**: raw path remains, `has_map=false` is reported to the server

### Accept Button Logic

The `hasMap()` value on each lobby slot is determined by the **server** (via polling), not the local cache. The server receives `has_map` from the client's `UpdateCurrentLobby_HasMap()`, which calls `findMap()`. If the map path is malformed (missing prefix, wrong slashes), the chain fails and Accept remains disabled.

---

## Build and Run

```bash
sh build_run_mac.sh                  # build + run
sh build_run_mac.sh --clean          # full clean rebuild
sh build_run_mac.sh --screenshot=N   # screenshot after N seconds
sh build_run_mac.sh --test           # Metal bridge tests
sh build_run_mac.sh --lldb           # run under debugger
```

### Log Files

| File | Content | Activation |
|:---|:---|:---|
| `Platform/MacOS/Build/Logs/game.log` | Game stdout (`printf`, `DEBUG_INFO_MAC`) | Always (stdout → pipe) |
| `~/Command and Conquer Generals Zero Hour Data/\GeneralsOnlineData\GeneralsOnline.log` | `NetworkLog()` — HTTP, ICE, mesh | Always (⚠ backslash in filename) |
| `CRCLogs/DebugFrame_*.txt` | Per-frame CRC dump | `--saveDebugCRCPerFrame ./CRCLogs` |
| `Platform/MacOS/Build/Logs/screenshot_game_window.png` | Screenshot | `--screenshot=N` |

> **⚠ GeneralsOnline.log — backslash path quirk:**
> `NetworkLog` constructs the path with Windows-style backslashes.
> On macOS, `\` is not a path separator — the file is created with **literal backslashes in its name**.
> Path: `~/Command and Conquer Generals Zero Hour Data/\GeneralsOnlineData\GeneralsOnline.log`
>
> To read in terminal:
> ```bash
> cat ~/Command\ and\ Conquer\ Generals\ Zero\ Hour\ Data/\\GeneralsOnlineData\\GeneralsOnline.log
> ```

---

## DEBUG_INFO_MAC — Diagnostic System

### Activation

```bash
export GENERALS_MAC_DEBUG=1
```

The `DEBUG_INFO_MAC((fmt, ...))` macro is defined in `Core/GameEngine/Include/Common/Debug.h`.
On macOS: `printf("[DEBUG_INFO_MAC] " fmt "\n"); fflush(stdout)` → goes to `game.log`.
On non-Apple: `((void)0)` — no-op. **No `#ifdef __APPLE__` needed at call sites.**

### Tags by Subsystem

#### Network Lobby and Map Transfer

| Tag | File | What it logs |
|:---|:---|:---|
| `[ROOM_DATA]` | `OnlineServices_LobbyInterface.cpp` | Map path correction, member parsing, SyncWithLobby calls |
| `[SYNC_LOBBY]` | `NGMPGame.cpp` | Map resolution: OFFICIAL / CUSTOM / FALLBACK with paths |
| `[SLOT_SYNC]` | `NGMPGame.cpp` | Each slot during sync (uid/hasMap/name) |
| `[GAME_START]` | `WOLGameSetupMenu.cpp` | All slots before/after `*TheNGMPGame = *myGame` |
| `[START_GAME]` | `NGMPGame.cpp` | Entry into `startGame()`, all human slots |
| `[LAUNCH]` | `NGMPGame.cpp` | All slots before `DoAnyMapTransfers`, result, BAIL |
| `[MAP_XFER]` | `FileTransfer.cpp` | Each slot in mask-loop, final mask |

#### CRC / Out-of-Sync

| Tag | File | What it logs |
|:---|:---|:---|
| `[CRC_CHECK]` | `GameLogic.cpp` | Validator CRC, each player CRC, MISMATCH/ok |

#### Rendering

| Tag | File | What it logs |
|:---|:---|:---|
| `[DIAG]` | Metal backend | Present, BeginScene, DrawCalls, matrices, viewport, TSS |

---

## Per-Frame CRC Debug

Enabled via CLI flag in `build_run_mac.sh`:
```
-saveDebugCRCPerFrame /path/to/CRCLogs
```

Generates `DebugFrame_NNNN.txt` for each frame with full state dump.
`NET_CRC_INTERVAL` = 100 (release) / 1 (debug) — CRC comparison interval between clients.

---

## NetworkLog

`NetworkLog(ELogVerbosity, fmt, ...)` is defined in `NGMP_Helpers.cpp`.
Writes to `GeneralsOnline.log` (see backslash quirk above).

### Verbosity Levels

| Level | When |
|:---|:---|
| `LOG_RELEASE` | Always (errors, packet drops, disconnects) |
| `LOG_DEBUG` | Only when `Debug_VerboseLogging()` is true |

### Coverage

- HTTP requests/responses to `api.playgenerals.online`
- ICE/P2P mesh — connect, disconnect, signaling
- Game packet send/recv — sizes, drop reasons, buffer overflow
- Lobby sync polling
- `[PRESEED]` — latency seeding
