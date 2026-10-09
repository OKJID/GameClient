# macOS Port — Build System

---

## Build Commands

```bash
# Development (Debug + run):
sh build_run_mac.sh              # configure + build + run
sh build_run_mac.sh --clean      # clean + configure + build + run
sh build_run_mac.sh --test       # build + run tests

# Release (optimized):
sh build_mac.sh                  # release build only
sh build_mac.sh --launcher       # release + Launcher + dylib bundling + .zip
sh build_mac.sh --launcher --clean  # clean + full distribution package

# Manual build (not recommended):
cmake --preset macos
cmake --build build/macos
```

---

## Dependency Graph

```
                    CMakeLists.txt (root)
                          │
  ┌───────────────────────┼───────────────────────────┐
  │     DEPENDENCIES      │                           │
  │                       │                           │
  │  DX8 (APPLE) ─────────┼──► Platform/MacOS/Include │
  │    d3d8lib INTERFACE   │    d3d8_interfaces.h      │
  │    d3d8, d3dx8 empty   │    MetalDevice8 impl      │
  │                       │                           │
  │  GameSpy (APPLE) ──────┼──► INTERFACE only         │
  │  Miles (APPLE) ────────┼──► milesstub INTERFACE    │
  │  Bink (APPLE) ─────────┼──► binkstub INTERFACE     │
  │  Win32 libs (APPLE) ───┼──► INTERFACE dummies      │
  │  zlib (APPLE) ─────────┼──► vcpkg zlib             │
  │                       │                           │
  ├───────────────────────┼───────────────────────────┤
  │     TARGETS           │                           │
  │                       │                           │
  │  macos_platform ───────┼──► Platform/MacOS/        │
  │    STATIC library      │    MetalDevice8, MacOSMain│
  │    Links: Metal, AppKit│    Input, Audio, Stubs    │
  │      QuartzCore        │                           │
  │                       │                           │
  │  GeneralsOnlineZH ─────┼──► .app bundle            │
  │    Links: z_gameengine │                           │
  │      z_gameenginedevice│                           │
  │      macos_platform    │                           │
  │                       │                           │
  └───────────────────────┴───────────────────────────┘
```

---

## Key CMake Targets

| Target | Type | Output | Description |
|:---|:---|:---|:---|
| `macos_platform` | STATIC | `libmacos_platform.a` | All macOS-specific code |
| `GeneralsOnlineZH` | EXECUTABLE | `.app` bundle | Zero Hour application |
| `z_gameengine` | STATIC | `libz_gameengine.a` | ZH engine library |
| `z_gameenginedevice` | STATIC | `libz_gameenginedevice.a` | ZH device library |
| `metal_bridge_tests` | EXECUTABLE | `Tests/metal_bridge_tests` | Metal bridge unit tests |
| `GeneralsLauncher` | SwiftUI (swiftc) | Inside `.app` bundle | Game data folder picker + launcher (compiled by `assemble_distribution.sh`, not CMake) |

---

## macOS Framework Dependencies

| Framework | Purpose |
|:---|:---|
| `Metal` | GPU rendering |
| `MetalKit` | Metal utilities |
| `AppKit` | Window management, events |
| `QuartzCore` | `CAMetalLayer` |
| `CoreGraphics` | Gamma ramp (`CGSetDisplayTransferByTable`) |
| `AVFoundation` | Audio (planned, not yet linked) |

---

## Vendor Libraries

Third-party libraries come from vcpkg (`vcpkg.json`), the same manifest the Windows build uses.

| Library | Source | Linking | Purpose |
|:---|:---|:---|:---|
| `curl` | vcpkg | Static | HTTP/WebSocket for Generals Online |
| `GameNetworkingSockets` | vcpkg, `tintinhamans/GNS` registry | Static | P2P networking (ICE + WebRTC) |
| `sentry-native` | vcpkg | Static | Error and crash reporting |
| `zlib`, `stb` | vcpkg | Static / header-only | Compression, screenshot capture |
| FFmpeg (Bink only) | `cmake/ffmpeg_bink.cmake` | Static | Bink video decoding; vcpkg's `ffmpeg` is excluded on macOS |
| `nlohmann/json` | In tree | Header-only | JSON parsing |
| `miniupnpc` | In tree | Source | UPnP port forwarding |

---

## Preset Configuration

The `macos` preset in `CMakePresets.json` builds Release with Ninja into `build/macos`, using the vcpkg toolchain
(`$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake`) and the overlay triplet `triplets/arm64-osx-static.cmake`:
static arm64 libraries built for the game's minimum macOS version.

---

## Environment Variables

| Variable | Description | Default |
|:---|:---|:---|
| `VCPKG_ROOT` | vcpkg checkout used by the `macos` preset | (required) |
| `GENERALS_INSTALL_PATH` | Root path to game installation | (required) |
| `GENERALS_FPS_LIMIT` | FPS cap | 60 |
| `GENERALS_MSAA` | MSAA sample count | 1 (off) |
| `GENERALS_MAC_DEBUG` | Enable `DEBUG_INFO_MAC` logging | 0 (off) |
