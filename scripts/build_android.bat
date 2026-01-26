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

:: This script automates the standard build process for Dive Device Libraries

@echo off
setlocal enabledelayedexpansion
set PROJECT_ROOT=%~dp0\..
set DIVE_RELEASE_TYPE="dev"
set startTime=%time%

if "%~1"=="" (
    echo "Must specify BUILD_TYPE"
    echo "Valid usage: 'build_android.bat Debug|Release|RelWithDebInfo [dive_release_type_str]'"
    exit /b 1
)

if "%1"=="Debug" set BUILD_TYPE="%1"
if "%1"=="Release" set BUILD_TYPE="%1"
if "%1"=="RelWithDebInfo" set BUILD_TYPE="%1"
if not !BUILD_TYPE!=="%1" (
    echo "Invalid parameter passed for BUILD_TYPE: %1"
    echo "Valid usage: 'build_android.bat Debug|Release|RelWithDebInfo [dive_release_type_str]'"
    exit /b 1
)

if NOT "%~2"=="" set DIVE_RELEASE_TYPE="%2"

echo Building with BUILD_TYPE: !BUILD_TYPE! DIVE_RELEASE_TYPE: !DIVE_RELEASE_TYPE!

pushd %PROJECT_ROOT%

cmake . -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake ^
    -G "Ninja Multi-Config"^
    -Bbuild/device ^
    -DCMAKE_MAKE_PROGRAM="ninja" ^
    -DCMAKE_SYSTEM_NAME=Android ^
    -DANDROID_ABI=arm64-v8a ^
    -DANDROID_PLATFORM=android-26 ^
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER ^
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER ^
    -DDIVE_RELEASE_TYPE=!DIVE_RELEASE_TYPE!

if not !ERRORLEVEL!==0 exit /b 1

cmake --build build/device --config=!BUILD_TYPE!
    if not !ERRORLEVEL!==0 exit /b 1

cmake --install build/device --prefix build/pkg --config=!BUILD_TYPE! 
if not !ERRORLEVEL!==0 exit /b 1

popd

echo Start Time: %startTime%
echo Finish Time: %time%
