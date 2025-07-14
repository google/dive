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

print_usage() {
    set +x
    echo "End-to-end capture-replay test for package"
    echo
    echo "Usage: gfxr_capture_replay_test.sh [-hcdrs] -p PACKAGE"
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
    echo
    echo "Example:"
    echo " gfxr_capture_replay_test.sh -p com.google.bigwheels.project_cube_xr.debug"
    set -x
}

CAPTURE_DEBUG=0
REPLAY_DEBUG=0
REPLAY_VALIDATION=0
CAPTURE_VALIDATION=0
while getopts cdhp:rs getopts_flag
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
    esac
done

THIS_DIR=$(dirname "$0")
. "${THIS_DIR}/test_automation/common.sh"

ACTIVITY="$(find_default_activity "${PACKAGE}")"

# Fairly reliable directory on remote device, as long as app has MANAGE_EXTERNAL_STORAGE permissions.
# /data/local/tmp doesn't work on all devices tested.
REMOTE_TEMP_DIR=/sdcard/Download
GFXR_CAPTURE_DIR_BASENAME=gfxr_capture
REPLAY_PACKAGE=com.lunarg.gfxreconstruct.replay
GFXRECON=./third_party/gfxreconstruct/android/scripts/gfxrecon.py
BUILD_DIR=./build
GFXR_DUMP_RESOURCES="${BUILD_DIR}/gfxr_dump_resources/gfxr_dump_resources"
JSON_BASENAME=dump.json
DUMP_DIR="${REMOTE_TEMP_DIR}/dump"
# Needs to be high enough to skip over loading screens but not too high to trip the wait_for_logcat_line timeout.
CAPTURE_FRAME=720
GFXR_BASENAME="${PACKAGE}_frame_${CAPTURE_FRAME}.gfxr"
GFXA_BASENAME="${PACKAGE}_asset_file.gfxa"
GFXR_REPLAY_APK=./install/gfxr-replay.apk
RESULTS_DIR="${PACKAGE}-$(date +%Y%m%d_%H%M%S)"
JSON="${RESULTS_DIR}/${JSON_BASENAME}"
LOCAL_TEMP_DIR=/tmp
ARCHIVE_BASENAME=android-binaries-1.4.313.0
ARCHIVE_FILE=android-binaries-1.4.313.0.tar.gz
VALIDATION_LAYER_DIR=${LOCAL_TEMP_DIR}/${ARCHIVE_BASENAME}
VALIDATION_LAYER_LIB=libVkLayer_khronos_validation.so
REMOTE_TEMP_FILEPATH="/data/local/tmp/${VALIDATION_LAYER_LIB}"
ARCH=$(adb shell getprop ro.product.cpu.abi)
LOCAL_VALIDATION_LAYER_FILEPATH="${VALIDATION_LAYER_DIR}/${ARCH}/${VALIDATION_LAYER_LIB}"
FRAME_DELIMITER_PROP=openxr.enable_frame_delimiter

#
# Clear anything previously set (in case the script exited prematurely)
#

unset_gfxr_props
unset_vulkan_debug_settings

#
# Ask OpenXR runtime to emit frame debug marker
#

# Only try if it's not set (to avoid having to root all the time)
ENABLE_FRAME_DELIMITER="$(adb shell getprop ${FRAME_DELIMITER_PROP})"
if [ -z "${ENABLE_FRAME_DELIMITER}" ]
then
    adb root
    adb shell setprop ${FRAME_DELIMITER_PROP} true
    adb unroot
fi

#
# Download validation layers
#

