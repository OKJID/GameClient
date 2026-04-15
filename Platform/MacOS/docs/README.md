# macOS Port — Documentation

> **Command & Conquer: Generals — Zero Hour** on Apple Silicon (ARM64)
>
> Branch: `okji/feat/macos-port` | Native Metal rendering backend

https://github.com/user-attachments/assets/dd032fec-724d-4958-9ea0-d7bac97059ce

## Documents

| Document | Description |
|:---|:---|
| **[Setup Guide](SETUP.md)** | Prerequisites, building, running |
| **[Developer Guide](DEVELOPMENT.md)** | Architecture, rules, pitfalls, debugging |
| **[Rendering Pipeline](RENDERING.md)** | Metal backend, DX8→Metal translation, shaders |
| **[File System](FILE_SYSTEM.md)** | VFS architecture, .big archives, path normalization |
| **[Build System](BUILD_SYSTEM.md)** | CMake structure, dependencies, targets |
| **[Implementation Status](IMPLEMENTATION_STATUS.md)** | Per-component implementation status |

## Quick Start

```bash
# Development (Debug build + run with logging)
sh build_run_mac.sh
sh build_run_mac.sh --clean
sh build_run_mac.sh --screenshot=15
sh build_run_mac.sh --test

# Release (optimized build)
sh build_mac.sh
sh build_mac.sh --launcher            # + SwiftUI Launcher + dylib bundling
sh build_mac.sh --launcher --clean    # clean + full distribution package
```

## Current Status

| Subsystem | Status | Details |
|:---|:---|:---|
| **Build** | ✅ Complete | `GeneralsOnlineZH.app` bundle (ARM64) |
| **Rendering** | ✅ Complete | Full DX8→Metal pipeline: terrain, units, water, particles, UI, fog of war, MSAA |
| **Input** | ✅ Complete | MacOSKeyboard + MacOSMouse via NSEvent |
| **File System** | ✅ Complete | Dual-tier VFS: loose files + .big archives, case-insensitive lookup |
| **Multiplayer** | ✅ Complete | Online play via Generals Online (P2P through GameNetworkingSockets) |
| **CRC / Sync** | ✅ Complete | Deterministic math (fdlibm), cross-platform lockstep parity with Windows |
| **Display** | ✅ Complete | Resolution switching, windowed mode, gamma ramp |
| **Audio** | ✅ Complete | AVAudioEngine backend: 2D/3D playback, WAV loading, 64-source pool, listener positioning |
| **Video** | ⚠️ Skipped | Intro/cutscene videos bypassed (Bink codec not ported) |

## Architecture

### Strategy: Hybrid A+B (DX8Wrapper with Metal implementation)

The original `DX8Wrapper` class name and public API are preserved:
- `#ifndef __APPLE__` — original DX8 implementation (Windows)
- `#ifdef __APPLE__` — Metal implementation with **identical public API**

All consumer code (WW3D2, W3DDevice, ShaderManager) calls `DX8Wrapper::Begin_Scene()`,
`DX8Wrapper::Draw_Triangles()`, etc. — **no includes or call sites need modification**.

### File Structure

```
Platform/MacOS/
├── CMakeLists.txt                     # macOS cmake target
├── Build/
│   ├── Logs/game.log                  # Game log (stdout redirected)
│   └── screenshot.py                  # Screenshot utility
├── Include/                           # Compat headers (d3d8, windows.h, etc.)
├── Source/
│   ├── Main/
│   │   ├── MacOSMain.mm              # Entry point (NSApplication + GameMain)
│   │   ├── MacOSGameEngine.h/.mm     # GameEngine subsystem factory
│   │   └── MacOSDebugLog.h           # DLOG_RFLOW macro
│   ├── Metal/
│   │   ├── dx8wrapper_metal.mm       # DX8Wrapper Metal implementation (~1700 lines)
│   │   ├── MetalDevice8.h/.mm        # IDirect3DDevice8 → Metal (~2900 lines)
│   │   ├── MetalInterface8.h/.mm     # IDirect3D8 → Metal
│   │   ├── MetalTexture8.h/.mm       # IDirect3DTexture8 → MTLTexture
│   │   ├── MetalSurface8.h/.mm       # IDirect3DSurface8 → staging buffer
│   │   ├── MetalVertexBuffer8.h/.mm  # VB → MTLBuffer
│   │   ├── MetalIndexBuffer8.h/.mm   # IB → MTLBuffer
│   │   ├── MacOSDisplayManager.h/.mm # Display mode enumeration
│   │   └── MacOSShaders.metal        # FFP emulation (vertex + fragment)
│   ├── Input/
│   │   ├── MacOSKeyboard.h/.cpp      # NSEvent → Keyboard
│   │   └── MacOSMouse.h/.cpp         # NSEvent → Mouse
│   ├── Audio/
│   │   ├── MacOSAudioManager.h/.cpp      # AudioManager (AVAudioEngine, 64-source pool)
│   │   └── AVAudioBridge.h/.mm           # C bridge → AVAudioEngine/AVAudioPlayerNode
│   ├── FileSystem/
│   │   └── MacOSLocalFileSystem.h/.mm # Path normalization + case-insensitive lookup
│   └── GeneralsOnlineStubs.cpp       # Remaining network stubs (sentry)
├── Launcher/                          # SwiftUI Launcher app
│   ├── Sources/
│   │   ├── LauncherApp.swift         # @main entry point
│   │   └── MainView.swift            # Game path selector + launch button
│   ├── assets/                        # background.png, dir_image.png
│   ├── assemble_distribution.sh      # Full distribution pipeline
│   └── outputs/                       # Final .zip artifact
└── docs/                              # ← You are here
```

## Scope

- **Zero Hour only** (`GeneralsMD/`). Base Generals (`Generals/`) is not supported.
- Tools (WorldBuilder, etc.) are not built on macOS.
- Game data files (`.big`) must be supplied from a Windows installation.

## Core Rules

1. **Shared code** (`Core/`, `GeneralsMD/Code/`) — modify only under `#ifdef __APPLE__`
2. **Platform code** — freely modify in `Platform/MacOS/`
3. **Build and run** — always via `sh build_run_mac.sh`
4. **Mirror the Windows flow** — no workarounds, replicate the tested Windows behavior
5. **Attribution** — all shared code changes carry `// TheSuperHackers` comments per [CONTRIBUTING.md](../../../CONTRIBUTING.md)
