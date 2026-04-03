#!/usr/bin/env python3
"""
Capture a screenshot of the game window.
Always saves to: Platform/MacOS/Build/Logs/screenshot_game_window.png
Usage: python3 screenshot.py
"""
import subprocess
import sys
import os
import json

OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "Logs", "screenshot_game_window.png")


def find_game_window_id():
    """Find the main game window ID using CGWindowListCopyWindowInfo.
    Picks the largest generalszh window (the actual game, not titlebar helpers)."""
    result = subprocess.run(
        ["python3", "-c", """
import Quartz, json
windows = Quartz.CGWindowListCopyWindowInfo(
    Quartz.kCGWindowListOptionAll, Quartz.kCGNullWindowID
)
candidates = []
for w in windows:
    owner = w.get('kCGWindowOwnerName', '')
    if 'generalszh' not in owner.lower():
        continue
    bounds = w.get('kCGWindowBounds', {})
    width = int(bounds.get('Width', 0))
    height = int(bounds.get('Height', 0))
    candidates.append({
        'id': w['kCGWindowNumber'],
        'owner': owner,
        'title': w.get('kCGWindowName', ''),
        'width': width,
        'height': height,
        'area': width * height
    })
# Sort by area descending — largest window is the game
candidates.sort(key=lambda c: c['area'], reverse=True)
if candidates:
    print(json.dumps(candidates[0]))
"""],
        capture_output=True, text=True
    )
    if result.stdout.strip():
        return json.loads(result.stdout.strip())
    return None


def capture_window():
    """Capture the game window screenshot."""
    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)

    info = find_game_window_id()
    if info:
        wid = info['id']
        print(f"Found game window: '{info['title']}' (id={wid}, {info['width']}x{info['height']})")
        subprocess.run(["screencapture", "-l", str(wid), "-x", OUTPUT_PATH], check=True)
        print(f"Screenshot saved: {OUTPUT_PATH}")
        return OUTPUT_PATH

    # Fallback: full screen capture
    print("Game window not found, falling back to full screen capture...")
    subprocess.run(["screencapture", "-x", OUTPUT_PATH], check=True)
    print(f"Screenshot saved: {OUTPUT_PATH}")
    return OUTPUT_PATH


if __name__ == "__main__":
    capture_window()