if [ ${REPLAY_VALIDATION} -eq 1 -o ${CAPTURE_VALIDATION} -eq 1 ]
then
    # Download the archive and cache the result
    if [ ! -f "${LOCAL_TEMP_DIR}/${ARCHIVE_FILE}" ]; then
        $(cd "${LOCAL_TEMP_DIR}" && wget https://github.com/KhronosGroup/Vulkan-ValidationLayers/releases/download/vulkan-sdk-1.4.313.0/android-binaries-1.4.313.0.tar.gz)
    fi
    # Extract the archive and cache the result
    if [ ! -e "${LOCAL_VALIDATION_LAYER_FILEPATH}" ]; then
        $(cd "${LOCAL_TEMP_DIR}" && tar xf "${ARCHIVE_FILE}")
    fi
fi

mkdir -p "${RESULTS_DIR}"

#
# 1. Install replay package for both capture layer and replay activity
#

# Check if we need to reinstall the replay APK. First, is it installed.
# TODO would be nice if we could isolate this into a function
install_replay_apk=0
if is_app_installed "${REPLAY_PACKAGE}"
then
    REMOTE_REPLAY_APK_FILEPATH="$(get_app_path "${REPLAY_PACKAGE}")"
    # Second, do the files match.
    REMOTE_APK_SHA=$(adb shell sha256sum -b "${REMOTE_REPLAY_APK_FILEPATH}")
    LOCAL_APK_SHA=$(sha256sum "${GFXR_REPLAY_APK}" | awk '{ print $1 }')
    if [ "${REMOTE_APK_SHA}" != "${LOCAL_APK_SHA}" ]
    then
        adb uninstall "${REPLAY_PACKAGE}"
        install_replay_apk=1
    fi
else
    install_replay_apk=1
fi

if [ $install_replay_apk -eq 1 ]
then
    python "${GFXRECON}" install-apk "${GFXR_REPLAY_APK}"
    # Replay with --dump-resources needs permissions to store generated BMPs
    adb shell appops set "${REPLAY_PACKAGE}" MANAGE_EXTERNAL_STORAGE allow
fi

# Install the validation layer into the replay app so we can easily find it in both capture and replay
if [ ${REPLAY_VALIDATION} -eq 1 -o ${CAPTURE_VALIDATION} -eq 1 ]
then
    # run-as is probably fine since we control the replay app is built
    adb push "${LOCAL_VALIDATION_LAYER_FILEPATH}" "${REMOTE_TEMP_FILEPATH}"
    # Can't mv since REMOTE_TEMP_FILEPATH is owned by shell or root
    adb shell run-as "${REPLAY_PACKAGE}" cp "${REMOTE_TEMP_FILEPATH}" .
    adb shell rm -rf "${REMOTE_TEMP_FILEPATH}"
fi

# TODO copy replay APK into RESULTS_DIR?

#
# 2. Configure PACKAGE to use capture layer from replay package
#

CAPTURE_LAYERS="VK_LAYER_LUNARG_gfxreconstruct"
CAPTURE_DEBUG_LAYER_APPS="${REPLAY_PACKAGE}"
if [ ${CAPTURE_VALIDATION} -eq 1 ]
then
    # Put validation layer last otherwise we try to capture bogus objects. Also replay fails otherwise.
    CAPTURE_LAYERS="${CAPTURE_LAYERS}:VK_LAYER_KHRONOS_validation"
    CAPTURE_DEBUG_LAYER_APPS="${PACKAGE}:${CAPTURE_DEBUG_LAYER_APPS}"

    # TODO need to use run-as until the validation layers are packed into the replay APK
    adb push "${LOCAL_VALIDATION_LAYER_FILEPATH}" "${REMOTE_TEMP_FILEPATH}"
    # Can't mv since REMOTE_TEMP_FILEPATH is owned by shell or root
    adb shell run-as "${PACKAGE}" cp "${REMOTE_TEMP_FILEPATH}" .
    adb shell rm -rf "${REMOTE_TEMP_FILEPATH}"
fi

# Adapted from https://developer.android.com/ndk/guides/graphics/validation-layer.
adb shell settings put global enable_gpu_debug_layers 1
adb shell settings put global gpu_debug_app "${PACKAGE}"
adb shell settings put global gpu_debug_layers "${CAPTURE_LAYERS}"
# Both the capture and validation layers are in the replay APK since it's an easy place to put them.
adb shell settings put global gpu_debug_layer_app "${CAPTURE_DEBUG_LAYER_APPS}"

#
# 3. Configure GFXR behavior
#

# See //third_party/gfxreconstruct/USAGE_android.md for more options.
adb shell mkdir -p "${REMOTE_TEMP_DIR}"
adb shell setprop debug.gfxrecon.capture_file "${REMOTE_TEMP_DIR}/${PACKAGE}.gfxr"
# NOTE: quit_after_capture_frames doesn't work with capture_android_trigger or capture_frames :(
# Prefer capture_frames over trigger since it's more friendly to scripting. It knows better when the app is ready and producing frames compared to trigger capture (where we have to guess).
# adb shell setprop debug.gfxrecon.capture_trigger_frames 1
# adb shell setprop debug.gfxrecon.capture_android_trigger false
adb shell setprop debug.gfxrecon.capture_frames "${CAPTURE_FRAME}"
adb shell setprop debug.gfxrecon.capture_use_asset_file false # XXX asset file is known to have problems
# Remove timestamp from capture filename so it's more predictable
adb shell setprop debug.gfxrecon.capture_file_timestamp false
# Try to write all packets, even when we crash
adb shell setprop debug.gfxrecon.capture_file_flush true
# More logging
adb shell setprop debug.gfxrecon.log_level debug
# Capture layer in PACKAGE needs permissions to read capture/asset file from storage.
adb shell appops set "${PACKAGE}" MANAGE_EXTERNAL_STORAGE allow

#
# 4. Capture
#

# Clear logcat so that we can use it to determine when capture is done
adb logcat -c

if [ ${CAPTURE_DEBUG} -eq 1 ]
then
    adb shell am set-debug-app -w "${PACKAGE}"
fi

adb shell am start -S -W -n "${PACKAGE}/${ACTIVITY}"

# Given how long it takes to attach the debugger, etc, it is unlikely that you'll want the script to proceed.
if [ ${CAPTURE_DEBUG} -eq 1 ]
then
    exit 0
fi

# Wait for capture to finish. Luckily, the app logs when it's done so use that as the signal to proceed.
# This only works since we clear the logcat at the start of the test.
if ! wait_for_logcat_line gfxrecon "Finished recording graphics API capture"
then
    adb bugreport
fi
adb shell am force-stop "${PACKAGE}"

# Pull the GFXR/GFXA for gfxr_dump_resources
adb pull "${REMOTE_TEMP_DIR}/${GFXR_BASENAME}" "${RESULTS_DIR}"
# TODO asset file was disabled for testing; re-enable
# adb pull "${REMOTE_TEMP_DIR}/${GFXA_BASENAME}" "${RESULTS_DIR}"

#
# 5. Post-capture clean-up
#

# NOTE: Don't clean up GFXA/GFXR since we can use it for replay. Saves having to push again.

# Next launch should not use GFXR
unset_gfxr_props
unset_vulkan_debug_settings

#
# 6. Replay with dump-resources
#

"${GFXR_DUMP_RESOURCES}" --last_draw_only "${RESULTS_DIR}/${GFXR_BASENAME}" "${JSON}"
adb shell mkdir -p "${DUMP_DIR}"
adb push "${JSON}" "${REMOTE_TEMP_DIR}"

if [ ${REPLAY_VALIDATION} -eq 1 ]
then
    adb shell settings put global enable_gpu_debug_layers 1
    adb shell settings put global gpu_debug_app "${REPLAY_PACKAGE}"
    adb shell settings put global gpu_debug_layers VK_LAYER_KHRONOS_validation
    adb shell settings put global gpu_debug_layer_app "${REPLAY_PACKAGE}"
fi

if [ ${REPLAY_DEBUG} -eq 1 ]
then
    adb shell am set-debug-app -w "${REPLAY_PACKAGE}"
fi

python "${GFXRECON}" replay \
    --dump-resources "${REMOTE_TEMP_DIR}/${JSON_BASENAME}" \
    --dump-resources-dir "${DUMP_DIR}" \
    --dump-resources-dump-all-image-subresources \
    --log-level debug \
    "${REMOTE_TEMP_DIR}/${GFXR_BASENAME}"

# `gfxrecon.py replay` does not wait for the app to start so. However, if it starts logging then we can assume that it has started.
# This only works since we clear the logcat at the start of the test.
if ! wait_for_logcat_line gfxrecon "Initializing GFXReconstruct capture layer"
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

adb shell rm -rf "${DUMP_DIR}"
adb shell rm -rf "${REMOTE_TEMP_DIR}/${GFXR_BASENAME}"
adb shell rm -rf "${REMOTE_TEMP_DIR}/${GFXA_BASENAME}"
adb shell rm -rf "${REMOTE_TEMP_DIR}/${JSON_BASENAME}"

# Next launch should not use GFXR


#
# 8. Collect results
#

# Show logcat for diagnostic purposes. Include DEBUG in case there was a crash.
adb logcat -d -s gfxrecon,DEBUG

# Convert BMP captures into JPG for convenience.
find "${RESULTS_DIR}" -name "*.bmp" | xargs -P0 -I {} convert {} {}.jpg

tar czf "${RESULTS_DIR}.tgz" "${RESULTS_DIR}"
echo "Results written to: ${RESULTS_DIR}"
