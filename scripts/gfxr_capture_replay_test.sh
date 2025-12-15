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

# TODO should script abort trigger cleanup? what about bugreport capture?

set -eux

# Include common.h for constants and functions
THIS_DIR=$(dirname "$0")
. "${THIS_DIR}/test_automation/common.sh"

cleanup() {
    echo "Starting cleanup"
    adb shell setprop debug.openxr.enable_frame_delimiter '""'
    unset_gfxr_props
    unset_vulkan_debug_settings
    # DUMP_DIR is from common.h, so should be defined by the time this is called.
    adb shell rm -rf "${DUMP_DIR}"
    adb shell am force-stop "${REPLAY_PACKAGE}"
    # These vars are set by this script so need to check if defined.
    # TODO does this interfere with set -u? set -e?
    [ -n "${PACKAGE+x}" ] && adb shell am clear-debug-app -w "${PACKAGE}"
    [ -n "${PACKAGE+x}" ] && uninstall_layer "${PACKAGE}" "${VALIDATION_LAYER_BASENAME}"
    [ -n "${PACKAGE+x}" ] && adb shell am force-stop "${PACKAGE}"
    [ -n "${JSON_BASENAME+x}" ] && adb shell rm -rf "${REMOTE_TEMP_DIR}/${JSON_BASENAME}"
    # TODO use timestamps like Dive
    [ -n "${GFXR_BASENAME+x}" ] && adb shell rm -rf "${REMOTE_TEMP_DIR}/${GFXR_BASENAME}"
    [ -n "${GFXA_BASENAME+x}" ] && adb shell rm -rf "${REMOTE_TEMP_DIR}/${GFXA_BASENAME}"
    # Turn the screen off to save on battery and prevent burn in
    adb shell input keyevent KEYCODE_SLEEP
    adb unroot
    # Cleanup should never cause the script to halt. Ensure this is the last command.
    return 0
}

trap_cleanup() {
    echo "Running cleanup from EXIT trap"
    cleanup
}

print_usage() {
    set +x
    echo "End-to-end capture-replay test for package"
    echo
    echo "Usage: gfxr_capture_replay_test.sh [-hcdrsz] -p PACKAGE"
    echo
    echo "Required:"
    echo " -p PACKAGE: The app package to test"
    echo
    echo "Optional:"
    echo " -h: Print help"
    echo " -c: Wait for debugger before capturing"
    echo " -d: Use validation layers during capture"
    echo " -r: Wait for debugger before replay"
    echo " -s: Use validation layers during replay"
    echo " -z: Package is a system app"
    echo
    echo "Example:"
    echo " gfxr_capture_replay_test.sh -p com.google.bigwheels.project_cube_xr.debug"
    set -x
}

#
# Process cmdline args
#

CAPTURE_DEBUG=0
REPLAY_DEBUG=0
REPLAY_VALIDATION=0
CAPTURE_VALIDATION=0
SYSTEM_PACKAGE=0
while getopts cdhp:rsz getopts_flag
do
    case "${getopts_flag}" in
        c) CAPTURE_DEBUG=1;;
        d) CAPTURE_VALIDATION=1;;
        h)
            print_usage
            exit 0
            ;;
        p) PACKAGE="${OPTARG}";;
        r) REPLAY_DEBUG=1;;
        s) REPLAY_VALIDATION=1;;
        z) SYSTEM_PACKAGE=1;;
    esac
done

#
# Additional setup
#

# Can't launch an app just with the package name, Try to find the default main activity
ACTIVITY="$(find_default_activity "${PACKAGE}")"
if [ -z "$ACTIVITY" -a ${SYSTEM_PACKAGE} -eq 0 ]
then
    echo "No default activity. Is this a system package? Try -z"
    exit 1
fi

# Clear anything previously set (in case the script exited prematurely)
cleanup

# Make directory to store results
RESULTS_DIR="${PACKAGE}-$(date +%Y%m%d_%H%M%S)"
mkdir -p "${RESULTS_DIR}"

# Cleanup on EXIT. Register right before device state modification
trap trap_cleanup EXIT

#
# 1. Install replay package for both capture layer and replay activity
#

