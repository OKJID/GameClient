#!/bin/bash

# Run:
#   sh profile_mac.sh                         # wait for the game, warm up, record a trace
#   TEMPLATE='Time Profiler' sh profile_mac.sh
#   RECORD_SECONDS=60 WARMUP_SECONDS=10 sh profile_mac.sh
#
# Start the game separately (build_run_mac.sh) and stay in the scene you want measured.

set -u

PROCESS_NAME="${PROCESS_NAME:-GeneralsOnlineZH}"
TEMPLATE="${TEMPLATE:-Time Profiler}"
WARMUP_SECONDS="${WARMUP_SECONDS:-10}"
RECORD_SECONDS="${RECORD_SECONDS:-30}"
WAIT_LIMIT_SECONDS="${WAIT_LIMIT_SECONDS:-1800}"

announce() {
    echo "[$(date +%H:%M:%S)] $1"
    say "$2" >/dev/null 2>&1 &
}

TRACE_DIR="$PWD/Platform/MacOS/Build/Logs/traces"
mkdir -p "$TRACE_DIR"
TRACE_PATH="$TRACE_DIR/$(date +%Y%m%d-%H%M%S).trace"

echo "Waiting for $PROCESS_NAME (up to ${WAIT_LIMIT_SECONDS}s)..."
waited=0
while ! pgrep -x "$PROCESS_NAME" >/dev/null; do
    if [ "$waited" -ge "$WAIT_LIMIT_SECONDS" ]; then
        echo "ERROR: $PROCESS_NAME did not start within ${WAIT_LIMIT_SECONDS}s"
        exit 1
    fi
    sleep 1
    waited=$((waited + 1))
done

GAME_PID="$(pgrep -x "$PROCESS_NAME" | head -1)"
echo "Found pid $GAME_PID, warming up ${WARMUP_SECONDS}s before recording..."
sleep "$WARMUP_SECONDS"

if ! pgrep -x "$PROCESS_NAME" >/dev/null; then
    echo "ERROR: $PROCESS_NAME exited during warmup"
    exit 1
fi

announce "Recording '$TEMPLATE' for ${RECORD_SECONDS}s -> $TRACE_PATH" "Profiler is starting. Keep the game running."
xcrun xctrace record \
    --template "$TEMPLATE" \
    --attach "$GAME_PID" \
    --time-limit "${RECORD_SECONDS}s" \
    --output "$TRACE_PATH" \
    --no-prompt

if pgrep -x "$PROCESS_NAME" >/dev/null; then
    announce "Trace written: $TRACE_PATH" "Recording finished. You can close the game."
    exit 0
fi

announce "Game exited during recording: $TRACE_PATH" "Warning. The game exited before recording finished."
