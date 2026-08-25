#!/usr/bin/env python3
"""Print the mac parity constants for a Generals Online Windows executable.

The engine seeds its exe CRC with a shift-add value (Core/GameEngine/.../crc.cpp), while
the service reports the IEEE CRC32 of the same file. Neither converts into the other, so
both are measured here and pasted into OnlineServices_Init.cpp.

Drop GeneralsOnlineZH_60.exe next to this script and run it.
"""

import os
import sys
import zlib

DEFAULT_EXE = "GeneralsOnlineZH_60.exe"


def shift_add_crc(data):
    """The engine's CRC: rotate left through the carry, then add the byte."""
    crc = 0
    for byte in data:
        hibit = 1 if crc & 0x80000000 else 0
        crc = (crc << 1) & 0xFFFFFFFF
        crc = (crc + byte) & 0xFFFFFFFF
        crc = (crc + hibit) & 0xFFFFFFFF
    return crc


def locate_exe(argument):
    if argument:
        return argument

    script_dir = os.path.dirname(os.path.abspath(__file__))

    preferred = os.path.join(script_dir, DEFAULT_EXE)
    if os.path.isfile(preferred):
        return preferred

    candidates = [name for name in sorted(os.listdir(script_dir)) if name.lower().endswith(".exe")]
    if len(candidates) == 1:
        return os.path.join(script_dir, candidates[0])

    if not candidates:
        print(f"[!] No .exe next to the script. Put {DEFAULT_EXE} in {script_dir}")
        return None

    print(f"[!] Several executables next to the script, pass one explicitly: {', '.join(candidates)}")
    return None


def main():
    exe_path = locate_exe(sys.argv[1] if len(sys.argv) > 1 else None)
    if not exe_path:
        return 1

    if not os.path.isfile(exe_path):
        print(f"[!] Not a file: {exe_path}")
        return 1

    with open(exe_path, "rb") as handle:
        data = handle.read()

    shift_add = shift_add_crc(data)
    ieee = zlib.crc32(data) & 0xFFFFFFFF

    print(f"File      : {exe_path}")
    print(f"Size      : {len(data)} bytes")
    print(f"IEEE CRC32: {ieee}")
    print(f"Shift-add : {shift_add}")
    print()
    print("GeneralsMD/.../GeneralsOnline/OnlineServices_Init.cpp:")
    print(f"    constexpr long MAC_PARITY_FALLBACK_EXE_CRC = {ieee};")
    print(f"    constexpr long MAC_PARITY_SHIFT_ADD_BASE   = {shift_add};")
    return 0


if __name__ == "__main__":
    sys.exit(main())
