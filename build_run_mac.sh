#!/bin/bash

# Run:
#   sh build_run_mac.sh                       # build + run
#   sh build_run_mac.sh --clean               # clean + build + run
#   sh build_run_mac.sh --screenshot          # build + run + screenshot after 12s
#   sh build_run_mac.sh --screenshot=8.5      # build + run + screenshot after 8.5s
#   sh build_run_mac.sh --test                # build + run tests
#   sh build_run_mac.sh --lldb                # build + run with lldb
#   sh build_run_mac.sh --release             # configure/build game with debug logging/crashing
#   sh build_run_mac.sh --crc_logs            # build + run with full crc logs

export PATH="/opt/homebrew/bin:$PATH"

# ── Game Selection ──
# Which game to launch. Both are always built; this picks the one that runs.
RUN_VANILLA=true

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
DO_CRC_LOGS=false
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
        --crc_logs)
            DO_CRC_LOGS=true
            ;;
        --rep_def)
            DO_REPLAY_DEF=true
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
LOG_KEEP=5

if [ -f "$LOG_FILE" ]; then
    gen=$((LOG_KEEP - 1))
    while [ $gen -ge 3 ]; do
        [ -f "$LOG_FILE.bak$((gen - 1))" ] && mv "$LOG_FILE.bak$((gen - 1))" "$LOG_FILE.bak$gen"
        gen=$((gen - 1))
    done
    [ -f "$LOG_FILE.bak" ] && mv "$LOG_FILE.bak" "$LOG_FILE.bak2"
    cp "$LOG_FILE" "$LOG_FILE.bak"
    rm -f "$LOG_FILE"
    echo "Logs rotated: game.log → game.log.bak (keeping $LOG_KEEP runs)"
fi

if [ "$RUN_VANILLA" = true ]; then
    GAME_NAME="GeneralsVanilla"
    GAME_BUNDLE="build/macos/Generals/GeneralsVanilla.app"
    GAME_DATA_DIR="$HOME/Command and Conquer Generals Data"
else
    GAME_NAME="GeneralsOnlineZH"
    GAME_BUNDLE="build/macos/GeneralsMD/GeneralsOnlineZH.app"
    GAME_DATA_DIR="$HOME/Command and Conquer Generals Zero Hour Data"
fi
GAME_CMD="$GAME_BUNDLE/Contents/MacOS/$GAME_NAME"

echo "Selected game: $GAME_NAME"

echo "Killing previous game instances..."
killall -9 GeneralsOnlineZH 2>/dev/null
killall -9 GeneralsVanilla 2>/dev/null
killall -9 lldb 2>/dev/null

sleep 1

