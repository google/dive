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
setlocal enabledelayedexpansion

set COMPOSITOR_CAPTURE_DIR=/data/local/tmp/gfxr_capture/compositor
set TIME_TO_WAIT_FOR_RESTART=30

adb root
adb shell setenforce 0

adb shell mkdir -p /data/local/debug/vulkan/
adb push install/gfxr_layer/jni/arm64-v8a/libVkLayer_gfxreconstruct.so /data/local/debug/vulkan
adb shell mkdir -p %COMPOSITOR_CAPTURE_DIR%
adb shell setprop debug.gfxrecon.capture_file "%COMPOSITOR_CAPTURE_DIR%/compositor.gfxr"
adb shell setprop debug.gfxrecon.capture_trigger_frames 1
adb shell setprop debug.gfxrecon.capture_android_trigger false
adb shell setprop debug.gfxrecon.capture_use_asset_file true

adb shell setprop persist.compositor.protected_context false
adb shell setprop persist.compositor.tiled_rendering false
adb shell setprop compositor.lazy_depth_buffer false

adb shell setprop cpm.gfxr_layer 1
adb shell stop && adb shell start

REM TODO(b/449739284): find a better way to detect surfaceflinger done initialization.
echo Waiting for surfaceflinger to restart...
timeout /t %TIME_TO_WAIT_FOR_RESTART% /nobreak

GOTO :while_loop
:while_loop
    echo Press key g+enter to trigger a capture and g+enter to retrieve the capture.
    echo Press any other key+enter to stop the application.
    set /p reply=
    if /i not "!reply!" == "g" (
        GOTO :end_while
    )

    adb shell setprop debug.gfxrecon.capture_android_trigger true

    echo Press any key to retrieve capture
    pause >nul
    adb shell setprop debug.gfxrecon.capture_android_trigger false
    adb pull %COMPOSITOR_CAPTURE_DIR% ./captures

    for /f %%f in ('adb shell ls %COMPOSITOR_CAPTURE_DIR%') do (
        adb shell rm "%COMPOSITOR_CAPTURE_DIR%/%%f"
    )
    GOTO :while_loop
:end_while

REM clean up
adb shell rm -rf %COMPOSITOR_CAPTURE_DIR%
adb shell rm /data/local/debug/vulkan/libVkLayer_gfxreconstruct.so
adb shell setprop cpm.gfxr_layer 0
adb shell setprop persist.compositor.protected_context "\"\""
adb shell setprop persist.compositor.tiled_rendering "\"\""
adb shell setprop compositor.lazy_depth_buffer "\"\""
adb shell stop && adb shell start

endlocal
