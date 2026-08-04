#!/bin/bash

# Run the assembled Generals Online.app with logs piped to terminal.
#
# Run:
#   sh run_launcher.sh
#   sh run_launcher.sh --refresh_swift             # recompile the Swift launcher first
#   sh run_launcher.sh 2>&1 | tee launcher.log     # also save to file

LAUNCHER_NAME="GeneralsLauncher"
APP_DIR="build/dist/Generals Online.app"
APP_PATH="$APP_DIR/Contents/MacOS/$LAUNCHER_NAME"
REFRESH_SWIFT=0

for arg in "$@"; do
    case "$arg" in
        --refresh_swift) REFRESH_SWIFT=1 ;;
        *) echo "🚨 ERROR: Unknown option: $arg"; exit 1 ;;
    esac
done

if [ ! -d "$APP_DIR" ]; then
    echo "🚨 ERROR: Distribution bundle not found at: $APP_DIR"
    echo "Run 'sh assemble_distribution.sh' first."
    exit 1
fi

if [ "$REFRESH_SWIFT" -eq 1 ]; then
    echo "🔨 Recompiling the Swift launcher..."
    killall "$LAUNCHER_NAME" 2>/dev/null || true

    swiftc $(find Sources -name "*.swift") \
           -o "$APP_PATH" \
           -target arm64-apple-macosx11.0

    if [ $? -ne 0 ]; then
        echo "❌ Swift compilation failed!"
        exit 1
    fi

    echo "🔏 Re-signing the bundle..."
    codesign --force --deep -s - "$APP_DIR"
fi

if [ ! -f "$APP_PATH" ]; then
    echo "🚨 ERROR: Launcher not found at: $APP_PATH"
    echo "Run 'sh assemble_distribution.sh' first."
    exit 1
fi

killall "$LAUNCHER_NAME" 2>/dev/null || true

echo "🚀 Launching from distribution bundle..."
echo "==========================================="
"$APP_PATH"
