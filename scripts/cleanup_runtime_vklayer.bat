:: Copyright 2025 Google LLC
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
@echo off
setlocal enabledelayedexpansion

REM --- Input Package Name ---
:getPackageName
set /p "packageName=Enter the package name: "
if "%packageName%"=="" (
    echo Package name cannot be empty. Please enter a valid package name.
    goto getPackageName
)

REM --- Print Cleanup Message ---
echo.
echo Cleanup Vulkan application %packageName%

REM --- Constants ---
set kVkLayerLibName=libVkLayer_rt_dive.so
set kTargetPath=/data/local/tmp

REM --- ADB Commands ---

echo.
echo --- Running ADB commands ---

echo.
echo 1. adb root
adb root
if "%ERRORLEVEL%" neq "0" goto error

echo.
echo 2. adb wait-for-device
adb wait-for-device
if "%ERRORLEVEL%" neq "0" goto error

echo.
echo 3. adb shell setenforce 0
adb shell setenforce 0
if "%ERRORLEVEL%" neq "0" goto error

echo.
echo 4. adb shell "run-as %packageName% rm %kVkLayerLibName%"
adb shell "run-as %packageName% rm %kVkLayerLibName%"
if "%ERRORLEVEL%" neq "0" goto error

echo.
echo 5. adb shell settings delete global enable_gpu_debug_layers
adb shell settings delete global enable_gpu_debug_layers
if "%ERRORLEVEL%" neq "0" goto error

echo.
echo 6. adb shell settings delete global gpu_debug_app
adb shell settings delete global gpu_debug_app
if "%ERRORLEVEL%" neq "0" goto error

echo.
echo 7. adb shell settings delete global gpu_debug_layers
adb shell settings delete global gpu_debug_layers
if "%ERRORLEVEL%" neq "0" goto error

echo.
echo 8. adb shell settings delete global gpu_debug_layer_app
adb shell settings delete global gpu_debug_layer_app
if "%ERRORLEVEL%" neq "0" goto error

echo.
echo.
echo Cleanup Vulkan application %packageName% is done
goto end

:error
echo.
echo --- Error occurred during ADB execution ---
echo --- Please check the output above for details ---

:end
pause