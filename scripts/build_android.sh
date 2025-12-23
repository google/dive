#!/bin/bash

# Copyright 2020 Google LLC
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

# This script automates the standard build process. For comprehensive documentation and advanced options, please refer to BUILD.md, "Dive Device Resources" section

PROJECT_ROOT="$(readlink -f $0)"
PROJECT_ROOT="${PROJECT_ROOT%/*}/.."
readonly PROJECT_ROOT="$(readlink -f ${PROJECT_ROOT})"
BUILD_TYPE=(Debug)
readonly START_TIME=`date +%r`

if [ $# -ne 0 ]; then
    if [ "$1" = "Debug" ] || [ "$1" = "Release" ]; then
        BUILD_TYPE=$1
    else
        echo "Invalid parameter passed for BUILD_TYPE: $1"
        echo "Valid options: 'Debug', 'Release'"
        exit 1
    fi
fi
echo "Building all the following types: $BUILD_TYPE"

pushd ${PROJECT_ROOT}

echo "current dir " `pwd`

cmake . -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake \
    -G "Ninja Multi-Config" \
    -Bbuild_android \
    -DCMAKE_MAKE_PROGRAM="ninja" \
    -DCMAKE_SYSTEM_NAME=Android \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-26 \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER \
    || exit 1

for build in "${BUILD_TYPE}"
do
    cmake --build build_android --config=${build} || exit 1

    cmake --install build_android --prefix install --config=${build} || exit 1
done

popd

echo "Start Time:" ${START_TIME}
echo "Finish Time:" `date +%r`