# macOS Port — Installation Guide

This guide is for **players** who want to run C&C Generals Zero Hour natively on macOS.

---

## System Requirements

| Requirement | Details |
|:---|:---|
| **Mac** | Apple Silicon (M1, M2, M3, M4 or later) |
| **macOS** | 14.0 Sonoma or later |
| **RAM** | 8 GB minimum |
| **Disk** | ~4 GB for game files + ~500 MB for the build |
| **Game files** | Original C&C Generals Zero Hour (Steam or Origin) |
| **Dev tools** | Xcode Command Line Tools |

---

## Step 1 — Install Xcode Command Line Tools

Open Terminal and run:

```bash
xcode-select --install
```

---

## Step 2 — Get the Original Game Files

You need the original game assets (maps, textures, audio, etc.). The easiest way:

1. Purchase *Command & Conquer: The Ultimate Collection* on [Steam](https://store.steampowered.com/bundle/39394).
2. Use [SteamCMD](https://developer.valvesoftware.com/wiki/SteamCMD) or the included macOS Launcher to download the game data files.

> **Important:** No game assets are included in this project. You must own the original game.

---

## Step 3 — Clone and Build

```bash
git clone https://github.com/OKJID/GameClient.git
cd GameClient
sh build_run_mac.sh
```

This will:
1. Configure the CMake project for macOS ARM64
2. Build the game binary
3. Launch the game

For a clean rebuild:
```bash
sh build_run_mac.sh --clean
```

---

## Step 4 — Set Up Game Data

The game expects data files in a specific location. See [Platform/MacOS/docs/SETUP.md](../Platform/MacOS/docs/SETUP.md) for detailed instructions on where to place your game files.

---

## Step 5 — Play

Launch the game:
```bash
sh build_run_mac.sh
```

Or use the macOS Launcher located in `Platform/MacOS/Launcher/`.

---

## Troubleshooting

| Problem | Solution |
|:---|:---|
| Game crashes on launch | Check that all game data files are in place. See SETUP.md. |
| No sound | Ensure macOS audio output is configured. The game uses AVAudioEngine. |
| Black screen | Try toggling fullscreen/windowed mode. Check display resolution settings. |
| Online not connecting | macOS ↔ macOS multiplayer requires both players to use this build. |

For more details, see [FAQ.md](FAQ.md).
