#!/bin/bash

# Run:
#   sh build_mac.sh                       # build release version
#   sh build_mac.sh --launcher            # build release version with launcher
#   sh build_mac.sh --launcher --clean    # clean + build release version with launcher

# build_mac.sh - Release Build Script for macOS
# This script performs a clean release build of the macOS port.

echo "========================================="
echo " macOS Release Build - GameClient "
echo "========================================="

# Stop on first error
set -e

BUILD_LAUNCHER=false
DO_CLEAN=false

for arg in "$@"; do
    case "$arg" in
        --launcher)
            BUILD_LAUNCHER=true
            ;;
        --clean)
            DO_CLEAN=true
            ;;
    esac
done

# Ensure Homebrew and Ninja are in PATH
export PATH="/opt/homebrew/bin:$PATH"

# 1. Disable debug logging completely for optimal performance
export GENERALS_MAC_DEBUG=0

BUILD_DIR="build/macos"

if [ "$DO_CLEAN" = true ]; then
    echo "[1/3] Cleaning previous build files..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
fi

echo "[2/3] Configuring CMake for Release (preset: macos)..."
# The 'macos' preset in CMakePresets.json automatically sets CMAKE_BUILD_TYPE=Release
cmake --preset macos

echo "[3/3] Building the project..."
# Ninja automatically parallelizes the build based on available CPU cores
cmake --build "$BUILD_DIR" --config Release

echo "========================================="
echo "✅ Release build completed successfully!"
echo "App bundle path: $BUILD_DIR/GeneralsMD/GeneralsOnlineZH.app"

if [ "$BUILD_LAUNCHER" = true ]; then
    echo "========================================="
    echo "[4/3] Assembling distribution with Launcher..."
    cd Platform/MacOS/Launcher
    sh assemble_distribution.sh
    cd ../../../
fi

echo "========================================="
