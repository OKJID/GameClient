#!/bin/bash

# Run:
#   sh build_run_mac.sh                       # build + run
#   sh build_run_mac.sh --clean               # clean + build + run
#   sh build_run_mac.sh --screenshot          # build + run + screenshot after 12s
#   sh build_run_mac.sh --screenshot=8.5      # build + run + screenshot after 8.5s
#   sh build_run_mac.sh --test                # build + run tests
#   sh build_run_mac.sh --lldb                # build + run with lldb
#   sh build_run_mac.sh --release             # configure/build game with debug logging/crashing
#   sh build_run_mac.sh --full_logs           # build + run with full logs

export PATH="/opt/homebrew/bin:$PATH"

# ── Game Command-Line Flags ──
# Toggle on/off: true = pass to game, false = skip
GAME_FLAG_NOSHELLMAP=false
GAME_FLAG_QUICKSTART=false
GAME_FLAG_NOAUDIO=false
GAME_FLAG_WIN=false
GAME_FLAG_XRES=""       # e.g. "1024"
GAME_FLAG_YRES=""       # e.g. "768"


DO_CLEAN=false
DO_SCREENSHOT=false
DO_TEST=false
DO_LLDB=false
DO_FULL_LOGS=false
DO_DEBUG=true
TEST_FILTER=""
SCREENSHOT_DELAY=""

for arg in "$@"; do
    case "$arg" in
        --clean)
            DO_CLEAN=true
            ;;
        --test)
            DO_TEST=true
            ;;
        --test=*)
            DO_TEST=true
            TEST_FILTER="${arg#--test=}"
            ;;
        --screenshot=*)
            DO_SCREENSHOT=true
            SCREENSHOT_DELAY="${arg#--screenshot=}"
            ;;
        --screenshot)
            DO_SCREENSHOT=true
            ;;
        --lldb)
            DO_LLDB=true
            ;;
        --release)
            DO_DEBUG=false
            ;;
        --full_logs)
            DO_FULL_LOGS=true
            ;;
    esac
done

# Bind the Mac debug logging to the script's debug flag, unless explicitly overriden
if [ -z "$GENERALS_MAC_DEBUG" ]; then
    if [ "$DO_DEBUG" = true ]; then
        export GENERALS_MAC_DEBUG=1
    else
        export GENERALS_MAC_DEBUG=0
    fi
else
    export GENERALS_MAC_DEBUG="$GENERALS_MAC_DEBUG"
fi

if [ "$DO_CLEAN" = true ]; then
    echo "Cleaning build directory..."
    rm -rf build
fi

if [ ! -d "build/macos" ]; then
    echo "Configuring CMake preset..."
    cmake --preset macos
    if [ $? -ne 0 ]; then
        exit 1
    fi
fi

echo "Building project..."
cmake --build build/macos
if [ $? -ne 0 ]; then
    exit 1
fi

# ── Test Mode ──
if [ "$DO_TEST" = true ]; then
    mkdir -p Platform/MacOS/Build/Logs
    TEST_LOG="Platform/MacOS/Build/Logs/test_results.log"
    echo ""
    echo "Running DX8→Metal Bridge Tests..."
    echo ""
    if [ -n "$TEST_FILTER" ]; then
        ./build/macos/Platform/MacOS/Tests/metal_bridge_tests "$TEST_FILTER" 2>&1 | tee "$TEST_LOG"
    else
        ./build/macos/Platform/MacOS/Tests/metal_bridge_tests 2>&1 | tee "$TEST_LOG"
    fi
    TEST_EXIT=${PIPESTATUS[0]}
    echo ""
    echo "Test log saved to: $TEST_LOG"
    exit $TEST_EXIT
fi

sleep 1

LOG_DIR="Platform/MacOS/Build/Logs"
LOG_FILE="$LOG_DIR/game.log"
if [ -f "$LOG_FILE" ]; then
    [ -f "$LOG_FILE.bak" ] && mv "$LOG_FILE.bak" "$LOG_FILE.bak2"
    cp "$LOG_FILE" "$LOG_FILE.bak"
    rm -f "$LOG_FILE"
    echo "Logs backed up: game.log → game.log.bak and cleared"
fi

echo "Killing previous generalszh instance..."
killall -9 GeneralsOnlineZH 2>/dev/null
killall -9 lldb 2>/dev/null

sleep 1

export GENERALS_INSTALL_PATH="/Users/okji/dev/games/General Online Common"

