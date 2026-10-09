# Frequently Asked Questions

## Is this Wine / CrossOver?

**No.** This is a full native recompilation of the game engine for macOS ARM64. There is no Wine, no translation layer for Windows APIs, and no x86 emulation. The game binary runs directly on Apple Silicon.

## Is this DXVK / MoltenVK?

**No.** The rendering backend is a custom **DirectX 8 → Metal** translation layer written specifically for this port. It translates DX8 render states, shaders, and draw calls directly to Metal API calls. MoltenVK (which translates Vulkan → Metal) is not used.

## Does online multiplayer work?

**macOS ↔ macOS:** Yes, fully working via Generals Online servers.

**macOS ↔ Windows:** In progress. The lockstep multiplayer model requires every player's simulation to produce bit-identical results. ARM64 and x87 floating-point units can produce subtly different results, which causes CRC mismatches. See [DETERMINISTIC_MATH.md](DETERMINISTIC_MATH.md) for details.

## Are old replays compatible?

**macOS ↔ macOS:** Yes, replays recorded on macOS work on macOS builds.

**From Windows:** Not yet. Windows replays require deterministic math parity between ARM64 and x87 FPU, which is still in progress. See [DETERMINISTIC_MATH.md](DETERMINISTIC_MATH.md).

## What macOS version do I need?

macOS 14 Sonoma or later on an Apple Silicon Mac (M1/M2/M3/M4).

## Do I need the original game?

Yes. This project contains only the game engine source code. You must own the original *Command & Conquer: Generals Zero Hour* to obtain the game data files (maps, textures, audio, etc.).

## How is this different from GeneralsX?

GeneralsX is a cross-platform port using SDL3 + DXVK (DX8 → Vulkan, via MoltenVK on macOS) + OpenAL. It targets Linux, macOS, and Windows with a single portable codebase.

This project is a native macOS-specific port with:
- Direct Metal rendering (no Vulkan/MoltenVK layer)
- Native macOS audio via AVAudioEngine (no OpenAL)
- Native Cocoa windowing (no SDL)
- Generals Online multiplayer support
- Deterministic math work for cross-platform sync

Both projects aim to keep C&C Generals playable on modern hardware.

## Can I play on Intel Mac?

Currently only Apple Silicon (ARM64) is supported. Intel Mac support is not planned but may work with additional build configuration.

## Where do I report bugs?

Open an issue on [GitHub](https://github.com/OKJID/GameClient/issues) or discuss in the related upstream PRs:
- [GameClient #457](https://github.com/GeneralsOnlineDevelopmentTeam/GameClient/pull/457)
- [GeneralsGameCode #2602](https://github.com/TheSuperHackers/GeneralsGameCode/pull/2602)
