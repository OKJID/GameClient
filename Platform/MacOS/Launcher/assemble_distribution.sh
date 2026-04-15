#!/bin/bash

LAUNCHER_NAME="GeneralsLauncher"
FINAL_APP_NAME="Generals Online"
CMAKE_APP_DIR="../../../build/macos/GeneralsMD/GeneralsOnlineZH.app"
DIST_DIR="build/dist"
OUTPUTS_DIR="outputs"
FINAL_APP_DIR="$DIST_DIR/$FINAL_APP_NAME.app"
ZIP_NAME="Generals_Online_Mac_Alpha.zip"
README_NAME="README_INSTALL.md"

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

echo "📦 [1/6] Bundling third-party dynamic libraries..."
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

echo "🔒 [2/6] Cleaning RPATHs and re-signing..."
EXISTING_RPATHS=$(otool -l "$GAME_BINARY" | grep -A 2 LC_RPATH | awk '/path / {print $2}')
for rp in $EXISTING_RPATHS; do
    while install_name_tool -delete_rpath "$rp" "$GAME_BINARY" 2>/dev/null; do true; done
done
install_name_tool -add_rpath "@executable_path/../Frameworks/" "$GAME_BINARY"

codesign --force --deep -s - "$FINAL_APP_DIR"

echo "🔨 [3/6] Compiling Swift Launcher into the package..."
swiftc Sources/LauncherApp.swift Sources/MainView.swift \
       -o "$MACOS_DIR/$LAUNCHER_NAME" \
       -target arm64-apple-macosx11.0

if [ $? -ne 0 ]; then
    echo "❌ Swift compilation failed!"
    exit 1
fi

echo "🎨 [4/6] Injecting Launcher UI assets and patching..."
cp assets/background.png "$RESOURCES_DIR/background.png" 2>/dev/null || true
cp assets/dir_image.png "$RESOURCES_DIR/dir_image.png" 2>/dev/null || true
cp assets/author_logo.png "$RESOURCES_DIR/author_logo.png" 2>/dev/null || true
cp Generals.png "$RESOURCES_DIR/AppIcon.png" 2>/dev/null || true

PLIST_FILE="$CONTENTS_DIR/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleExecutable $LAUNCHER_NAME" "$PLIST_FILE"
/usr/libexec/PlistBuddy -c "Set :CFBundleName $FINAL_APP_NAME" "$PLIST_FILE"
/usr/libexec/PlistBuddy -c "Delete :CFBundleIconFile" "$PLIST_FILE" 2>/dev/null || true
/usr/libexec/PlistBuddy -c "Add :CFBundleIconFile string AppIcon.png" "$PLIST_FILE"

echo "📝 [5/6] Generating README instruction..."
cat << 'EOF' > "$OUTPUTS_DIR/$README_NAME"
# Command and Conquer Generals – Mac OS Port 🍏

### ⚠️ Installation & Launch Instructions

Since this application is a free community port and lacks an official paid Apple Developer certificate, the macOS security system (Gatekeeper) will place the downloaded archive and the app into "quarantine". It may say the file is damaged or cannot be opened.

To remove this restriction, you need to run **one** simple command in the Terminal:

1. Unzip the downloaded ZIP archive.
2. Open the system application **"Terminal"** (Terminal.app).
3. Enter the following command (it will ask for your Mac administrator password):

```bash
sudo xattr -cr "Path to/Generals Online.app"
```

> **Tip:** You can just type `sudo xattr -cr ` (make sure there is a space at the end) and drag the unzipped game app directly into the Terminal window. The path will be inserted automatically!

4. Press **Enter** and type your password (characters will be hidden while typing).

After that, you will be able to launch **Generals Online** with a regular double-click.

---

### 🚀 Terminal Quick-Start (For Advanced Users)

If you prefer using the Terminal for the entire process, navigate to the folder where you downloaded the ZIP file and run this one-liner:

```bash
unzip -d Generals_Online_Mac_Alpha Generals_Online_Mac_Alpha.zip && cd Generals_Online_Mac_Alpha && sudo xattr -cr "Generals Online.app" && open "Generals Online.app"
```

---

### 📂 Connecting Game Data

When the Launcher opens, you **MUST** select the parent folder of your Windows version game files.  
*(Inside the folder you select, there must be two subdirectories: the Vanilla version and Zero Hour).*

Have a great game, General! 🫡
EOF

echo "🗜️ [6/6] Creating final deployment ZIP..."
# Идем в dist, чтобы в архиве корневым элементом была сама app, без папок build/dist
cd "$DIST_DIR" || exit
zip -qry "../../$OUTPUTS_DIR/$ZIP_NAME" "$FINAL_APP_NAME.app"
cd ../..

# Идем в outputs и добавляем ридми внутрь готового зипа
cd "$OUTPUTS_DIR" || exit
zip -rq "$ZIP_NAME" "$README_NAME"

echo "✅ Distribution package successfully created in: $OUTPUTS_DIR"
ls -lah
cd ..

# Удаляем build_launcher.sh раз мы все объединили
rm -f build_launcher.sh 2>/dev/null || true
