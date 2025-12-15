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

# Common functionality for building testing scripts

#
# Constants
#

# The basename of the Vulkan validation layer shared object for Android
readonly VALIDATION_LAYER_BASENAME="libVkLayer_khronos_validation.so"
# Name of the GFXR replay package
readonly REPLAY_PACKAGE="com.lunarg.gfxreconstruct.replay"
# Vulkan layer identifier for validation layers
readonly VALIDATION_LAYER_NAME="VK_LAYER_KHRONOS_validation"

#
# Base directories
#

# Location of project root.
# TODO derive from this script location
readonly PROJECT_SOURCE_DIR="."
# Where to find third_party source folder
readonly THIRD_PARTY_DIR="${PROJECT_SOURCE_DIR}/third_party"
# Location of the build artifacts
readonly BUILD_DIR="${PROJECT_SOURCE_DIR}/build"
# Location of the installed artifacts (after build)
readonly INSTALL_DIR="${PROJECT_SOURCE_DIR}/install"
# Fairly reliable directory on remote device, as long as app has MANAGE_EXTERNAL_STORAGE permissions.
# /data/local/tmp doesn't work on all devices tested.
readonly REMOTE_TEMP_DIR="/sdcard/Download"

#
# Derived filenames
#

# Location of the GFXR replay helper script for Android
# TODO look in install dir instead?
readonly GFXRECON="${THIRD_PARTY_DIR}/gfxreconstruct/android/scripts/gfxrecon.py"
# Location of gfxr_dump_resources
readonly GFXR_DUMP_RESOURCES="${BUILD_DIR}/gfxr_dump_resources/gfxr_dump_resources"
# The directory on the device which stores all --dump-resources output
readonly DUMP_DIR="${REMOTE_TEMP_DIR}/dump"
# Location of the installed replay APK
readonly GFXR_REPLAY_APK="${INSTALL_DIR}/gfxr-replay.apk"
# Location in the source tree of the validation layer for Android
# TODO prefer install
readonly LOCAL_VALIDATION_LAYER_FILEPATH="${THIRD_PARTY_DIR}/Vulkan-ValidationLayers/android/arm64-v8a/${VALIDATION_LAYER_BASENAME}"

#
# Helper functions
#

# Resets all modified GFXR settings. The next app launch should use the defaults.
# Usage: unset_gfxr_props
unset_gfxr_props() {
    adb shell setprop debug.gfxrecon.capture_file '""'
    adb shell setprop debug.gfxrecon.capture_trigger_frames '""'
    adb shell setprop debug.gfxrecon.capture_android_trigger '""'
    adb shell setprop debug.gfxrecon.capture_use_asset_file '""'
    adb shell setprop debug.gfxrecon.quit_after_capture_frames '""'
    adb shell setprop debug.gfxrecon.capture_file_timestamp '""'
    adb shell setprop debug.gfxrecon.capture_frames '""'
    adb shell setprop debug.gfxrecon.log_level '""'
    adb shell setprop debug.gfxrecon.capture_file_flush '""'
}

# Resets the Android debug settings so that the next app launch does not have layers injected.
# Usage: unset_vulkan_debug_settings
unset_vulkan_debug_settings() {
    adb shell settings delete global enable_gpu_debug_layers
    adb shell settings delete global gpu_debug_app
    adb shell settings delete global gpu_debug_layers
    adb shell settings delete global gpu_debug_layer_app
}

# Check if the logcat contains a string. Returns 0 on successful find, 1 otherwise
# Usage: logcat_has_line TAG LINE
# Example: if logcat_has_line gfxrecon "Finished recording graphics API capture"; then echo done capturing; fi
logcat_has_line() {
    test $(adb logcat -s "$1" -e "$2" -d | wc -l) -gt 0
}

# Check if something crashed. Returns 0 if something crashed, 1 otherwise
# Usage: is_crash_detected
# Example: if is_crash_detected; then adb bugreport; fi
is_crash_detected() {
    logcat_has_line "DEBUG" "\*\*\* \*\*\* \*\*\*"
}

# Block execution until a specific line is printed to logcat from an app with a given tag, or a 20s timeout is reached.
# If the line was found, returns 0. Otherwise, if the timeout was reached, returns 1.
# Usage: wait_for_logcat_line TAG LINE
# Example: wait_for_logcat_line gfxrecon "Finished recording graphics API capture"
wait_for_logcat_line() {
    # Could use adb logcat -s "$1" -e "$2" --print but this blocks without timeout. Poll instead.
    # 30s should be long enough for the app to start and produce a capture. If not then something is likely wrong. 20s was too short if you wanted to capture with debugging.
    for i in {1..30}
    do
        sleep 1
        if logcat_has_line "$1" "$2"
        then
            return 0
        fi
    done
    return 1
}

