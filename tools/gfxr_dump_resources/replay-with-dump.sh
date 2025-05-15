#!/bin/bash

# Usage: replay-with-dump.sh GFXR GFXA
#
# If you have more than one adb device, set ANDROID_SERIAL

set -eux

GFXR=$1
GFXR_BASENAME=$(basename "$GFXR")
GFXA=$2
# TODO write to temp dir
BUILD_DIR=build
JSON_BASENAME=dump.json
JSON="$BUILD_DIR/$JSON_BASENAME"
REMOTE_TEMP_DIR=/sdcard/Download
PUSH_DIR="$REMOTE_TEMP_DIR/replay"
DUMP_DIR="$REMOTE_TEMP_DIR/dump"
GFXR_DUMP_RESOURCES=$(find "$BUILD_DIR" -name gfxr_dump_resources -executable -type f)
GFXRECON=./third_party/gfxreconstruct/android/scripts/gfxrecon.py

adb logcat -c

$GFXR_DUMP_RESOURCES "$GFXR" "$JSON"
adb shell mkdir -p "$PUSH_DIR"
adb shell mkdir -p "$DUMP_DIR"
adb push "$GFXR" "$GFXA" "$JSON" "$PUSH_DIR"
python "$GFXRECON" replay \
    --dump-resources "$PUSH_DIR/$JSON_BASENAME" \
    --dump-resources-dir "$DUMP_DIR" \
    --dump-resources-dump-depth-attachment \
    "$PUSH_DIR/$GFXR_BASENAME"

while adb shell pidof com.lunarg.gfxreconstruct.replay
do
    sleep 1
done

adb logcat -d -s gfxrecon
adb pull "$DUMP_DIR"
adb shell rm -rf "$DUMP_DIR"
