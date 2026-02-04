:: Copyright 2025 Google LLC
::
:: Licensed under the Apache License, Version 2.0 (the "License");
:: you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at
::
:: http://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: See the License for the specific language governing permissions and
:: limitations under the License.

:: Usage: replay-with-dump.bat <GFXR_filename>.gfxr <GFXA_filename>.gfxa

@echo off
setlocal

IF "%~1"=="" (
    echo Usage: replay-with-dump.bat GFXR [GFXA]
    exit /b 1
)

SET GFXR=%~1

IF NOT "%GFXR:~-5%"==".gfxr" (
    echo Error: First argument GFXR must have a .gfxr extension.
    exit /b 1
)

FOR %%i IN ("%GFXR%") DO SET GFXR_BASENAME=%%~nxi
SET BUILD_DIR=../build
SET JSON_BASENAME=dump.json
SET JSON=%BUILD_DIR%/%JSON_BASENAME%
:: Fairly reliable directory on remote device, as long as app has MANAGE_EXTERNAL_STORAGE permissions.
:: /data/local/tmp doesn't work on all devices tested.
SET REMOTE_TEMP_DIR=/sdcard/Download
SET PUSH_DIR=%REMOTE_TEMP_DIR%/replay
SET DUMP_DIR=%REMOTE_TEMP_DIR%/dump

SET GFXR_DUMP_RESOURCES_BASE_PATH=%BUILD_DIR%/gfxr_dump_resources

IF NOT EXIST "%BUILD_DIR%" (
    echo Error: %BUILD_DIR% folder does not exist. Please build the project first.
    exit /b 1
)

IF EXIST "%GFXR_DUMP_RESOURCES_BASE_PATH%/Debug/gfxr_dump_resources.exe" (
    SET "GFXR_DUMP_RESOURCES=%GFXR_DUMP_RESOURCES_BASE_PATH%/Debug/gfxr_dump_resources.exe"
    GOTO :GFXR_DUMP_RESOURCES_FOUND
) ELSE IF EXIST "%GFXR_DUMP_RESOURCES_BASE_PATH%/Release/gfxr_dump_resources.exe" (
    SET "GFXR_DUMP_RESOURCES=%GFXR_DUMP_RESOURCES_BASE_PATH%/Release/gfxr_dump_resources.exe"
    GOTO :GFXR_DUMP_RESOURCES_FOUND
) ELSE IF EXIST "%GFXR_DUMP_RESOURCES_BASE_PATH%/gfxr_dump_resources.exe" (
    SET "GFXR_DUMP_RESOURCES=%GFXR_DUMP_RESOURCES_BASE_PATH%/gfxr_dump_resources.exe"
    GOTO :GFXR_DUMP_RESOURCES_FOUND
)
echo Error: gfxr_dump_resources.exe not found in %BUILD_DIR%
exit /b 1

:GFXR_DUMP_RESOURCES_FOUND
echo Debug: Found gfxr_dump_resources.exe at: "%GFXR_DUMP_RESOURCES%"

SET DIVE_CLIENT_CLI_BASE_PATH=%BUILD_DIR%/bin

IF EXIST "%DIVE_CLIENT_CLI_BASE_PATH%/Debug/dive_client_cli.exe" (
    SET "DIVE_CLIENT_CLI=%DIVE_CLIENT_CLI_BASE_PATH%/Debug/dive_client_cli.exe"
    GOTO :DIVE_CLIENT_CLI_FOUND
) ELSE IF EXIST "%DIVE_CLIENT_CLI_BASE_PATH%/Release/dive_client_cli.exe" (
    SET "DIVE_CLIENT_CLI=%DIVE_CLIENT_CLI_BASE_PATH%/Release/dive_client_cli.exe"
    GOTO :DIVE_CLIENT_CLI_FOUND
) ELSE IF EXIST "%DIVE_CLIENT_CLI_BASE_PATH%/dive_client_cli.exe" (
    SET "DIVE_CLIENT_CLI=%DIVE_CLIENT_CLI_BASE_PATH%/dive_client_cli.exe"
    GOTO :DIVE_CLIENT_CLI_FOUND
)
echo Error: dive_client_cli.exe not found in %BUILD_DIR%
exit /b 1

:DIVE_CLIENT_CLI_FOUND
echo Debug: Found dive_client_cli.exe at: "%DIVE_CLIENT_CLI%"

call "%GFXR_DUMP_RESOURCES%" --last_draw_only "%GFXR%" "%JSON%"
IF %ERRORLEVEL% NEQ 0 (
    echo Error running gfxr_dump_resources.
    exit /b %ERRORLEVEL%
)

:: Remove the previous folders if they already exist
adb shell rm -rf "%DUMP_DIR%"
adb shell rm -rf "%PUSH_DIR%"

adb shell mkdir -p "%PUSH_DIR%"
adb shell mkdir -p "%DUMP_DIR%"
adb push "%GFXR%" "%JSON%" "%PUSH_DIR%"
IF %ERRORLEVEL% NEQ 0 (
    echo Error pushing files to device.
    exit /b %ERRORLEVEL%
)

if "%~2"=="" (
    goto REPLAY
)

SET GFXA=%~2
adb push "%GFXA%" "%PUSH_DIR%"
IF %ERRORLEVEL% NEQ 0 (
    echo Error pushing GFXA to device.
    exit /b %ERRORLEVEL%
)

:REPLAY
call "%DIVE_CLIENT_CLI%" --command gfxr_replay ^
    --gfxr_replay_file_path "%PUSH_DIR%/%GFXR_BASENAME%" ^
    --gfxr_replay_flags "--dump-resources %PUSH_DIR%/%JSON_BASENAME% --dump-resources-dir %DUMP_DIR%"

adb pull "%DUMP_DIR%"
IF %ERRORLEVEL% NEQ 0 (
    echo Error pulling replay dumped resources.
    exit /b %ERRORLEVEL%
)

adb shell rm -rf "%DUMP_DIR%"
adb shell rm -rf "%PUSH_DIR%"

endlocal
EXIT /b 0
