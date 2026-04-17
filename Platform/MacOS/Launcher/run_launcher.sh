#!/bin/bash

# Run the assembled Generals Online.app with logs piped to terminal.
#
# Run:
#   sh run_launcher.sh
#   sh run_launcher.sh 2>&1 | tee launcher.log   # also save to file

APP_PATH="build/dist/Generals Online.app/Contents/MacOS/GeneralsLauncher"

if [ ! -f "$APP_PATH" ]; then
    echo "🚨 ERROR: Launcher not found at: $APP_PATH"
    echo "Run 'sh assemble_distribution.sh' first."
    exit 1
fi

killall GeneralsLauncher 2>/dev/null || true

echo "🚀 Launching from distribution bundle..."
echo "==========================================="
"$APP_PATH"
