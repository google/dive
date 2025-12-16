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

set -ex

readonly DIVE_ROOT="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )/.." &> /dev/null && pwd )"

mkdir -p build/package_linux/device
mkdir -p build/package_linux/host

if [ ! -e "build/package_linux/dive_android_is_prebuilt" ]; then
    pushd build/package_linux/device
        cmake -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake \
            -G "Ninja" \
            -DCMAKE_MAKE_PROGRAM="ninja" \
            -DCMAKE_BUILD_TYPE=Debug \
            -DCMAKE_SYSTEM_NAME=Android \
            -DANDROID_ABI=arm64-v8a \
            -DANDROID_PLATFORM=android-26 \
            -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER \
            -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER \
            -DDIVE_GFXR_GRADLE_CONSOLE=plain \
            ${DIVE_ROOT} || exit 1
        cmake --build . --config=Debug -j || exit 1
        cmake --install . --prefix ../dive_android || exit 1
    popd
fi

pushd build/package_linux/host
    cmake -G "Ninja" \
        -DCMAKE_BUILD_TYPE=Release \
        -DDIVE_ANDROID_PREBUILT="`pwd`/../dive_android" \
        -DDIVE_INSTALL_EXTRAS="${DIVE_ROOT}/packaging/extras-example" \
        -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/clang-19 \
        -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++-19 \
        ${DIVE_ROOT} || exit 1
    cmake --build . --config Release || exit 1
    cmake --install . --prefix ../dive_linux --config Release
    cpack -G DEB
popd

pushd build/package_linux
  cp host/*.deb .
  dpkg -I *.deb
  dpkg -c *.deb
popd