if [ "$DO_CRC_LOGS" = true ]; then
    echo "Clearing old CRC logs..."
    mkdir -p "$PWD/.agent/temp_mac_logs"
    rm -rf "$PWD/.agent/temp_mac_logs/CRCLogs"/* 2>/dev/null
    rm -rf "$PWD/.agent/temp_mac_logs/CRCLogs2"/* 2>/dev/null
    mkdir -p "$PWD/.agent/temp_mac_logs/CRCLogs"
    mkdir -p "$PWD/.agent/temp_mac_logs/CRCLogs2"
fi

export GENERALS_INSTALL_PATH="/Users/okji/dev/games/General Online Common"
# export GENERALS_INSTALL_PATH="/Users/okji/Documents/Generals Online"

# Purge previous-run diagnostics from the game dir BEFORE the run, so append-mode
# ("a") diags never mix two runs. Same patterns the gather step collects below.
# The replay (.rep) is explicitly preserved. Only when collecting CRC logs.
if [ "$DO_CRC_LOGS" = true ]; then
    echo "Purging previous-run diagnostic logs from game dir (keeping replay)..."
    find "$GENERALS_INSTALL_PATH" -maxdepth 2 -type f \( -name "*Diag*.txt" -o -name "*Log*.txt" -o -name "*Debug*.txt" \) ! -name "*.rep" -delete 2>/dev/null
    rm -rf "$PWD/CRCLogs" 2>/dev/null
fi

# Copy splash screen into app bundle
SPLASH_SRC="Platform/MacOS/Launcher/assets/Install_Final.bmp"
if [ -f "$SPLASH_SRC" ]; then
    mkdir -p "$GAME_BUNDLE/Contents/Resources"
    cp -f "$SPLASH_SRC" "$GAME_BUNDLE/Contents/Resources/Install_Final.bmp"
fi

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
[ "$DO_REPLAY_DEF" = true ]        && GAME_ARGS="$GAME_ARGS -headless -replay 00000000.rep"
[ "$DO_CRC_LOGS" = true ]         && GAME_ARGS="$GAME_ARGS -saveDebugCRCPerFrame $PWD/.agent/temp_mac_logs/CRCLogs -keepCRCSave -logObjectCRCs -logRandom"

if [ ! -x "$GAME_CMD" ]; then
    echo "ERROR: $GAME_NAME executable not found at $GAME_CMD"
    echo "       Set RUN_VANILLA=false to run Zero Hour, or finish the vanilla port first."
    exit 1
fi

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
    lldb -n "$GAME_NAME" --wait-for -o "continue" &
    LLDB_PID=$!
    sleep 1
    if [ -n "$GAME_ARGS" ]; then
        open -n "$GAME_BUNDLE" --stdout "$LLDB_LOG" --stderr "$LLDB_LOG" --env GENERALS_INSTALL_PATH="$GENERALS_INSTALL_PATH" --env GENERALS_FPS_LIMIT="$GENERALS_FPS_LIMIT" --args $GAME_ARGS
    else
        open -n "$GAME_BUNDLE" --stdout "$LLDB_LOG" --stderr "$LLDB_LOG" --env GENERALS_INSTALL_PATH="$GENERALS_INSTALL_PATH" --env GENERALS_FPS_LIMIT="$GENERALS_FPS_LIMIT"
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
        open -W -n "$GAME_BUNDLE" --stdout "$PWD/Platform/MacOS/Build/Logs/game.log" --stderr "$PWD/Platform/MacOS/Build/Logs/game.log" --env GENERALS_INSTALL_PATH="$GENERALS_INSTALL_PATH" --env GENERALS_FPS_LIMIT="$GENERALS_FPS_LIMIT" --args $GAME_ARGS
    else
        open -W -n "$GAME_BUNDLE" --stdout "$PWD/Platform/MacOS/Build/Logs/game.log" --stderr "$PWD/Platform/MacOS/Build/Logs/game.log" --env GENERALS_INSTALL_PATH="$GENERALS_INSTALL_PATH" --env GENERALS_FPS_LIMIT="$GENERALS_FPS_LIMIT"
    fi
fi

if [ "$DO_CRC_LOGS" = true ]; then
    echo "Gathering diagnostic logs from $GENERALS_INSTALL_PATH to .agent/temp_mac_logs/..."
    mkdir -p "$PWD/.agent/temp_mac_logs"
    # Copy (not move) so the logs remain in the game dir for later inspection;
    # the purge step before the next run clears them so nothing accumulates.
    find "$GENERALS_INSTALL_PATH" -maxdepth 2 -type f \( -name "*Diag*.txt" -o -name "*Log*.txt" -o -name "*Debug*.txt" \) -exec cp {} "$PWD/.agent/temp_mac_logs/" \; 2>/dev/null
    
    if [ -d "$PWD/CRCLogs" ]; then
        rm -rf "$PWD/.agent/temp_mac_logs/CRCLogs"
        mv "$PWD/CRCLogs" "$PWD/.agent/temp_mac_logs/"
    fi

    REPLAY_FILE="$GAME_DATA_DIR/Replays/00000000.rep"
    if [ -f "$REPLAY_FILE" ]; then
        cp -f "$REPLAY_FILE" "$PWD/.agent/temp_mac_logs/00000000.rep"
        echo "Copied last replay: 00000000.rep"
    fi
    
    echo "Logs successfully collected into .agent/temp_mac_logs/"
fi