# Get the main activity from a package, if it exists
# Usage: find_default_activity PACKAGE
# Example: ACTIVITY=$(find_default_activity com.google.bigwheels.project_xube_xr.debug) # ACTIVITY should be com.google.bigwheels.MainActivity
# TODO: If "No activity found", exit status should be > 0
find_default_activity() {
    adb shell cmd package resolve-activity "$1" | grep "name=" | head -n1 | sed 's/.*name=//'
}

# Exit status is 0 if an app is installed, otherwise non-0
# Usage: is_app_installed PACKAGE
# Example: if is_app_installed com.lunarg.gfxreconstruct.replay; then echo app is installed; fi
is_app_installed() {
    adb shell pm path "$1" 2>&1 /dev/null
}

# If the app is installed, return the path to the APK on the device
# Usage: get_app_path PACKAGE
# Example: REMOTE_APK_PATH=$(get_app_path com.lunarg.gfxreconstruct.replay) # REMOTE_APK_PATH should be similar to /data/app/~~mijWjamWRSB7_rLBy-xQsA==/com.lunarg.gfxreconstruct.replay-K6xMsr7YlvENFRXe82FdRQ==/base.apk
get_app_path() {
    adb shell pm path "$1" | sed 's/package://'
}

# Pushes a file into app internal storage.
# Usage: inject_layer APP LIB.SO
# EXAMPLE: inject_layer com.lunarg.gfxreconstruct.replay ./install/libVkLayer_khronos_validation.so
inject_layer() {
    local -r app="$1"
    local -r lib="$2"
    # This dir is deleted on reboot
    local -r remote_volatile_tmp="/data/local/tmp/$(basename "${lib}")"
    adb push "${lib}" "${remote_volatile_tmp}"
    # Can't mv since remote_volatile_tmp is owned by shell or root
    # NOTE: run-as only works for debuggable apps (even if system is userdebug)
    adb shell run-as "${app}" cp "${remote_volatile_tmp}" .
    # TODO ensure rm is run even if this fails
    adb shell rm -rf "${remote_volatile_tmp}"
}

# Removes a layer installed by inject_layer
# Usage: uninstall_layer APP LIB_BASENAME
# Example: uninstall_layer com.lunarg.gfxreconstruct.replay libVkLayer_khronos_validation.so
uninstall_layer() {
    local -r app="$1"
    local -r lib_basename="$2"
    adb shell run-as "${app}" rm -rf -- "${lib_basename}"
}

# Tells Android to use the given LAYERS found in LAYER_APP when launching APP.
# Precondition: the layer is installed (either during APK generation by inject_layer)
# Usage: enable_layer APP LAYERS LAYES_APP
# Example: enable_layer com.lunarg.gfxreconstruct.replay VK_LAYER_KHRONOS_validation com.lunarg.gfxreconstruct.replay
enable_layer() {
    local -r app="$1"
    local -r layers="$2"
    local -r layer_app="$3"
    # Adapted from https://developer.android.com/ndk/guides/graphics/validation-layer
    adb shell settings put global enable_gpu_debug_layers 1
    adb shell settings put global gpu_debug_app "${app}"
    adb shell settings put global gpu_debug_layers "${layers}"
    adb shell settings put global gpu_debug_layer_app "${layer_app}"
}

# Returns 0 if the app is installed and matches the APK exactly.
# Usage: is_apk_installed APP APK
# Example: if ! is_apk_installed com.lunarg.gfxreconstruct.replay ./install/gfxr-replay.apk; then python gfxrecon.py install-apk ./install/gfxr-replay.apk; fi
# TODO: Use android tools to extract app name from APK to remove a param
is_apk_installed() {
    local -r app="$1"
    local -r apk="$2"

    # First, is it installed
    if is_app_installed "${app}"
    then
        # Second, do the files match.
        local -r remote_apk_filepath="$(get_app_path "${app}")"
        local -r remote_apk_sha=$(adb shell sha256sum -b "${remote_apk_filepath}")
        local -r local_apk_sha=$(sha256sum "${apk}" | awk '{ print $1 }')
        if [ "${remote_apk_sha}" == "${local_apk_sha}" ]
        then
            return 0
        fi
    fi

    # Not installed or files do not match
    return 1
}
