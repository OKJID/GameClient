#!/bin/bash

# Deterministic-math parity run (macOS side).
# Launches the game headless with -mathCrcCheck. GameMain runs the parity dump right after
# engine init (working dir = game dir, just like a replay), writes SimulationMathCrc.txt and
# exits. The result is copied to .agent/temp_mac_math/ for diffing against the Windows side
# (.agent/temp_win_math/, pulled from the context repo).
#
#   sh run_math_mac.sh              # build + run + collect
#   sh run_math_mac.sh --no-build   # skip build, just run + collect

export PATH="/opt/homebrew/bin:$PATH"

DO_BUILD=true
for arg in "$@"; do
    case "$arg" in
        --no-build) DO_BUILD=false ;;
    esac
done

REPO="$PWD"
APP="build/macos/GeneralsMD/GeneralsOnlineZH.app"
OUT_DIR="$REPO/.agent/temp_mac_math"
LOG_DIR="$REPO/Platform/MacOS/Build/Logs"
STDOUT_LOG="$LOG_DIR/math.log"

export GENERALS_INSTALL_PATH="/Users/okji/dev/games/General Online Common"
export GENERALS_FPS_LIMIT="${GENERALS_FPS_LIMIT:-60}"

if [ "$DO_BUILD" = true ]; then
    if [ ! -d "build/macos" ]; then
        cmake --preset macos || exit 1
    fi
    echo "Building..."
    cmake --build build/macos --target z_generals || exit 1
fi

mkdir -p "$OUT_DIR" "$LOG_DIR"

echo "Killing previous instance..."
killall -9 GeneralsOnlineZH 2>/dev/null
sleep 1

# Purge stale parity files so only this run is collected.
find "$GENERALS_INSTALL_PATH" -maxdepth 3 -type f -name "SimulationMathCrc.txt" -delete 2>/dev/null
rm -f "$OUT_DIR/SimulationMathCrc.txt"

echo "Running -mathCrcCheck (headless)..."
open -W -n "$APP" --stdout "$STDOUT_LOG" --stderr "$STDOUT_LOG" \
    --env GENERALS_INSTALL_PATH="$GENERALS_INSTALL_PATH" \
    --env GENERALS_FPS_LIMIT="$GENERALS_FPS_LIMIT" \
    --args -headless -mathCrcCheck

FOUND="$(find "$GENERALS_INSTALL_PATH" -maxdepth 3 -type f -name "SimulationMathCrc.txt" -print 2>/dev/null | head -1)"
if [ -z "$FOUND" ]; then
    echo "ERROR: SimulationMathCrc.txt not found under $GENERALS_INSTALL_PATH"
    echo "       Aggregate CRCs are also printed to stdout - see $STDOUT_LOG"
    exit 1
fi

cp -f "$FOUND" "$OUT_DIR/SimulationMathCrc.txt"
echo "Collected -> $OUT_DIR/SimulationMathCrc.txt"
echo "--- aggregate CRCs ---"
grep "= " "$OUT_DIR/SimulationMathCrc.txt" | grep "SimulationMathCrc" | tail -4
