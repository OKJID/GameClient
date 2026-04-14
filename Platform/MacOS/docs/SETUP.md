# macOS Port — Setup Guide

## Prerequisites

| Requirement | Version | Note |
|:---|:---|:---|
| **macOS** | 13+ (Ventura) | Apple Silicon (ARM64) |
| **Xcode Command Line Tools** | Latest | `xcode-select --install` |
| **CMake** | 3.25+ | `brew install cmake` |
| **Ninja** | Latest | `brew install ninja` |
| **dylibbundler** | Latest | `brew install dylibbundler` (required for `--launcher`) |
| **Game Data** | Generals: Zero Hour | `.big` files from a Windows installation |

## Building

### 1. Clone

```bash
git clone https://github.com/OKJID/GameClient.git
cd GameClient
git checkout okji/feat/macos-port
```

### 2. Build and Run

```bash
sh build_run_mac.sh
```

This automatically:
- Configures CMake (preset `macos`, Ninja, Debug, ARM64)
- Builds the project
- Launches the game with log redirection

### 3. Game Data

Set the `GENERALS_INSTALL_PATH` variable in `build_run_mac.sh`:

```bash
export GENERALS_INSTALL_PATH="/path/to/Command and Conquer - Generals"
```

The directory must contain two subdirectories — one for Zero Hour (identified by `INIZH.big`) and one for base Generals (identified by `INI.big`). Exact folder names are auto-detected.

Expected structure:
```
Command and Conquer - Generals/
├── Command and Conquer Generals Zero Hour/   ← ZH (marker: INIZH.big)
│   ├── *.big
│   └── Data/Scripts/SkirmishScripts.scb
└── Command and Conquer Generals/             ← Base (marker: INI.big, no INIZH.big)
    ├── *.big
    └── Data/...
```

## Commands

### Development (`build_run_mac.sh`)

| Command | Description |
|:---|:---|
| `sh build_run_mac.sh` | Debug build + run with log redirection |
| `sh build_run_mac.sh --clean` | Full clean rebuild + run |
| `sh build_run_mac.sh --screenshot=N` | Screenshot after N seconds |
| `sh build_run_mac.sh --test` | Run Metal bridge tests |
| `sh build_run_mac.sh --lldb` | Run under debugger |

Flags can be combined: `sh build_run_mac.sh --clean --screenshot`

### Release (`build_mac.sh`)

| Command | Description |
|:---|:---|
| `sh build_mac.sh` | Release build only |
| `sh build_mac.sh --launcher` | Release + compile Launcher + bundle dylibs + create `.zip` |
| `sh build_mac.sh --launcher --clean` | Clean + full distribution package |

## Launcher & Distribution

The **Launcher** is a SwiftUI app (`Platform/MacOS/Launcher/`) that replaces `GeneralsOnlineZH` as the `.app` entry point. It:
1. Presents a folder picker for the user to select the game data directory
2. Validates the selection (checks for `INIZH.big` + `INI.big` markers)
3. Persists the path to `UserDefaults`
4. Launches `GeneralsOnlineZH` with `GENERALS_INSTALL_PATH` set in the environment
5. Terminates itself after launch

### Distribution Pipeline (`assemble_distribution.sh`)

Run via `sh build_mac.sh --launcher`. Performs 6 steps:

1. **Bundle dylibs** — `dylibbundler` copies all Homebrew `.dylib` deps into `Contents/Frameworks/` and rewrites `@rpath` to `@executable_path/../Frameworks/`
2. **Clean RPATHs** — removes all stale rpath entries, adds the Frameworks path
3. **Compile Launcher** — `swiftc` compiles `LauncherApp.swift` + `MainView.swift` into the app bundle
4. **Inject assets** — `background.png`, `dir_image.png`, `AppIcon.png` → `Contents/Resources/`; patches `Info.plist` to set `CFBundleExecutable` to `GeneralsLauncher`
5. **Generate README** — creates `README_INSTALL.md` with Gatekeeper bypass instructions (`xattr -cr`)
6. **Create ZIP** — packages `Generals Online.app` + README into `outputs/Generals_Online_Mac_Alpha.zip`

Final output: `Platform/MacOS/Launcher/outputs/Generals_Online_Mac_Alpha.zip`

## Logging

Game logs are written to `Platform/MacOS/Build/Logs/game.log`.

Useful patterns in the log:
- `[DIAG]` — rendering diagnostic messages
- `[RFLOW:N]` — render flow (levels 1-17)
- `[MetalDevice8]` — Metal initialization
- `StdLocalFileSystem` — file system operations

## Troubleshooting

### Game does not launch
- Verify `.big` files exist and `GENERALS_INSTALL_PATH` is correct
- Kill stale processes: `killall GeneralsOnlineZH`
- Clean rebuild: `sh build_run_mac.sh --clean`

### Black screen
- Check `game.log` for `[MetalDevice8] Initialized:`
- Verify `[DIAG] Present frame=N drawCalls=N` — drawCalls must be > 0
- Verify `[DIAG] BindUniforms World:` — world matrix must not be all zeros

### Magenta/purple screen
- Missing textures. Check `[MetalDevice8::CreateTexture]` entries in log
- Verify formats: `fmt=21` (A8R8G8B8), `fmt=26` (A4R4G4B4)

### No audio for specific sounds
- Only PCM WAV format is supported. ADPCM-encoded `.wav` files are silently skipped
- Music streaming (MP3) is not yet supported — music tracks are silent
- Check `game.log` for `loadAudioBuffer: WAV parse failed` entries
