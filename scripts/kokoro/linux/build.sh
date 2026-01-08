#!/bin/bash

# TODO: remove -x after...
set -ex

sudo apt-get install --yes \
  cmake ninja-build clang-18 python3-mako \
  qtbase5-dev libarchive-dev liblz4-dev libtinfo-dev \
  openjdk-17-jdk

export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64

# Github build only:
git config --global --add safe.directory "${KOKORO_ARTIFACTS_DIR}/github/dive"
git config --global --add safe.directory "${KOKORO_ARTIFACTS_DIR}/github/dive/third_party/OpenXR-SDK"
git config --global --add safe.directory "${KOKORO_ARTIFACTS_DIR}/github/dive/third_party/Vulkan-Headers"
git config --global --add safe.directory "${KOKORO_ARTIFACTS_DIR}/github/dive/third_party/abseil-cpp"
git config --global --add safe.directory "${KOKORO_ARTIFACTS_DIR}/github/dive/third_party/crashpad"
git config --global --add safe.directory "${KOKORO_ARTIFACTS_DIR}/github/dive/third_party/gfxreconstruct/external/OpenXR-SDK"
git config --global --add safe.directory "${KOKORO_ARTIFACTS_DIR}/github/dive/third_party/gfxreconstruct/external/SPIRV-Headers"
git config --global --add safe.directory "${KOKORO_ARTIFACTS_DIR}/github/dive/third_party/gfxreconstruct/external/SPIRV-Reflect"
git config --global --add safe.directory "${KOKORO_ARTIFACTS_DIR}/github/dive/third_party/gfxreconstruct/external/Vulkan-Headers"
git config --global --add safe.directory "${KOKORO_ARTIFACTS_DIR}/github/dive/third_party/googletest"
git config --global --add safe.directory "${KOKORO_ARTIFACTS_DIR}/github/dive/third_party/libarchive"
git submodule update --init

pyenv install 3.14
pyenv global 3.14

pip install mako

cmake -G "Ninja" -Bbuild \
    -DCMAKE_BUILD_TYPE=Release \
    -DDIVE_BUILD_WITH_CRASHPAD=OFF \
    -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/clang-18 \
    -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++-18 \
    .
cmake --build build --config Release
