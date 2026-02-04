#!/bin/bash
# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# --- Input Package Name ---
packageName=""
while [ -z "$packageName" ]; do
    read -p "Enter the package name: " packageName
    if [ -z "$packageName" ]; then
        echo "Package name cannot be empty. Please enter a valid package name."
    fi
done

# --- Constants ---
kVkLayerLibName="libVkLayer_rt_dive.so"
kTargetPath="/data/local/tmp"
kVkLayerName="VK_LAYER_Dive"

# --- Helper function for error handling ---
check_error() {
    if [ $1 -ne 0 ]; then
        echo ""
        echo "--- Error occurred during ADB execution ---"
        echo "--- Please check the output above for details ---"
        read -p "Press enter to exit..."
        exit 1
    fi
}

# --- ADB Commands ---

echo ""
echo "--- Running ADB commands ---"

echo ""
echo "1. adb root"
adb root
check_error $?

echo ""
echo "2. adb wait-for-device"
adb wait-for-device
check_error $?

echo ""
echo "3. adb shell setenforce 0"
adb shell setenforce 0
check_error $?

echo ""
echo "4. adb push ./build/pkg/device/$kVkLayerLibName $kTargetPath"
adb push ./build/pkg/device/$kVkLayerLibName $kTargetPath
check_error $?

echo ""
echo "5. adb shell \"run-as $packageName cp $kTargetPath/$kVkLayerLibName .\""
adb shell "run-as $packageName cp $kTargetPath/$kVkLayerLibName ."
check_error $?

echo ""
echo "6. adb shell settings put global enable_gpu_debug_layers 1"
adb shell settings put global enable_gpu_debug_layers 1
check_error $?

echo ""
echo "7. adb shell settings put global gpu_debug_app $packageName"
adb shell settings put global gpu_debug_app $packageName
check_error $?

echo ""
echo "8. adb shell settings put global gpu_debug_layer_app $packageName"
adb shell settings put global gpu_debug_layer_app $packageName
check_error $?

echo ""
echo "9. adb shell settings put global gpu_debug_layers $kVkLayerName"
adb shell settings put global gpu_debug_layers $kVkLayerName
check_error $?

echo ""
echo ""
echo "--- ADB commands completed successfully ---"
read -p "Press enter to exit..."
