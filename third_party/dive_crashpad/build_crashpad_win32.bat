
:: Copyright 2026 Google LLC
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

set SCRIPT_DIR=%~dp0
set PREBUILT_DIR=%SCRIPT_DIR%prebuilt\win32

if not exist "%~1" (
    echo Commit hash file not found: %~1
    exit /b 1
)
set /p COMMIT_HASH=<"%~1"

if not exist depot_tools (
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools depot_tools
)
set PATH=%CD%\depot_tools;%PATH%
call git config --global core.longpaths true

if not exist crashpad (
    call fetch crashpad
)

pushd crashpad
call git fetch origin %COMMIT_HASH%
call git checkout %COMMIT_HASH%
call gclient sync

call gn gen out/Default --args="is_debug=false target_cpu=\"x64\" extra_cflags=\"/MD\""
call ninja -C out/Default

echo Caching artifacts to %PREBUILT_DIR%...
if not exist "%PREBUILT_DIR%" mkdir "%PREBUILT_DIR%"

copy /Y out\Default\crashpad_handler.exe "%PREBUILT_DIR%\"
copy /Y out\Default\obj\third_party\mini_chromium\mini_chromium\base\base.lib "%PREBUILT_DIR%\"
copy /Y out\Default\obj\client\common.lib "%PREBUILT_DIR%\"
copy /Y out\Default\obj\client\client.lib "%PREBUILT_DIR%\"
copy /Y out\Default\obj\util\util.lib "%PREBUILT_DIR%\"

popd
