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

:: This script automates the standard build process. For comprehensive documentation and advanced options, please refer to BUILD.md, "Dive Device Libraries" section

@echo off
setlocal enabledelayedexpansion
set PROJECT_ROOT=%~dp0\..
set BUILD_TYPE=Debug
set startTime=%time%

if "%~1"=="" goto parsingdone
if "%1"=="Debug" set BUILD_TYPE=%1
if "%1"=="Release" set BUILD_TYPE=%1
if not !BUILD_TYPE!==%1 (
    echo Invalid parameter passed for BUILD_TYPE: %1
    echo Valid options: 'Debug', 'Release'
    exit /b 1
)
:parsingdone
echo Building all the following types: !BUILD_TYPE!

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

if not !ERRORLEVEL!==0 exit /b 1

(for %%b in (!BUILD_TYPE!) do (
    setlocal enabledelayedexpansion
    echo.
    set build=%%b

    cmake --build build/device --config=!build!
    if not !ERRORLEVEL!==0 exit /b 1

    cmake --install build/device --prefix build/pkg/device --config=!build! 
    if not !ERRORLEVEL!==0 exit /b 1
))

popd

echo Start Time: %startTime%
echo Finish Time: %time%