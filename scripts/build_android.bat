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
set startTime=%time%

(for %%b in (%BUILD_TYPE%) do (
    setlocal enabledelayedexpansion
    echo %%b
    set build=%%b
    set BUILD_DIR=%PROJECT_ROOT%\\build_android\\!build!
    echo "BUILD_DIR: " !BUILD_DIR!
    md !BUILD_DIR!
    pushd !BUILD_DIR!
    cmake  -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake ^
        -G "Ninja"^
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

    cmake --build . --config=!build! -j %NUMBER_OF_PROCESSORS%
    popd
))

echo Start Time: %startTime%
echo Finish Time: %time%