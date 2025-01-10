:: Copyright 2023 Google LLC
::
:: Licensed under the Apache License, Version 2.0 (the "License");
:: you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at
::
::      https://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: See the License for the specific language governing permissions and
:: limitations under the License.

@echo off

set PROJECT_ROOT=%~dp0\..
set BUILD_TYPE=Debug Release
set SRC_DIR=%PROJECT_ROOT%
set GFXR_ROOT_DIR=%PROJECT_ROOT%\\third_party\\gfxreconstruct\\android
set startTime=%time%

(for %%b in (%BUILD_TYPE%) do (
    setlocal enabledelayedexpansion
    echo.
    echo %%b : Building dive android layer
    set build=%%b
    set BUILD_DIR=%PROJECT_ROOT%\\build_android\\!build!
    echo BUILD_DIR: !BUILD_DIR!
    md !BUILD_DIR!

    pushd !BUILD_DIR!
    cmake  -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake ^
        -G "Ninja"^
        -DCMAKE_MAKE_PROGRAM="ninja" ^
        -DCMAKE_BUILD_TYPE=!build!  ^
        -DCMAKE_SYSTEM_NAME=Android ^
        -DANDROID_ABI=arm64-v8a ^
        -DANDROID_PLATFORM=android-26 ^
        -DgRPC_BUILD_CODEGEN=OFF ^
        -Dprotobuf_BUILD_PROTOC_BINARIES=OFF ^
        -DCARES_BUILD_TOOLS=OFF ^
        -DCARES_INSTALL=OFF ^
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER ^
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER ^
        %SRC_DIR%

    cmake --build . --config=!build! -j
    if "%%b" == "Release" cmake --install .
    popd
))

pushd !GFXR_ROOT_DIR!
(for %%b in (%BUILD_TYPE%) do (
    setlocal enabledelayedexpansion
    echo.
    echo %%b : Building gfxr android layer
    set build=%%b
    
    echo GFXR_ROOT_DIR: !GFXR_ROOT_DIR!
    call gradlew assemble!build!
))
popd


(for %%b in (%BUILD_TYPE%) do (
    setlocal enabledelayedexpansion
    if "%%b" == "Release" set build_lowercase=release
    if "%%b" == "Debug" set build_lowercase=debug
    echo.
    echo %%b : Moving gfxr files
    set build=%%b
    set BUILD_DIR=%PROJECT_ROOT%\\build_android\\!build!

    set GFXR_BUILD_DIR=!BUILD_DIR!\\third_party\\gfxreconstruct\\android
    if not exist !GFXR_BUILD_DIR! md !GFXR_BUILD_DIR!

    echo Extracting gfxr android layer into build_android
    set GFXR_LAYER_SRC=!GFXR_ROOT_DIR!\\layer\\build\\outputs\\aar\\layer-!build_lowercase!.aar
    set GFXR_LAYER_DST=!GFXR_BUILD_DIR!\\layer
    if not exist !GFXR_LAYER_DST! md !GFXR_LAYER_DST!
    tar -xf !GFXR_LAYER_SRC! -C !GFXR_LAYER_DST!

    echo Copying gfxr android replay into build_android
    set GFXR_REPLAY_SRC=!GFXR_ROOT_DIR!\\tools\\replay\\build\\outputs\\apk\\!build_lowercase!
    set GFXR_REPLAY_DST=!GFXR_BUILD_DIR!\\tools\\replay
    if not exist !GFXR_REPLAY_DST! md !GFXR_REPLAY_DST!
    copy !GFXR_REPLAY_SRC! !GFXR_REPLAY_DST!
))

echo Start Time: %startTime%
echo Finish Time: %time%