# Metal frame rate control:
# 60 = VSync (default)
# 0  = uncapped
# 30/120/240 = custom
export GENERALS_FPS_LIMIT="${GENERALS_FPS_LIMIT:-60}"

# Screenshot delay (default 12s)
if [ -z "$SCREENSHOT_DELAY" ]; then
    SCREENSHOT_DELAY=12
fi

# ── Build Game Args ──
GAME_ARGS=""
[ "$GAME_FLAG_NOSHELLMAP" = true ] && GAME_ARGS="$GAME_ARGS -noshellmap"
[ "$GAME_FLAG_QUICKSTART" = true ] && GAME_ARGS="$GAME_ARGS -quickstart"
[ "$GAME_FLAG_NOAUDIO" = true ]    && GAME_ARGS="$GAME_ARGS -noaudio"
[ "$GAME_FLAG_WIN" = true ]        && GAME_ARGS="$GAME_ARGS -win"
[ -n "$GAME_FLAG_XRES" ]           && GAME_ARGS="$GAME_ARGS -xRes $GAME_FLAG_XRES"
[ -n "$GAME_FLAG_YRES" ]           && GAME_ARGS="$GAME_ARGS -yRes $GAME_FLAG_YRES"
[ "$DO_FULL_LOGS" = true ]         && GAME_ARGS="$GAME_ARGS -saveDebugCRCPerFrame /Users/okji/dev/games/GameClient/CRCLogs"

GAME_CMD="build/macos/GeneralsMD/GeneralsOnlineZH.app/Contents/MacOS/GeneralsOnlineZH"

if [ -n "$GAME_ARGS" ]; then
    echo "Game args:$GAME_ARGS"
fi

echo "Starting game..."
if [ "$DO_LLDB" = true ]; then
    echo "Launching under lldb (attach-on-start)..."
    echo "  lldb will attach as soon as the app launches via 'open'"
    echo "  After crash: type 'bt' for backtrace, 'bt all' for all threads"
    LLDB_LOG="$PWD/Platform/MacOS/Build/Logs/lldb_logs.log"
    # Start lldb waiting for process, then launch .app via open
    lldb -n GeneralsOnlineZH --wait-for -o "continue" &
    LLDB_PID=$!
    sleep 1
    if [ -n "$GAME_ARGS" ]; then
        open -n "build/macos/GeneralsMD/GeneralsOnlineZH.app" --stdout "$LLDB_LOG" --stderr "$LLDB_LOG" --env GENERALS_INSTALL_PATH="$GENERALS_INSTALL_PATH" --env GENERALS_FPS_LIMIT="$GENERALS_FPS_LIMIT" --args $GAME_ARGS
    else
        open -n "build/macos/GeneralsMD/GeneralsOnlineZH.app" --stdout "$LLDB_LOG" --stderr "$LLDB_LOG" --env GENERALS_INSTALL_PATH="$GENERALS_INSTALL_PATH" --env GENERALS_FPS_LIMIT="$GENERALS_FPS_LIMIT"
    fi
    wait $LLDB_PID
elif [ "$DO_SCREENSHOT" = true ]; then
    $GAME_CMD $GAME_ARGS > Platform/MacOS/Build/Logs/game.log 2>&1 &
    GAME_PID=$!
    echo "Waiting ${SCREENSHOT_DELAY}s for game to load..."
    sleep ${SCREENSHOT_DELAY}
    python3 Platform/MacOS/Build/screenshot.py --pid $GAME_PID
    echo "Killing game (pid=$GAME_PID)..."
    kill $GAME_PID 2>/dev/null
    wait $GAME_PID 2>/dev/null
else
    if [ -n "$GAME_ARGS" ]; then
        open -W -n "build/macos/GeneralsMD/GeneralsOnlineZH.app" --stdout "$PWD/Platform/MacOS/Build/Logs/game.log" --stderr "$PWD/Platform/MacOS/Build/Logs/game.log" --env GENERALS_INSTALL_PATH="$GENERALS_INSTALL_PATH" --env GENERALS_FPS_LIMIT="$GENERALS_FPS_LIMIT" --args $GAME_ARGS
    else
        open -W -n "build/macos/GeneralsMD/GeneralsOnlineZH.app" --stdout "$PWD/Platform/MacOS/Build/Logs/game.log" --stderr "$PWD/Platform/MacOS/Build/Logs/game.log" --env GENERALS_INSTALL_PATH="$GENERALS_INSTALL_PATH" --env GENERALS_FPS_LIMIT="$GENERALS_FPS_LIMIT"
    fi
fi
