#!/bin/bash

# Usage: replay-with-dump.sh GFXR GFXA

set -eux

GFXR=$1
GFXR_BASENAME=$(basename "$GFXR")
GFXA=$2
JSON=dump.json
REMOTE_TEMP_DIR=/sdcard/Download
PUSH_DIR="$REMOTE_TEMP_DIR/replay"
DUMP_DIR="$REMOTE_TEMP_DIR/dump"
GFXR_DUMP_RESOURCES=$(find ./build -name gfxr_dump_resources -executable -type f)
GFXRECON=./third_party/gfxreconstruct/android/scripts/gfxrecon.py

$GFXR_DUMP_RESOURCES "$GFXR" "$JSON"
adb shell mkdir -p "$PUSH_DIR"
adb shell mkdir -p "$DUMP_DIR"
adb push "$GFXR" "$GFXA" "$JSON" "$PUSH_DIR"
python "$GFXRECON" replay \
    --dump-resources "$PUSH_DIR/$JSON" \
    --dump-resources-dir "$DUMP_DIR" \
    "$PUSH_DIR/$GFXR_BASENAME"

adb logcat -d -s gfxrecon

adb pull "$DUMP_DIR"
