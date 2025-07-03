#!/bin/bash

# Copyright 2025 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Usage: replay-with-dump.sh GFXR GFXA
#
# If you have more than one adb device, set ANDROID_SERIAL

set -eux

GFXR=$1
GFXR_BASENAME=$(basename "$GFXR")
GFXA=$2
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
