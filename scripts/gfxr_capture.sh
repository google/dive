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

# Launch GFXR capture without needing root permissions.
#
# Usage: ./scripts/gfxr_capture.sh PACKAGE ACTIVITY
#
# PACKAGE must be debuggable.

set -eux

PACKAGE="$1"
ACTIVITY="$2"

REMOTE_TEMP_DIR=/data/local/tmp
GFXR_CAPTURE_REMOTE_DIR=/sdcard/Download/gfxr_capture
GFXR_CAPTURE_LIB=libVkLayer_gfxreconstruct.so

# Install GFXR layer.
# Adapted from https://developer.android.com/ndk/guides/graphics/validation-layer.
adb push "./install/gfxr_layer/jni/arm64-v8a/${GFXR_CAPTURE_LIB}" "${REMOTE_TEMP_DIR}"
adb shell run-as "${PACKAGE}" cp "${REMOTE_TEMP_DIR}/${GFXR_CAPTURE_LIB}" .
adb shell settings put global enable_gpu_debug_layers 1
adb shell settings put global gpu_debug_app "${PACKAGE}"
adb shell settings put global gpu_debug_layers VK_LAYER_LUNARG_gfxreconstruct
adb shell settings put global gpu_debug_layer_app "${PACKAGE}"

# Set up GFXR.
# See //third_party/gfxreconstruct/USAGE_android.md for more options.
adb shell mkdir -p "${GFXR_CAPTURE_REMOTE_DIR}"
adb shell setprop debug.gfxrecon.capture_file "${GFXR_CAPTURE_REMOTE_DIR}/${PACKAGE}.gfxr"
adb shell setprop debug.gfxrecon.capture_trigger_frames 1
adb shell setprop debug.gfxrecon.capture_android_trigger false
adb shell setprop debug.gfxrecon.capture_use_asset_file true
adb shell appops set "${PACKAGE}" MANAGE_EXTERNAL_STORAGE allow

adb shell am start -S -W -n "${PACKAGE}/${ACTIVITY}"

while true; do
  # Let the user dictate which frame they want the capture.
  echo "Press key g+enter to trigger a capture and g+enter to retrieve the capture."
  echo "Press any other key+enter to stop the application."
  read reply
  if [ "${reply}" != "g" ]; then
    break
  fi

  adb shell setprop debug.gfxrecon.capture_android_trigger true

  # Don't know when the capture has finished so defer to the user.
  # Ideally, they have `adb logcat -s gfxrecon` running in another terminal.
  echo "Press any key to retrieve capture"
  read
  adb shell setprop debug.gfxrecon.capture_android_trigger false
  # Easiest to just pull the entire directory.
  adb pull "${GFXR_CAPTURE_REMOTE_DIR}"
  # Clean up so that we're not constantly pulling the same files.
  # Also causes names to be printed to output so the user knows.
  for file in $(adb shell ls "${GFXR_CAPTURE_REMOTE_DIR}"); do  
    adb shell rm "${GFXR_CAPTURE_REMOTE_DIR}/${file}"
  done
done

adb shell rm -rf "${GFXR_CAPTURE_REMOTE_DIR}"

# Next launch should not use GFXR
adb shell setprop debug.gfxrecon.capture_file '""'
adb shell setprop debug.gfxrecon.capture_trigger_frames '""'
adb shell setprop debug.gfxrecon.capture_android_trigger '""'
adb shell setprop debug.gfxrecon.capture_use_asset_file '""'
adb shell settings delete global enable_gpu_debug_layers
adb shell settings delete global gpu_debug_app
adb shell settings delete global gpu_debug_layers
adb shell settings delete global gpu_debug_layer_app
adb shell rm -f "${REMOTE_TEMP_DIR}/${GFXR_CAPTURE_LIB}"
adb shell run-as "${PACKAGE}" rm -f "${GFXR_CAPTURE_LIB}"

adb shell am force-stop "${PACKAGE}"
