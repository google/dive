
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

if "%~7"=="" (
    echo Error: Unexpected number of arguments.
    echo Usage: %~nx0 ^<commit_hash_file^> ^<prebuilt_dir^> ^<metadata_file^> ^<config^> ^<compiler_stamp^> ^<cached_libs^> ^<cached_handler^>
    exit /b 1
)

set COMMIT_HASH_FILE=%~1
set /p COMMIT_HASH=<"%COMMIT_HASH_FILE%"
set PREBUILT_DIR=%~2
set METADATA_FILE=%~3
set CONFIG=%~4
set COMPILER_STAMP=%~5
set CURRENT_STAMP=%CONFIG%_%COMPILER_STAMP%
set CACHED_LIBS_STR=%~6
set CACHED_HANDLER=%~7

if exist "%METADATA_FILE%" (
    set /p CACHED_STAMP=<"%METADATA_FILE%"
    
    if "!CACHED_STAMP!"=="!CURRENT_STAMP!" (
        set ALL_FILES_EXIST=true
        
        if not exist "%CACHED_HANDLER%" (
            echo Crashpad handler missing at %CACHED_HANDLER%
            set ALL_FILES_EXIST=false
        )
        
        if "!ALL_FILES_EXIST!"=="true" (
            for %%f in (%CACHED_LIBS_STR%) do (
                if not exist "%%f" (
                    echo Crashpad library missing at %%f
                    set ALL_FILES_EXIST=false
                )
            )
        )
        
        if "!ALL_FILES_EXIST!"=="true" (
            echo All files exist and metadata matches (!CURRENT_STAMP!^). Skipping Crashpad build.
            exit /b 0
        )
    ) else (
        echo Crashpad metadata mismatch.
        echo   Cached:  !CACHED_STAMP!
        echo   Current: !CURRENT_STAMP!
    )
) else (
    echo Crashpad metadata file not found.
)

if "%CONFIG%"=="Debug" (
    set GN_DEBUG_FLAG=true
    set GN_CFLAGS=/MDd
) else (
    set GN_DEBUG_FLAG=false
    set GN_CFLAGS=/MD
)

echo Starting Crashpad build (Windows_%CONFIG%)...

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

call gn gen out/Default --args="is_debug=%GN_DEBUG_FLAG% target_cpu=\"x64\" extra_cflags=\"%GN_CFLAGS%\""
call ninja -C out/Default

echo Caching artifacts to %PREBUILT_DIR%...
if not exist "%PREBUILT_DIR%" mkdir "%PREBUILT_DIR%"

copy /Y out\Default\crashpad_handler.exe "%PREBUILT_DIR%\"
copy /Y out\Default\obj\third_party\mini_chromium\mini_chromium\base\base.lib "%PREBUILT_DIR%\"
copy /Y out\Default\obj\client\common.lib "%PREBUILT_DIR%\"
copy /Y out\Default\obj\client\client.lib "%PREBUILT_DIR%\"
copy /Y out\Default\obj\util\util.lib "%PREBUILT_DIR%\"

popd

echo !CURRENT_STAMP!>"%METADATA_FILE%"
echo Crashpad build complete and metadata updated.
