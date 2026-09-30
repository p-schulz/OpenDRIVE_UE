#!/usr/bin/env bash
# Compiles the OpenDrive plugin's engine-independent runtime code (FOpenDriveMap, FOpenDriveWriter,
# FOpenDriveModelEdit, UOpenDriveAsset) against a tiny mock of the Unreal core types and runs sanity
# checks. No Unreal Engine installation required. Mirrors OpenScenario_UE's Tests/Standalone harness.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="$HERE/../../Source/OpenDrive"
OUT="${TMPDIR:-/tmp}/opendrive_standalone_test"

${CXX:-g++} -std=c++20 -Wall -Wextra -Wno-unused-parameter -Wno-misleading-indentation \
  -I "$HERE/MockUE" -I "$SRC/Public" -I "$SRC/Private" "$HERE/main.cpp" \
  "$SRC/Private/OpenDriveModule.cpp" "$SRC/Private/OpenDrive/OpenDriveMap.cpp" \
  "$SRC/Private/OpenDrive/OpenDriveAsset.cpp" "$SRC/Private/OpenDriveWriter.cpp" \
  "$SRC/Private/OpenDriveModelEdit.cpp" -o "$OUT"

"$OUT"
