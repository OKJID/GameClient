# macOS Port — Detailed Status

Detailed status of the native macOS Apple Silicon port of C&C Generals Zero Hour / Generals Online.

---

## ✅ Working

- **Native ARM64 build** — Compiles and runs natively on Apple Silicon (M1/M2/M3/M4)
- **Direct Metal renderer** — Custom DX8 → Metal translation layer with MSAA, alpha blending, cubemaps, clip planes
- **Singleplayer** — Skirmish and Campaign fully playable
- **AI** — All AI difficulties working (including Brutal)
- **Online multiplayer (macOS ↔ macOS)** — Working via Generals Online servers (P2P via GameNetworkingSockets)
- **Native audio** — AVAudioEngine-based audio system replacing Miles Sound System
- **File system** — NativeFileSystem facade handling path translation (backslash ↔ forward slash)
- **Input** — Native macOS keyboard and mouse input
- **macOS Launcher** — SwiftUI-based launcher with SteamCMD integration
- **Water rendering** — Transparent water with Z-bias fix for banding artifacts
- **Fog of War / Shroud** — Working on minimap and terrain
- **UI / HUD** — Adapted for macOS display scaling and resolution switching

## 🔄 In Progress

- **Cross-platform multiplayer (macOS ↔ Windows)** — Requires deterministic math parity between ARM64 NEON and x87 FPU. CRC mismatches occur due to floating-point precision differences. See [DETERMINISTIC_MATH.md](DETERMINISTIC_MATH.md).
- **Replay compatibility (from Windows)** — Windows replays require deterministic math parity (ARM64 vs x87 FPU). macOS ↔ macOS replays work correctly.

## ⚠️ Known Limitations

- **`std::chrono::utc_clock`** — Not available in Apple libc++. Frame pacing uses `system_clock` on macOS (functionally equivalent for this use case).
- **`std::format` edge cases** — Some C++20 `<format>` features may behave differently on Apple Clang vs MSVC.
- **`localtime_s`** — MSVC extension, replaced with POSIX `localtime_r` on macOS via `#ifdef __APPLE__`.
- **Windows-only features** — Patcher/updater (`ShellExecuteA`, `CryptProtectData`) stubbed or replaced with macOS equivalents.
- **Intel Mac** — Not tested. ARM64 only.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────┐
│                  Game Engine                     │
│          (C++20, platform-agnostic)              │
├─────────────────────────────────────────────────┤
│  Platform/MacOS/                                │
│  ├── Metal/        DX8 → Metal bridge           │
│  ├── Audio/        AVAudioEngine bridge         │
│  ├── Input/        Native keyboard/mouse        │
│  ├── System/       MacOSLocalFileSystem          │
│  └── Main/         MacOSGameEngine entry point  │
├─────────────────────────────────────────────────┤
│  Common/System/NativeFileSystem                  │
│  (Cross-platform path normalization facade)     │
└─────────────────────────────────────────────────┘
```

For full architecture details, see [Platform/MacOS/docs/README.md](../Platform/MacOS/docs/README.md).
