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

# Usage: replay-with-dump.sh GFXR [GFXA]
#
# If you have more than one adb device, set ANDROID_SERIAL.
# GFXR is required. If the capture doesn't use the asset file, GFXA is optional

set -eux

# Fairly reliable directory on remote device, as long as app has MANAGE_EXTERNAL_STORAGE permissions.
# /data/local/tmp doesn't work on all devices tested.
REMOTE_TEMP_DIR=/sdcard/Download
PUSH_DIR="$REMOTE_TEMP_DIR/replay"
DUMP_DIR="$REMOTE_TEMP_DIR/dump"

cleanup() {
    adb shell rm -rf -- "$DUMP_DIR"
    adb shell rm -rf -- "$PUSH_DIR"
}

# Ensure device is in a safe, usable state when the script terminates
trap cleanup EXIT

if [ $# -lt 1 ]; then
    echo "Usage: replay-with-dump.sh GFXR [GFXA]"
    exit 1
fi

GFXR=$1
GFXR_BASENAME=$(basename "$GFXR")
BUILD_DIR=build
JSON_BASENAME=dump.json
JSON="$BUILD_DIR/$JSON_BASENAME"
GFXR_DUMP_RESOURCES=$(find "$BUILD_DIR" -name gfxr_dump_resources -executable -type f)
DIVE_CLIENT_CLI=$(find "$BUILD_DIR" -name dive_client_cli -executable -type f)

$GFXR_DUMP_RESOURCES --last_draw_only "$GFXR" "$JSON"
adb shell mkdir -p "$PUSH_DIR"
adb shell mkdir -p "$DUMP_DIR"
adb push "$GFXR" "$JSON" "$PUSH_DIR"
if [ $# -eq 2 ]; then
    GFXA="$2"
    adb push "$GFXA" "$PUSH_DIR"
fi
$DIVE_CLIENT_CLI --command gfxr_replay \
    --gfxr_replay_file_path "$PUSH_DIR/$GFXR_BASENAME" \
    --gfxr_replay_flags "--dump-resources $PUSH_DIR/$JSON_BASENAME --dump-resources-dir $DUMP_DIR --dump-resources-dump-all-image-subresources"

adb pull "$DUMP_DIR"
echo "Results in $DUMP_DIR"
