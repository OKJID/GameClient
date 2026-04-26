#!/bin/bash

# Build a styled DMG installer image from an existing distribution.
#
# Run standalone (from Platform/MacOS/Launcher/):
#   sh build_dmg.sh
#
# Prerequisites:
#   - create-dmg  (brew install create-dmg) — optional, for premium DMG

set -e

FINAL_APP_NAME="Generals Online"
DIST_DIR="build/dist"
OUTPUTS_DIR="outputs"
DMG_NAME="Generals_Online_Mac_Alpha.dmg"
DMG_OUTPUT="$OUTPUTS_DIR/$DMG_NAME"

if [ ! -d "$DIST_DIR/$FINAL_APP_NAME.app" ]; then
    echo "🚨 ERROR: Distribution not found at $DIST_DIR/$FINAL_APP_NAME.app"
    echo "Run assemble_distribution.sh first."
    exit 1
fi

mkdir -p "$OUTPUTS_DIR"
rm -f "$DMG_OUTPUT"

echo "💿 Creating DMG installer image..."

if command -v create-dmg &>/dev/null; then
    create-dmg \
        --volname "Generals Online" \
        --volicon "Generals.png" \
        --background "assets/dmg_background.png" \
        --window-pos 200 120 \
        --window-size 660 400 \
        --icon-size 80 \
        --icon "$FINAL_APP_NAME.app" 170 190 \
        --app-drop-link 490 190 \
        --hide-extension "$FINAL_APP_NAME.app" \
        --no-internet-enable \
        "$DMG_OUTPUT" \
        "$DIST_DIR/"
else
    echo "⚠️  create-dmg not found, falling back to basic hdiutil (install with: brew install create-dmg)"
    DMG_STAGING="build/dmg_staging"
    rm -rf "$DMG_STAGING"
    mkdir -p "$DMG_STAGING"
    cp -R "$DIST_DIR/$FINAL_APP_NAME.app" "$DMG_STAGING/"
    ln -s /Applications "$DMG_STAGING/Applications"

    hdiutil create \
        -volname "Generals Online" \
        -srcfolder "$DMG_STAGING" \
        -ov \
        -format UDZO \
        "$DMG_OUTPUT"

    rm -rf "$DMG_STAGING"
fi

echo "✅ DMG created: $DMG_OUTPUT"
ls -lh "$DMG_OUTPUT"
