# Deterministic Math — Cross-Platform Multiplayer Synchronization

## The Problem

C&C Generals uses a **lockstep multiplayer** model: every player's game simulation must produce bit-identical results every frame. If any calculation differs — even by a single bit — the games desynchronize (CRC mismatch) and the match is aborted.

The original game was written for 32-bit Windows using the **x87 FPU**, which uses 80-bit extended precision internally. This means that the exact same C++ floating-point code can produce subtly different results on different hardware:

| Platform | FPU | Internal Precision | Rounding Behavior |
|:---|:---|:---|:---|
| Windows x86/x64 | x87 (legacy mode) | 80-bit extended | Platform-specific |
| macOS ARM64 | NEON (IEEE 754) | 64-bit double | Strictly IEEE 754 |
| Linux x64 (SSE) | SSE2 | 64-bit double | Strictly IEEE 754 |

Even between Windows builds, the x87 FPU can produce different results depending on compiler settings, precision control word, and rounding mode.

## Why This Matters

In a lockstep game, **every** floating-point operation across **every** player must produce the **exact same result**. This includes:

- Unit movement and pathfinding
- Projectile trajectory calculations
- Damage calculations
- Resource generation rates
- AI decision-making

A single bit difference in any of these accumulates over frames and eventually causes a CRC mismatch.

## The Solution

To achieve cross-platform deterministic simulation, the approach is:

1. **Identify all floating-point operations** in the simulation (game logic) path.
2. **Replace platform-dependent FPU calls** with deterministic wrappers that guarantee identical results on all platforms.
3. **Ensure compiler settings** do not reorder or optimize floating-point operations in ways that change results (`-ffp-contract=off`, `-fno-fast-math`).

### Current Status

| Scope | Status |
|:---|:---:|
| macOS ↔ macOS (same architecture) | ✅ Deterministic |
| macOS ↔ Windows (cross-architecture) | 🔄 In Progress |
| Compiler flags configured | ✅ Done |
| FPU precision control | 🔄 In Progress |

## Technical Details

### Compiler Flags

The macOS build uses strict floating-point flags:
- `-ffp-contract=off` — Prevents fused multiply-add (FMA) which changes precision
- No `-ffast-math` — Preserves IEEE 754 compliance
- `-mno-fma` on specific translation units where required

### Key Files

- Game simulation logic: `GameLogic/` directory
- Math utilities: `Common/` directory
- CRC verification: `GameNetwork/` — CRC is computed every N frames and compared across all players

## Further Reading

- [IEEE 754 Floating-Point Standard](https://en.wikipedia.org/wiki/IEEE_754)
- [What Every Computer Scientist Should Know About Floating-Point Arithmetic](https://docs.oracle.com/cd/E19957-01/806-3568/ncg_goldberg.html)
- [x87 vs SSE: Precision Differences](https://randomascii.wordpress.com/2013/07/16/floating-point-determinism/)
