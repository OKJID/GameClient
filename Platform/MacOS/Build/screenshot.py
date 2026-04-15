#!/usr/bin/env python3
"""
Capture a screenshot of the game window by PID or process name.
Always saves to: Platform/MacOS/Build/Logs/screenshot_game_window.png

Usage:
  python3 screenshot.py              # find by process name
  python3 screenshot.py --pid 12345  # find by specific PID
"""
import subprocess
import sys
import os
import json

OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "Logs", "screenshot_game_window.png")

PROCESS_NAME_PATTERNS = ["generalsonlinezh", "generalszh", "generals"]


def find_window_by_pid(target_pid):
    """Find the largest window owned by a specific PID."""
    code = f"""
import Quartz, json
windows = Quartz.CGWindowListCopyWindowInfo(
    Quartz.kCGWindowListOptionAll, Quartz.kCGNullWindowID
)
candidates = []
for w in windows:
    pid = w.get('kCGWindowOwnerPID', -1)
    if pid != {target_pid}:
        continue
    layer = w.get('kCGWindowLayer', 999)
    if layer != 0:
        continue
    bounds = w.get('kCGWindowBounds', {{}})
    width = int(bounds.get('Width', 0))
    height = int(bounds.get('Height', 0))
    if width < 100 or height < 100:
        continue
    candidates.append({{
        'id': w['kCGWindowNumber'],
        'owner': w.get('kCGWindowOwnerName', ''),
        'title': w.get('kCGWindowName', ''),
        'width': width,
        'height': height,
        'area': width * height,
        'pid': pid,
    }})
candidates.sort(key=lambda c: c['area'], reverse=True)
if candidates:
    print(json.dumps(candidates[0]))
"""
    result = subprocess.run(["python3", "-c", code], capture_output=True, text=True)
    if result.stdout.strip():
        return json.loads(result.stdout.strip())
    return None


def find_window_by_name():
    """Find the largest window whose owner name matches known game process names."""
    patterns_str = json.dumps(PROCESS_NAME_PATTERNS)
    code = f"""
import Quartz, json
patterns = {patterns_str}
windows = Quartz.CGWindowListCopyWindowInfo(
    Quartz.kCGWindowListOptionAll, Quartz.kCGNullWindowID
)
candidates = []
for w in windows:
    owner = w.get('kCGWindowOwnerName', '').lower()
    if not any(p in owner for p in patterns):
        continue
    layer = w.get('kCGWindowLayer', 999)
    if layer != 0:
        continue
    bounds = w.get('kCGWindowBounds', {{}})
    width = int(bounds.get('Width', 0))
    height = int(bounds.get('Height', 0))
    if width < 100 or height < 100:
        continue
    candidates.append({{
        'id': w['kCGWindowNumber'],
        'owner': w.get('kCGWindowOwnerName', ''),
        'title': w.get('kCGWindowName', ''),
        'width': width,
        'height': height,
        'area': width * height,
        'pid': w.get('kCGWindowOwnerPID', -1),
    }})
candidates.sort(key=lambda c: c['area'], reverse=True)
if candidates:
    print(json.dumps(candidates[0]))
"""
    result = subprocess.run(["python3", "-c", code], capture_output=True, text=True)
    if result.stdout.strip():
        return json.loads(result.stdout.strip())
    return None


def capture_window():
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)

    target_pid = None
    for i, arg in enumerate(sys.argv[1:], 1):
        if arg == "--pid" and i < len(sys.argv) - 1:
            target_pid = int(sys.argv[i + 1])

    info = None
    if target_pid:
        info = find_window_by_pid(target_pid)
        if not info:
            print(f"No window found for PID {target_pid}, trying by name...")

    if not info:
        info = find_window_by_name()

    if info:
        wid = info['id']
        print(f"Found game window: '{info.get('title','')}' owner='{info['owner']}' "
              f"(id={wid}, {info['width']}x{info['height']}, pid={info.get('pid','')})")
        subprocess.run(["screencapture", "-l", str(wid), "-x", OUTPUT_PATH], check=True)
        print(f"Screenshot saved: {OUTPUT_PATH}")
        return OUTPUT_PATH

    print("Game window not found, falling back to full screen capture...")
    subprocess.run(["screencapture", "-x", OUTPUT_PATH], check=True)
    print(f"Screenshot saved: {OUTPUT_PATH}")
    return OUTPUT_PATH


if __name__ == "__main__":
    capture_window()
