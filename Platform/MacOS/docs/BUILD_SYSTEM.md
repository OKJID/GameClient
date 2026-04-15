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
  │  zlib (APPLE) ─────────┼──► System zlib            │
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

| Library | Linking | Purpose |
|:---|:---|:---|
| `libcurl` | Dynamic (`.dylib`) | HTTP/WebSocket for Generals Online |
| `GameNetworkingSockets` | Dynamic (`.dylib`) | P2P networking (Valve) |
| `libsodium` | Dynamic (`.dylib`) | Cryptography (GNS dependency) |
| `protobuf` | Dynamic (`.dylib`) | Serialization (GNS dependency) |
| `OpenSSL` | Dynamic (`.dylib`) | TLS (GNS dependency) |
| `nlohmann/json` | Header-only | JSON parsing |
| `stb_image_write` | Header-only | Screenshot capture |
| `miniupnpc` | Source | UPnP port forwarding |

> **Note:** Dynamic libraries must be bundled into `GeneralsOnlineZH.app/Contents/Frameworks/`
> for distribution. Use `dylibbundler` or equivalent to rewrite `@rpath` references.

---

## Preset Configuration

`CMakePresets.json` defines the `macos` preset:

```json
{
  "name": "macos",
  "generator": "Ninja",
  "binaryDir": "build/macos",
  "cacheVariables": {
    "CMAKE_BUILD_TYPE": "Debug",
    "CMAKE_OSX_ARCHITECTURES": "arm64"
  }
}
```

---

## Environment Variables

| Variable | Description | Default |
|:---|:---|:---|
| `GENERALS_INSTALL_PATH` | Root path to game installation | (required) |
| `GENERALS_FPS_LIMIT` | FPS cap | 60 |
| `GENERALS_MSAA` | MSAA sample count | 1 (off) |
| `GENERALS_MAC_DEBUG` | Enable `DEBUG_INFO_MAC` logging | 0 (off) |