# Save on time by only reinstalling replay if absolutely necessary. This does a SHA match so works even if an app by the same name is installed.
if ! is_apk_installed "${REPLAY_PACKAGE}" "${GFXR_REPLAY_APK}"
then
    # Try incremental install. IF that fails (e.g. different cert) then explicitly uninstall and try again.
    if ! python "${GFXRECON}" install-apk "${GFXR_REPLAY_APK}"
    then
        adb uninstall "${REPLAY_PACKAGE}"
        # If this fails then halt because something is wrong.
        python "${GFXRECON}" install-apk "${GFXR_REPLAY_APK}"
    fi
fi

# TODO copy replay APK into RESULTS_DIR?

#
# 2. Configure PACKAGE to use capture layer from replay package
#

CAPTURE_LAYERS="VK_LAYER_LUNARG_gfxreconstruct"
CAPTURE_DEBUG_LAYER_APPS="${REPLAY_PACKAGE}"
if [ ${CAPTURE_VALIDATION} -eq 1 ]
then
    # Put validation layer first in dispatch otherwise we try to capture bogus objects. Also replay fails otherwise.
    CAPTURE_LAYERS="${CAPTURE_LAYERS}:VK_LAYER_KHRONOS_validation"
    CAPTURE_DEBUG_LAYER_APPS="${PACKAGE}:${CAPTURE_DEBUG_LAYER_APPS}"
    inject_layer "${PACKAGE}" "${LOCAL_VALIDATION_LAYER_FILEPATH}"
fi

enable_layer "${PACKAGE}" "${CAPTURE_LAYERS}" "${CAPTURE_DEBUG_LAYER_APPS}"

#
# 3. Configure GFXR behavior
#

# See //third_party/gfxreconstruct/USAGE_android.md for more options.
adb shell mkdir -p "${REMOTE_TEMP_DIR}"
adb shell setprop debug.gfxrecon.capture_file "${REMOTE_TEMP_DIR}/${PACKAGE}.gfxr"
# Use trigger trim with asset file since it's what Dive prefers
adb shell setprop debug.gfxrecon.capture_trigger_frames 1
adb shell setprop debug.gfxrecon.capture_android_trigger false
adb shell setprop debug.gfxrecon.capture_use_asset_file true
# Remove timestamp from capture filename so it's more predictable.
# Since we copy into a timestamped results folder, we don't end up overwriting pulled results.
adb shell setprop debug.gfxrecon.capture_file_timestamp false
# Since we focused on "does this work?" the extra logging helps
adb shell setprop debug.gfxrecon.log_level debug
# Capture layer in PACKAGE needs permissions to read capture/asset file from storage.
adb shell appops set "${PACKAGE}" MANAGE_EXTERNAL_STORAGE allow

#
# 4. Capture
#

# Ask OpenXR runtime to emit frame debug marker
adb shell setprop debug.openxr.enable_frame_delimiter true

# Clear logcat so that we can use logcat to determine when capture is done.
adb logcat -c

if [ ${CAPTURE_DEBUG} -eq 1 ]
then
    adb shell am set-debug-app -w "${PACKAGE}"
fi

# Wake up the screen, otherwise capture will crash and replay won't replay.
# TODO does this work for system apps?
adb shell input keyevent KEYCODE_WAKEUP

# Start app, wait for it to start
if [ ${SYSTEM_PACKAGE} -eq 1 ]
then
    adb root
    adb shell stop
    adb shell start
else
    adb shell am start -S -W -n "${PACKAGE}/${ACTIVITY}"
fi

# TODO assert that screen is on

# Given how long it takes to attach the debugger, etc, it is unlikely that you'll want the script to proceed.
if [ ${CAPTURE_DEBUG} -eq 1 ]
then
    exit 0
fi

# Likely redundant, but wait for the capture layer to log that it's been loaded
if ! wait_for_logcat_line gfxrecon "Initializing GFXReconstruct capture layer"
then
    adb bugreport
fi

# Trigger a capture after the app has enough time to load.
# Use this over the capture_frame setting since Dive doesn't use capture_frame.
# Unfortunately "the app is loaded" is not something we can determine so we need to sleep... This is where capture_frame could really help.
# 20s is too short for some large Unity apps.
# TODO 30s is a long time to wait for, e.g. cube_xr. Consider adding a debug print on first frame rendered
sleep 30
adb shell setprop debug.gfxrecon.capture_android_trigger true

