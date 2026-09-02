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

VERSION="2.3.2"
BUILD="20"

if [ -f ".env" ]; then
    set -a
    . ./.env
    set +a
fi

LAUNCHER_NAME="GeneralsLauncher"
FINAL_APP_NAME="Generals Online"
CMAKE_APP_DIR="../../../build/macos/GeneralsMD/GeneralsOnlineZH.app"
CMAKE_VANILLA_APP_DIR="../../../build/macos/Generals/GeneralsVanilla.app"
VANILLA_BINARY_NAME="GeneralsVanilla"
DIST_DIR="build/dist"
OUTPUTS_DIR="outputs"
FINAL_APP_DIR="$DIST_DIR/$FINAL_APP_NAME.app"
ZIP_NAME="Generals_Online_Mac_Alpha.zip"
DMG_NAME="Generals_Online_Mac_Alpha.dmg"
INSTRUCTIONS_NAME="Instructions.html"
INSTRUCTIONS_BUILDER="../../../Dependencies/general_online_zh/build.sh"

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
VANILLA_BINARY="$MACOS_DIR/$VANILLA_BINARY_NAME"
GNS_SEARCH_PATH="../../../build/macos/bin"

# Both games share this shell: their Resources are byte-identical and classic
# Generals carries no libraries of its own, so it needs only its binary here.
if [ -f "$CMAKE_VANILLA_APP_DIR/Contents/MacOS/$VANILLA_BINARY_NAME" ]; then
    echo "📂 Adding classic Generals binary..."
    cp "$CMAKE_VANILLA_APP_DIR/Contents/MacOS/$VANILLA_BINARY_NAME" "$VANILLA_BINARY"
else
    echo "⚠️ Warning: $CMAKE_VANILLA_APP_DIR not built, shipping Zero Hour only."
    VANILLA_BINARY=""
fi

echo "📦 [1/7] Bundling third-party dynamic libraries..."
export PATH="/opt/homebrew/bin:$PATH"

if ! command -v dylibbundler &>/dev/null; then
    echo "🚨 ERROR: dylibbundler not found. Install with: brew install dylibbundler"
    exit 1
fi

if [ -n "$VANILLA_BINARY" ]; then
    dylibbundler -od -b \
        -x "$GAME_BINARY" \
        -x "$VANILLA_BINARY" \
        -d "$FRAMEWORKS_DIR" \
        -p @executable_path/../Frameworks/ \
        -s "$GNS_SEARCH_PATH"
else
    dylibbundler -od -b \
        -x "$GAME_BINARY" \
        -d "$FRAMEWORKS_DIR" \
        -p @executable_path/../Frameworks/ \
        -s "$GNS_SEARCH_PATH"
fi

if [ $? -ne 0 ]; then
    echo "❌ dylibbundler failed!"
    exit 1
fi

echo "🔒 [2/7] Cleaning RPATHs..."

# dylibbundler modifies all dylibs in Frameworks/, including libEOSSDK-Mac-Shipping.dylib
# (EAC plugin dependency), corrupting its original code signature.
# Restore the original from the CMake build output.
EOS_SDK_BUILD="$CMAKE_APP_DIR/Contents/Frameworks/libEOSSDK-Mac-Shipping.dylib"
if [ -f "$EOS_SDK_BUILD" ]; then
    cp -f "$EOS_SDK_BUILD" "$FRAMEWORKS_DIR/libEOSSDK-Mac-Shipping.dylib"
fi

retarget_rpaths() {
    binary="$1"
    existing=$(otool -l "$binary" | grep -A 2 LC_RPATH | awk '/path / {print $2}')
    for rp in $existing; do
        while install_name_tool -delete_rpath "$rp" "$binary" 2>/dev/null; do true; done
    done
    install_name_tool -add_rpath "@executable_path/../Frameworks/" "$binary"
}

