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

set -eux

COMPOSITOR_CAPTURE_DIR=/data/local/tmp/gfxr_capture/compositor
TIME_TO_WAIT_FOR_RESTART=30

setup() {
  adb root
  adb shell setenforce 0

  # setup gfxr layers
  adb shell mkdir -p /data/local/debug/vulkan/
  adb push install/gfxr_layer/jni/arm64-v8a/libVkLayer_gfxreconstruct.so /data/local/debug/vulkan
  adb shell mkdir -p  ${COMPOSITOR_CAPTURE_DIR}
  adb shell setprop debug.gfxrecon.capture_file "${COMPOSITOR_CAPTURE_DIR}/compositor.gfxr"
  adb shell setprop debug.gfxrecon.capture_trigger_frames 1
  adb shell setprop debug.gfxrecon.capture_android_trigger false
  adb shell setprop debug.gfxrecon.capture_use_asset_file true

  # compositor settings
  adb shell setprop persist.compositor.protected_context false
  adb shell setprop persist.compositor.tiled_rendering false
  adb shell setprop compositor.lazy_depth_buffer false

  adb shell setprop cpm.gfxr_layer 1
  adb shell stop && adb shell start
  # TODO(b/449739284): find a better way to detect surfaceflinger done initialization.
  echo -n "Waiting for surfaceflinger to restart... "
  for i in $(seq ${TIME_TO_WAIT_FOR_RESTART} -1 1); do
      echo -n "$i, "
      sleep 1
  done
  echo "done."
}

trigger_capture() {
  # Copied from the gfxr_capture.sh script.
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
    adb pull ${COMPOSITOR_CAPTURE_DIR} ./captures
    # Clean up so that we're not constantly pulling the same files.
    # Also causes names to be printed to output so the user knows.
    for file in $(adb shell ls "${COMPOSITOR_CAPTURE_DIR}" | tr -d '\r'); do
      adb shell rm "${COMPOSITOR_CAPTURE_DIR}/${file}"
    done
  done
}

cleanup() {
  adb shell rm -rf ${COMPOSITOR_CAPTURE_DIR}
  adb shell rm /data/local/debug/vulkan/libVkLayer_gfxreconstruct.so
  adb shell setprop cpm.gfxr_layer 0
  adb shell setprop persist.compositor.protected_context "\"\""
  adb shell setprop persist.compositor.tiled_rendering "\"\""
  adb shell setprop compositor.lazy_depth_buffer "\"\""
  adb shell stop && adb shell start
}

is_passthrough() {
  adb shell dumpsys cpm | \
  grep -A 2 "XrPassthroughController:" | \
  awk '/running:/ { exit ($2 == "true" ? 0 : 1) }'
}

setup
if is_passthrough; then 
  echo "*** Warning: Capturing in passthrough mode is not supported. ***"
  cleanup
  exit 1;
fi

trigger_capture
cleanup