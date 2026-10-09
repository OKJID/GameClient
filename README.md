![Platform](https://img.shields.io/badge/platform-macOS%2013%2B%20Apple%20Silicon-000000?style=flat&logo=apple&logoColor=white)
![Renderer](https://img.shields.io/badge/renderer-Metal-8E44AD?style=flat&logo=apple&logoColor=white)
![Version](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fgeneral-online-zh.web.app%2Fapi%2Fupdate.json&query=%24.version&label=version&color=blue&style=flat)
![Online](https://img.shields.io/badge/online-macOS↔macOS%20playable-brightgreen?style=flat)
![Price](https://img.shields.io/badge/price-free-brightgreen?style=flat)

# Play C&C Generals Zero Hour on Mac — Generals Online macOS Port

**Native Apple Silicon port of Command & Conquer: Generals and Zero Hour with Generals Online multiplayer, a direct Metal renderer and a one-click launcher.**

Not Wine, not CrossOver, not Parallels, not an emulator: the game engine is recompiled for ARM64 macOS with its own DirectX 8 → Metal translation layer.

| | |
|:---|:---|
| 🌐 **Website** | [general-online-zh.web.app](https://general-online-zh.web.app/) — install guide in 12 languages, FAQ, mod list |
| 🎮 **About the project** | [generals-online-mac.github.io](https://generals-online-mac.github.io/) — what the port is, in plain words, with screenshots |
| 📥 **Download** | [Latest Generals Online for macOS](https://general-online-zh.web.app/download) |
| 🎬 **YouTube** | [Install guide, 5–10 minutes](https://youtu.be/Kxk3NJ307mY) · [Channel: Metal gameplay, 8-AI stress test](https://www.youtube.com/@okjid) |
| 💬 **Find players** | [Telegram chat](https://t.me/GeneralsOnlineMacOS) · [Discord](https://discord.gg/Mm3yjnv6V) |
| 📣 **News** | [Telegram channel](https://t.me/GeneralsOnlineMacOSChannel) |

The game data is not shipped with the port: you need your own copy of C&C Generals Zero Hour, for example from [Steam](https://store.steampowered.com/bundle/39394).

---

## Current status

| Area | Status |
|:---|:---:|
| Native Apple Silicon (ARM64) build, macOS 13+ | ✅ Working |
| Direct Metal rendering backend | ✅ Working |
| Zero Hour and classic Generals: skirmish, campaign, saves | ✅ Working |
| Online multiplayer (macOS ↔ macOS) | ✅ Working |
| Online multiplayer (macOS ↔ Windows) | 🔄 In review — [PR #2670](https://github.com/TheSuperHackers/GeneralsGameCode/pull/2670) |
| Mods installed from the launcher | ✅ Working |
| Native macOS audio (AVAudioEngine) | ✅ Working |
| macOS launcher (SwiftUI) with Steam download | ✅ Working |
| Replay compatibility (macOS ↔ macOS) | ✅ Working |
| Replay compatibility (from Windows) | 🔄 Requires deterministic math |

---

## How this differs from GeneralsX

[GeneralsX](https://github.com/fbraz3/GeneralsX) by fbraz3 is a separate project with a different goal — one codebase for every desktop OS:

| | This Port | GeneralsX |
|:---|:---|:---|
| **Target platform** | macOS Apple Silicon (native) | Linux, macOS, Windows (cross-platform) |
| **Rendering** | Custom DX8 → Metal bridge | DXVK (DX8 → Vulkan → MoltenVK on macOS) |
| **Windowing** | Native macOS (Cocoa) | SDL3 |
| **Audio** | AVAudioEngine (native macOS) | OpenAL |
| **App Size & Overhead** | **~12 MB** (Native APIs, no wrappers) | **~78 MB** (Bundled DXVK, MoltenVK, SDL3) |
| **Online** | Official Generals Online servers ([playgenerals.online](https://www.playgenerals.online/)), shared with the main community | Own server, a fork of the Generals Online server, for GeneralsX builds |
| **Cross-platform play** | macOS ↔ macOS today, Windows once deterministic math lands upstream | macOS, Linux and Windows GeneralsX builds play together |
| **Codebase** | Fork of Generals Online (GOD Team) | Fork of TheSuperHackers, with the Generals Online network protocol integrated by hand |

Both projects keep C&C Generals alive on modern platforms and take different routes to it:

- **This Port** focuses on a first-class **native macOS** experience: writing directly to Metal, Cocoa and AVAudioEngine leaves no translation layer between the game and Apple Silicon.
- **GeneralsX** targets broad **cross-platform** compatibility: a separate backend for each operating system would be a huge effort, so DXVK serves as one rendering path for every OS.

The long-term hope on both sides is the same: once deterministic math is part of Generals Online, every port can play in one shared player pool instead of splitting the community across separate servers.

---

## Building from source

### Prerequisites
- Apple Silicon Mac (M1 or newer)
- macOS 13 (Ventura) or later
- Xcode Command Line Tools (`xcode-select --install`)
- Original C&C Generals Zero Hour game files

### Build & Run
The port lives in the [`okji/feat/macos-port`](https://github.com/OKJID/GameClient/tree/okji/feat/macos-port) branch.

```bash
git clone -b okji/feat/macos-port https://github.com/OKJID/GameClient.git
cd GameClient
sh build_run_mac.sh          # Build and launch
sh build_run_mac.sh --clean  # Clean rebuild
```

### Documentation

| Document | Description |
|:---|:---|
| [Platform/MacOS/docs/README.md](https://github.com/OKJID/GameClient/blob/okji/feat/macos-port/Platform/MacOS/docs/README.md) | macOS port architecture overview |
| [Platform/MacOS/docs/SETUP.md](https://github.com/OKJID/GameClient/blob/okji/feat/macos-port/Platform/MacOS/docs/SETUP.md) | Build setup and prerequisites |
| [Platform/MacOS/docs/RENDERING.md](https://github.com/OKJID/GameClient/blob/okji/feat/macos-port/Platform/MacOS/docs/RENDERING.md) | DX8 → Metal rendering pipeline |
| [Platform/MacOS/docs/FILE_SYSTEM.md](https://github.com/OKJID/GameClient/blob/okji/feat/macos-port/Platform/MacOS/docs/FILE_SYSTEM.md) | File system and resource resolution |
| [Platform/MacOS/docs/DEVELOPMENT.md](https://github.com/OKJID/GameClient/blob/okji/feat/macos-port/Platform/MacOS/docs/DEVELOPMENT.md) | Development guidelines |
| [Platform/MacOS/docs/BUILD_SYSTEM.md](https://github.com/OKJID/GameClient/blob/okji/feat/macos-port/Platform/MacOS/docs/BUILD_SYSTEM.md) | CMake build system details |
| [Platform/MacOS/docs/IMPLEMENTATION_STATUS.md](https://github.com/OKJID/GameClient/blob/okji/feat/macos-port/Platform/MacOS/docs/IMPLEMENTATION_STATUS.md) | Detailed implementation status |

---

## Upstream

```
EA — the original game (2003), source code released under GPL-3.0
└── TheSuperHackers/GeneralsGameCode              the community codebase: Generals and Zero Hour
    ├── GeneralsOnlineDevelopmentTeam/GameClient  the fast track behind the Generals Online service
    │   └── OKJID/GameClient                      this macOS port
    └── Okladnoj/GeneralsGameCode                 deterministic math
```

- [TheSuperHackers](https://github.com/TheSuperHackers/GeneralsGameCode) and the [Generals Online Development Team](https://github.com/GeneralsOnlineDevelopmentTeam/GameClient) are one Windows community working on two tracks. TheSuperHackers keep the full codebase and move carefully; the GOD Team is the fast track that launched the Generals Online service and regularly pulls from TheSuperHackers.
- This macOS port is an independent effort, not part of either team. It forks the GOD Team client because the online service lives there, and sends its shared changes back to both projects as pull requests, for example [GameClient #457](https://github.com/GeneralsOnlineDevelopmentTeam/GameClient/pull/457).
- Deterministic math is developed in [Okladnoj/GeneralsGameCode](https://github.com/Okladnoj/GeneralsGameCode) and contributed to TheSuperHackers as [#2670](https://github.com/TheSuperHackers/GeneralsGameCode/pull/2670). Once it lands, the two tracks are expected to converge, and Windows and Mac players share one simulation.

## Contributing

Contributions are welcome! If you're interested in helping with the macOS port — especially in areas like deterministic math, Metal rendering, or audio — join the discussion in the pull requests above or open an issue.

Please read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting a pull request.

## License & Legal Disclaimer

EA has not endorsed and does not support this product. All trademarks are the property of their respective owners.

This project is licensed under the [GPL-3.0 License](https://www.gnu.org/licenses/gpl-3.0.html). See [LICENSE.md](LICENSE.md) for details.

<!-- Published from src/readme.md of Okladnoj/general_online_zh by "sh deploy.sh readme" — edit it there, not here. -->