retarget_rpaths "$GAME_BINARY"
if [ -n "$VANILLA_BINARY" ]; then
    retarget_rpaths "$VANILLA_BINARY"
fi

echo "🔨 [3/7] Compiling Swift Launcher into the package..."
swiftc $(find Sources -name "*.swift") \
       -o "$MACOS_DIR/$LAUNCHER_NAME" \
       -target arm64-apple-macosx11.0

if [ $? -ne 0 ]; then
    echo "❌ Swift compilation failed!"
    exit 1
fi

echo "🎨 [4/7] Injecting Launcher UI assets and patching..."
cp assets/background.png "$RESOURCES_DIR/background.png" 2>/dev/null || true
cp assets/background_mod.png "$RESOURCES_DIR/background_mod.png" 2>/dev/null || true
cp assets/dir_image.png "$RESOURCES_DIR/dir_image.png" 2>/dev/null || true
cp assets/author_logo.png "$RESOURCES_DIR/author_logo.png" 2>/dev/null || true
cp assets/medallion_logo.png "$RESOURCES_DIR/medallion_logo.png" 2>/dev/null || true
cp assets/Install_Final.bmp "$RESOURCES_DIR/Install_Final.bmp" 2>/dev/null || true
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

if [ -n "$GO_GA_MEASUREMENT_ID" ] && [ -n "$GO_GA_API_SECRET" ]; then
    /usr/libexec/PlistBuddy -c "Delete :GAMeasurementId" "$PLIST_FILE" 2>/dev/null || true
    /usr/libexec/PlistBuddy -c "Add :GAMeasurementId string $GO_GA_MEASUREMENT_ID" "$PLIST_FILE"
    /usr/libexec/PlistBuddy -c "Delete :GAApiSecret" "$PLIST_FILE" 2>/dev/null || true
    /usr/libexec/PlistBuddy -c "Add :GAApiSecret string $GO_GA_API_SECRET" "$PLIST_FILE"
    echo "   Analytics: enabled ($GO_GA_MEASUREMENT_ID)"
else
    echo "   Analytics: disabled (no .env keys)"
fi

echo "🔏 [5/7] Signing the finished bundle..."
# Signing has to come last: compiling the launcher in and editing Info.plist
# both invalidate an earlier seal.
codesign --force --deep -s - "$FINAL_APP_DIR"

echo "📝 [6/7] Building HTML instructions..."
# The generator lives in the site repository, which is an optional dependency
# here. Without it the checked-in www/instructions.html ships as it is.
if [ -f "$INSTRUCTIONS_BUILDER" ]; then
    sh "$INSTRUCTIONS_BUILDER"
else
    echo "⚠️ Warning: $INSTRUCTIONS_BUILDER not found, shipping www/instructions.html as is."
fi

if [ -f "www/instructions.html" ]; then
    cp "www/instructions.html" "$OUTPUTS_DIR/$INSTRUCTIONS_NAME"
else
    echo "⚠️ Warning: www/instructions.html not found, skipping HTML instructions."
fi

echo "🗜️ [7/7] Creating final deployment ZIP..."
# Enter dist so the archive carries the app itself at its root, without build/dist
cd "$DIST_DIR" || exit
zip -qry "../../$OUTPUTS_DIR/$ZIP_NAME" "$FINAL_APP_NAME.app"
cd ../..

# Enter outputs and add the instructions inside the finished ZIP
cd "$OUTPUTS_DIR" || exit
if [ -f "$INSTRUCTIONS_NAME" ]; then
    zip -rq "$ZIP_NAME" "$INSTRUCTIONS_NAME"
fi
cd ..

# echo "💿 [7/7] Creating DMG installer image..."
# sh build_dmg.sh

echo "✅ Distribution package successfully created in: $OUTPUTS_DIR"
ls -lah "$OUTPUTS_DIR"

# Drop build_launcher.sh now that everything is assembled here
rm -f build_launcher.sh 2>/dev/null || true
