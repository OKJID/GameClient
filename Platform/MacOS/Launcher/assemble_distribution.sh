#!/bin/bash

# Assemble the final macOS distribution package (.zip + .dmg)
# Requires a successful CMake build of the game first.
#
# Run standalone (from Platform/MacOS/Launcher/):
#   sh assemble_distribution.sh
#
# Or via the root build script:
#   sh build_mac.sh --launcher            # build + assemble
#   sh build_mac.sh --launcher --clean    # clean build + assemble
#
# Prerequisites:
#   - dylibbundler  (brew install dylibbundler)
#   - create-dmg    (brew install create-dmg)  — optional, for premium DMG

VERSION="1.2.0"
BUILD="4"

LAUNCHER_NAME="GeneralsLauncher"
FINAL_APP_NAME="Generals Online"
CMAKE_APP_DIR="../../../build/macos/GeneralsMD/GeneralsOnlineZH.app"
DIST_DIR="build/dist"
OUTPUTS_DIR="outputs"
FINAL_APP_DIR="$DIST_DIR/$FINAL_APP_NAME.app"
ZIP_NAME="Generals_Online_Mac_Alpha.zip"
DMG_NAME="Generals_Online_Mac_Alpha.dmg"
INSTRUCTIONS_NAME="Instructions.html"

echo "=========================================="
echo "📦 Assembling Final Distribution Package"
echo "=========================================="

echo "🧹 Cleaning previous distribution..."
killall "$LAUNCHER_NAME" 2>/dev/null || true
rm -rf "$DIST_DIR" "$OUTPUTS_DIR"
mkdir -p "$DIST_DIR"
mkdir -p "$OUTPUTS_DIR"

if [ ! -d "$CMAKE_APP_DIR" ]; then
    echo "🚨 ERROR: CMake game build not found at $CMAKE_APP_DIR!"
    echo "Please build the game using CMake first."
    exit 1
fi

echo "📂 Copying game build to distribution folder..."
cp -R "$CMAKE_APP_DIR" "$FINAL_APP_DIR"

CONTENTS_DIR="$FINAL_APP_DIR/Contents"
MACOS_DIR="$CONTENTS_DIR/MacOS"
RESOURCES_DIR="$CONTENTS_DIR/Resources"
FRAMEWORKS_DIR="$CONTENTS_DIR/Frameworks"
GAME_BINARY="$MACOS_DIR/GeneralsOnlineZH"
GNS_SEARCH_PATH="../../../build/macos/bin"

echo "📦 [1/7] Bundling third-party dynamic libraries..."
export PATH="/opt/homebrew/bin:$PATH"

if ! command -v dylibbundler &>/dev/null; then
    echo "🚨 ERROR: dylibbundler not found. Install with: brew install dylibbundler"
    exit 1
fi

dylibbundler -od -b \
    -x "$GAME_BINARY" \
    -d "$FRAMEWORKS_DIR" \
    -p @executable_path/../Frameworks/ \
    -s "$GNS_SEARCH_PATH"

if [ $? -ne 0 ]; then
    echo "❌ dylibbundler failed!"
    exit 1
fi

echo "🔒 [2/7] Cleaning RPATHs and re-signing..."
EXISTING_RPATHS=$(otool -l "$GAME_BINARY" | grep -A 2 LC_RPATH | awk '/path / {print $2}')
for rp in $EXISTING_RPATHS; do
    while install_name_tool -delete_rpath "$rp" "$GAME_BINARY" 2>/dev/null; do true; done
done
install_name_tool -add_rpath "@executable_path/../Frameworks/" "$GAME_BINARY"

codesign --force --deep -s - "$FINAL_APP_DIR"

echo "🔨 [3/7] Compiling Swift Launcher into the package..."
swiftc Sources/LauncherApp.swift Sources/MainView.swift \
       Sources/SteamCMDManager.swift Sources/AboutWindow.swift \
       Sources/UpdateChecker.swift \
       -o "$MACOS_DIR/$LAUNCHER_NAME" \
       -target arm64-apple-macosx11.0

if [ $? -ne 0 ]; then
    echo "❌ Swift compilation failed!"
    exit 1
fi

echo "🎨 [4/7] Injecting Launcher UI assets and patching..."
cp assets/background.png "$RESOURCES_DIR/background.png" 2>/dev/null || true
cp assets/dir_image.png "$RESOURCES_DIR/dir_image.png" 2>/dev/null || true
cp assets/author_logo.png "$RESOURCES_DIR/author_logo.png" 2>/dev/null || true
cp Generals.png "$RESOURCES_DIR/AppIcon.png" 2>/dev/null || true

PLIST_FILE="$CONTENTS_DIR/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleExecutable $LAUNCHER_NAME" "$PLIST_FILE"
/usr/libexec/PlistBuddy -c "Set :CFBundleName $FINAL_APP_NAME" "$PLIST_FILE"
/usr/libexec/PlistBuddy -c "Delete :CFBundleIconFile" "$PLIST_FILE" 2>/dev/null || true
/usr/libexec/PlistBuddy -c "Add :CFBundleIconFile string AppIcon.png" "$PLIST_FILE"

/usr/libexec/PlistBuddy -c "Delete :GOLauncherVersion" "$PLIST_FILE" 2>/dev/null || true
/usr/libexec/PlistBuddy -c "Add :GOLauncherVersion string $VERSION" "$PLIST_FILE"
/usr/libexec/PlistBuddy -c "Delete :GOLauncherBuild" "$PLIST_FILE" 2>/dev/null || true
/usr/libexec/PlistBuddy -c "Add :GOLauncherBuild string $BUILD" "$PLIST_FILE"
echo "   Launcher version: v$VERSION (build $BUILD)"

echo "📝 [5/7] Copying HTML instructions..."
if [ -f "www/instructions.html" ]; then
    cp "www/instructions.html" "$OUTPUTS_DIR/$INSTRUCTIONS_NAME"
else
    echo "⚠️ Warning: www/instructions.html not found, skipping HTML instructions."
fi

echo "🗜️ [6/7] Creating final deployment ZIP..."
# Идем в dist, чтобы в архиве корневым элементом была сама app, без папок build/dist
cd "$DIST_DIR" || exit
zip -qry "../../$OUTPUTS_DIR/$ZIP_NAME" "$FINAL_APP_NAME.app"
cd ../..

# Идем в outputs и добавляем инструкции внутрь готового зипа
cd "$OUTPUTS_DIR" || exit
if [ -f "$INSTRUCTIONS_NAME" ]; then
    zip -rq "$ZIP_NAME" "$INSTRUCTIONS_NAME"
fi
cd ..

# echo "💿 [7/7] Creating DMG installer image..."
# sh build_dmg.sh

echo "✅ Distribution package successfully created in: $OUTPUTS_DIR"
ls -lah "$OUTPUTS_DIR"

# Удаляем build_launcher.sh раз мы все объединили
rm -f build_launcher.sh 2>/dev/null || true
