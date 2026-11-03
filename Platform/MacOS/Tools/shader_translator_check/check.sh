#!/bin/bash
# Builds the DX8 shader translator check and runs it over every shader in the given sources.
#
# Usage: sh check.sh <archive.big | folder>...
#   Sources are BIG archives or loose Shaders folders (game resource, mods).
#   fixtures/*.asm hold the shaders the engine assembles from text at run time.
set -euo pipefail

cd "$(dirname "$0")"
METAL=../../Source/Metal
SRC="$METAL/ShaderTranslator"
BUILD=../../Build/ShaderTranslatorCheck
TOOL="$BUILD/shader_translator_check"

mkdir -p "$BUILD"
clang++ -std=c++17 -O1 -Wall -Wextra -I "$SRC" -I "$METAL" "$SRC"/*.cpp "$METAL/MetalShaderWrappers.cpp" main.cpp -o "$TOOL"

rm -rf "$BUILD/shaders" "$BUILD/libraries"
mkdir -p "$BUILD/shaders" "$BUILD/libraries"
python3 extract_shaders.py "$BUILD/shaders" "$@"

"$TOOL" roundtrip "$BUILD"/shaders/* fixtures/*.asm
"$TOOL" translate "$BUILD/libraries" "$METAL/MacOSShaderInterface.h" "$BUILD"/shaders/* fixtures/*.asm

failures=0
for library in "$BUILD"/libraries/*.metal; do
  if ! xcrun -sdk macosx metal -std=metal3.0 -mmacosx-version-min=13.3 -Wno-pragma-once-outside-header -Wno-unused-function \
      -c "$library" -o "${library%.metal}.air"; then
    echo "FAIL Metal compile: $library"
    failures=$((failures + 1))
  fi
done
echo "Metal compile: $failures failed of $(ls "$BUILD"/libraries/*.metal | wc -l | tr -d ' ')"
[ "$failures" -eq 0 ]
