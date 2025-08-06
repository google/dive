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

# The next app launch should use the default GFXR settings
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

# The next app launch should not use debug layers
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

# Get the package name from the APK.
# NOTE: build-tools must be in your PATH
# Usage: get_apk_package_name MY_APP.APK
# Example: PACKAGE=$(get_apk_package_name ./install/gfxr-replay.apk) # PACKAGE should be com.lunarg.gfxreconstruct.replay
# get_apk_package_name() {
#     # TODO if we don't want build-tools dep, we could just ask the user for the name
#     aapt2 dump badging "$1" | grep "package:" | grep -oE " name='[^']*'"| sed -E "s/ name='([^']*)'/\1/"
# }

# If the local replay APK differs from the device, install it
# Usage: install_if_different APK
# Example: install_if_different ./install/gfxr-replay.apk
# install_replay_if_different() {
#     apk_file="$1"
#     # This is replay-specific so hard-code
#     package="com.lunarg.gfxreconstruct.replay"
#     if is_app_installed "${package}"
#     then

#     fi
# }