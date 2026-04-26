![Platform](https://img.shields.io/badge/platform-macOS%20Apple%20Silicon-000000?style=flat&logo=apple&logoColor=white)
![Renderer](https://img.shields.io/badge/renderer-Metal-8E44AD?style=flat&logo=apple&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=flat&logo=cplusplus&logoColor=white)
![Status](https://img.shields.io/badge/status-Technical%20Preview-orange?style=flat)
![Online](https://img.shields.io/badge/online-macOS↔macOS%20playable-brightgreen?style=flat)

# C&C Generals Online — macOS Port

🌐 **Website:** [general-online-zh.web.app](https://general-online-zh.web.app/)

**Native Apple Silicon macOS port of C&C Generals Zero Hour / Generals Online with a direct Metal renderer.**

This is not Wine, not DXVK, not MoltenVK, and not an emulator-based wrapper — it's a full native recompilation of the game engine targeting ARM64 macOS with a custom DirectX 8 → Metal translation layer.

---

## Current Status

| Area | Status |
|:---|:---:|
| Native Apple Silicon (ARM64) build | ✅ Working |
| Direct Metal rendering backend | ✅ Working |
| Singleplayer (Skirmish & Campaign) | ✅ Working |
| Online multiplayer (macOS ↔ macOS) | ✅ Working |
| Online multiplayer (macOS ↔ Windows) | 🔄 In Progress |
| Native macOS audio (AVAudioEngine) | ✅ Working |
| macOS Launcher (SwiftUI) | ✅ Working |
| Replay compatibility (macOS ↔ macOS) | ✅ Working |
| Replay compatibility (from Windows) | 🔄 Requires deterministic math |

> **Note:** Cross-platform multiplayer (macOS ↔ Windows) requires deterministic math parity between ARM64 and x87 FPU pipelines. This is actively being worked on. See [docs/DETERMINISTIC_MATH.md](docs/DETERMINISTIC_MATH.md) for details.

---

## Videos

| | |
|:---|:---|
| 🎬 **Installation Guide** | [YouTube — How to install C&C Generals on macOS](https://www.youtube.com/@okjid) |
| 🎮 **Native Metal Demo** | [YouTube — Native macOS Metal gameplay](https://www.youtube.com/@okjid) |
| 🔥 **Stress Test (8 Brutal AIs)** | [YouTube — 8 bot stress test, ~40 FPS stable](https://www.youtube.com/@okjid) |

---

## How This Differs from GeneralsX

This project is a separate effort with different goals:

| | This Port | GeneralsX |
|:---|:---|:---|
| **Target platform** | macOS Apple Silicon (native) | macOS via CrossOver / Wine |
| **Rendering** | Custom DX8 → Metal bridge | DXVK / MoltenVK / Wine D3D |
| **Online** | Generals Online (new multiplayer) | GameRanger / LAN |
| **Simulation** | Targeting deterministic cross-platform sync | Windows-only simulation |
| **Codebase** | Fork of Generals Online (GOD Team) | Fork of TheSuperHackers |

Both projects share the same goal of keeping C&C Generals alive. This port focuses specifically on delivering a first-class native experience on Apple Silicon with Generals Online multiplayer support.

---

## Quick Start

### Prerequisites
- Apple Silicon Mac (M1/M2/M3/M4)
- macOS 14+ (Sonoma or later)
- Xcode Command Line Tools (`xcode-select --install`)
- Original C&C Generals Zero Hour game files ([Steam](https://store.steampowered.com/bundle/39394))

### Build & Run
```bash
sh build_run_mac.sh          # Build and launch
sh build_run_mac.sh --clean  # Clean rebuild
```

For detailed setup, prerequisites, and technical documentation see:
- **[Setup Guide](Platform/MacOS/docs/SETUP.md)** — Full installation walkthrough
- **[macOS Port Documentation](Platform/MacOS/docs/README.md)** — Architecture overview

---

## Documentation

| Document | Description |
|:---|:---|
| [Platform/MacOS/docs/README.md](Platform/MacOS/docs/README.md) | macOS port architecture overview |
| [Platform/MacOS/docs/SETUP.md](Platform/MacOS/docs/SETUP.md) | Build setup and prerequisites |
| [Platform/MacOS/docs/RENDERING.md](Platform/MacOS/docs/RENDERING.md) | DX8 → Metal rendering pipeline |
| [Platform/MacOS/docs/FILE_SYSTEM.md](Platform/MacOS/docs/FILE_SYSTEM.md) | File system and resource resolution |
| [Platform/MacOS/docs/DEVELOPMENT.md](Platform/MacOS/docs/DEVELOPMENT.md) | Development guidelines |
| [Platform/MacOS/docs/BUILD_SYSTEM.md](Platform/MacOS/docs/BUILD_SYSTEM.md) | CMake build system details |
| [Platform/MacOS/docs/IMPLEMENTATION_STATUS.md](Platform/MacOS/docs/IMPLEMENTATION_STATUS.md) | Detailed implementation status |
| [docs/MACOS_INSTALL.md](docs/MACOS_INSTALL.md) | Player-facing installation guide |
| [docs/DETERMINISTIC_MATH.md](docs/DETERMINISTIC_MATH.md) | Cross-platform math synchronization |
| [docs/FAQ.md](docs/FAQ.md) | Frequently asked questions |

---

## Related Pull Requests

This port is being contributed back to the upstream projects:

- **GameClient** — [GeneralsOnlineDevelopmentTeam/GameClient #457](https://github.com/GeneralsOnlineDevelopmentTeam/GameClient/pull/457)
- **GeneralsGameCode** — [TheSuperHackers/GeneralsGameCode #2602](https://github.com/TheSuperHackers/GeneralsGameCode/pull/2602)

---

## Upstream

This fork is based on the [Generals Online Development Team](https://github.com/GeneralsOnlineDevelopmentTeam/GameClient) codebase. The original community project by [TheSuperHackers](https://github.com/TheSuperHackers/GeneralsGameCode) serves as the foundation for all Generals source code efforts.

## Contributing

Contributions are welcome! If you're interested in helping with the macOS port — especially in areas like deterministic math, Metal rendering, or audio — join the discussion in the related PRs above or open an issue.

Please read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting a pull request.

## License & Legal Disclaimer

EA has not endorsed and does not support this product. All trademarks are the property of their respective owners.

This project is licensed under the [GPL-3.0 License](https://www.gnu.org/licenses/gpl-3.0.html). See [LICENSE.md](LICENSE.md) for details.