# Wait for capture to finish. Luckily, the app logs when it's done so use that as the signal to proceed.
# This only works since we clear the logcat at the start of the test.
# We prefer this over quit_after_capture_frames since that setting seems broken.
if ! wait_for_logcat_line gfxrecon "Finished recording graphics API capture"
then
    adb bugreport
fi
adb shell am force-stop "${PACKAGE}"

# Pull the GFXR/GFXA for gfxr_dump_resources
readonly GFXR_BASENAME="${PACKAGE}_trim_trigger.gfxr"
readonly GFXA_BASENAME="${PACKAGE}_asset_file.gfxa"
adb pull "${REMOTE_TEMP_DIR}/${GFXR_BASENAME}" "${RESULTS_DIR}"
adb pull "${REMOTE_TEMP_DIR}/${GFXA_BASENAME}" "${RESULTS_DIR}"

#
# 5. Post-capture clean-up
#

# NOTE: Don't clean up GFXA/GFXR since we can use it for replay. Saves having to push again.

# Next launch of PACKAGE/ACTIVITY should not use GFXR
unset_gfxr_props
unset_vulkan_debug_settings
uninstall_layer "${PACKAGE}" ""

#
# 6. Replay with dump-resources
#

# --last-draw-only saves time by only dumping the final draw call. This should represent what the user sees.
# Name of the JSON file produced by gfxr_dump_resources
readonly JSON_BASENAME=dump.json
readonly JSON="${RESULTS_DIR}/${JSON_BASENAME}"
"${GFXR_DUMP_RESOURCES}" --last_draw_only "${RESULTS_DIR}/${GFXR_BASENAME}" "${JSON}"
adb shell mkdir -p "${DUMP_DIR}"
adb push "${JSON}" "${REMOTE_TEMP_DIR}"

if [ ${REPLAY_VALIDATION} -eq 1 ]
then
    inject_layer "${REPLAY_PACKAGE}" "${LOCAL_VALIDATION_LAYER_FILEPATH}"
    enable_layer "${REPLAY_PACKAGE}" "${VALIDATION_LAYER_NAME}" "${REPLAY_PACKAGE}"
fi

# Replay needs permissions to read the capture. With --dump-resources it needs to store generated BMPs.
# Always do this since this permission resets on reboot or stop/start if not explicitly granted.
adb shell appops set "${REPLAY_PACKAGE}" MANAGE_EXTERNAL_STORAGE allow

if [ ${REPLAY_DEBUG} -eq 1 ]
then
    adb shell am set-debug-app -w "${REPLAY_PACKAGE}"
fi

# TODO --dump-resources-dump-all-image-subresources has moved into the json file
python "${GFXRECON}" replay \
    --dump-resources "${REMOTE_TEMP_DIR}/${JSON_BASENAME}" \
    --dump-resources-dir "${DUMP_DIR}" \
    --log-level debug \
    "${REMOTE_TEMP_DIR}/${GFXR_BASENAME}"

# `gfxrecon.py replay` does not wait for the app to start. However, if it starts logging then we can assume that it has started.
# This only works since we clear the logcat at the start of the test.
if ! wait_for_logcat_line gfxrecon "Loading state for captured frame"
then
    adb bugreport
fi
# We can infer that replay is finished when the replay app process is gone.
while adb shell pidof "${REPLAY_PACKAGE}"
do
    sleep 1
done
if is_crash_detected
then
    adb bugreport
fi

# Pull the entire dump dir since it has both a meta JSON file along with the BMP of the final image.
adb pull "${DUMP_DIR}" "${RESULTS_DIR}"

#
# 7. Post-replay cleanup
#

cleanup

#
# 8. Collect results
#

# Show logcat to the user for diagnostic purposes. Include DEBUG in case there was a crash.
adb logcat -d -s gfxrecon,DEBUG

# Convert BMP captures into JPG for convenience.
find "${RESULTS_DIR}" -name "*.bmp" | xargs -P0 -I {} convert {} {}.jpg

# Compress files to make them easier to share.
tar czf "${RESULTS_DIR}.tgz" "${RESULTS_DIR}"

set +x
echo "------------------------------------------"
echo "Results written to: ${RESULTS_DIR}.tgz"
