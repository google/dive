# Checkout the code

```
git clone --recursive https://github.com/google/dive.git
```

If the code has been checked out without `--recursive` or you're pulling from the main branch later, please run following command to make sure the submodules are retrieved properly.
```
git submodule update --init --recursive
```

# Prerequisites

- CMake
- Ninja
- The QT framework, can be installed from [QT online installer](https://download.qt.io/archive/online_installers/4.6/). We are currently using QT 5.15.2. Note that to install QT 5.15.2 from the online installer, you have to enable (turn on) the `archived` versions and then click on `filter`.
- Android NDK (currently we are using 25.2.9519653). Set the `ANDROID_NDK_HOME` environment variable.
```
export ANDROID_NDK_HOME=~/android_sdk/ndk/25.2.9519653
``` 
- Python is installed with `python` in your PATH. It is recommended to use a virtual environment such as virtualenv or pipenv. Alternatively, on Debian, you can `sudo apt install python-is-python3`.
- Mako Templates for Python: can be installed with following commandline
```
pip install Mako
```
- gfxreconstruct [dependencies](https://github.com/LunarG/gfxreconstruct/blob/dev/BUILD.md#android-development-requirements), if targetting Android. Specifically:
  - Android Studio. Make sure to install an SDK and accept the licenses.
  - On Linux, set up environment variables for building GFXReconstruct as explained [here](https://github.com/LunarG/gfxreconstruct/blob/dev/BUILD.md#additional-linux-command-linux-prerequisites)
    - Note: Use Java 17, because this uses an older version of Gradle. Set the `JAVA_HOME` environment variable before building:
    ```
    export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
    ```

# Building

## Building Dive host tool on Linux

```
# Assumes QTDIR is set to gcc_64 directory (eg. ~/Qt/5.11.2/gcc_64)
# Assumes Python is on the PATH

export CMAKE_PREFIX_PATH=$QTDIR
export PATH=$QTDIR:$PATH

cd <dive_path>
rm -rf build

cmake . -GNinja -DCMAKE_BUILD_TYPE=Debug -Bbuild 

ninja -C build
```

You can also specify the build type as Release.

## Building Dive host tool on Windows

```
REM Assumes QTDIR is set to msvc directory (eg. C:\Qt\5.11.2\msvc2017_64)
REM Assumes Python is on the PATH

set CMAKE_PREFIX_PATH=%QTDIR%
set PATH=%QTDIR%\bin;%PATH%

cd <dive_path>
rmdir build

cmake . -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -Bbuild
```

Open `dive.sln` in Visual Studio and build in Debug or Release.

Or run:
```
cmake --build build --config Debug
cmake --build build --config Release
```

## Building Android libraries on Linux

Warning: Release build is not supported

```
cd <dive_path>

rm -rf build_android

cmake . -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake \
    -G "Ninja" \
    -Bbuild_android \
    -DCMAKE_MAKE_PROGRAM="ninja" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_SYSTEM_NAME=Android \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-26 \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER

cmake --build build_android --config=Debug

cmake --install build_android
```

## Building Android libraries on Windows

Warning: Release build is not supported

Note: On Windows, run the following in the Visual Studio Developer Command Prompt for VS 2022 (or 2019)

```
cd <dive_path>

rmdir build_android

cmake  -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake ^
    -G "Ninja"^
    -Bbuild_android ^
    -DCMAKE_MAKE_PROGRAM="ninja" ^
    -DCMAKE_BUILD_TYPE=Debug  ^
    -DCMAKE_SYSTEM_NAME=Android ^
    -DANDROID_ABI=arm64-v8a ^
    -DANDROID_PLATFORM=android-26 ^
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER ^
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER

cmake --build build_android --config=Debug

cmake --install build_android
```

## Troubleshooting tips for building Android libraries

- Open the gradle project at `third_party/gfxreconstruct/android` in Android Studio and try making recommended changes to the project and building from there.
- Delete GFXR build folders for a clean build
  - `third_party/gfxreconstruct/android/layer/build`
  - `third_party/gfxreconstruct/android/tools/replay/build`

# UI

The recommended way of using Dive. Please follow the instructions in the sections above to first build the Dive host tools and the Android libraries.

```
// On Linux
<dive_path>/build/ui/dive

// On Windows
<dive_path>\build\ui\<build_type>\dive.exe
```

# CLI Tools

Please follow the instructions in the sections above to first build the Dive host tools and the Android libraries.

```
// On Linux
<dive_path>/build/bin/dive_client_cli
<dive_path>/build/bin/divecli
<dive_path>/build/bin/host_cli

// On Windows
<dive_path>\build\bin\<build_type>\dive_client_cli.exe
<dive_path>\build\bin\<build_type>\divecli.exe
<dive_path>\build\bin\<build_type>\host_cli.exe
```
## `divecli`
Supports manipulation of PM4-related files and raw strings

## `host_cli`
Supports manipulation of GFXR files

### Modifying GFXR File
Modifications to the GFXR file can be made using the Dive Host Tool `host_cli`

Example:
 ```
 ./host_cli --input_file_path original/file.gfxr --output_gfxr_path new/file.gfxr
 ```

## `dive_client_cli`
Supports capturing OpenXR and Vulkan applications on Android: 
- PM4 capture
- GFXR capture, modification and replay

### Standalone PM4 Capture

Examples:
 - Install the dependencies on device and start the package and do a capture after the applications runs 5 seconds.
 ```
 ./dive_client_cli --device 9A221FFAZ004TL --command capture --package de.saschawillems.vulkanBloom --type vulkan --trigger_capture_after 5 --download_dir "/path/to/save/captures"
 ```

 - Install the dependencies on device and start the package
 ```
 ./dive_client_cli --device 9A221FFAZ004TL --command run --package com.google.bigwheels.project_cube_xr.debug --type openxr --download_dir "/path/to/save/captures"
 ```
Then you can follow the hint output to trigger a capture by press key `t` and `enter` or exit by press key `enter` only.

The capture files will be saved at the path specified with the `--download_dir` option or the current directory if this option not specified.

### GFXR Capture
GFXR capturing can be triggered in the ui or within the cli.

To begin a GFXR capture in the ui, either press key `F6` or click `Capture` at the top left corner and select `GFXR Capture` from the dropdown menu.

To begin a GFXR capture with the cli, first ensure you know the correct architecture for the device you are attempting to capture on. This is required when intiating a GFXR capture.

Examples:
 - Install the dependencies on device, start the package, and initiate a GFXR capture.
 ```
 ./dive_client_cli --device 9A221FFAZ004TL --command gfxr_capture --package com.google.bigwheels.project_cube_xr.debug --type vulkan --device_architecture arm64-v8a --gfxr_capture_file_dir gfxr_bigwheels_capture --download_dir "/path/to/save/captures"
 ```

Then you can follow the hint output to trigger a capture by pressing key `g` and `enter`, stopping it with the same key combination, or exiting by pressing key `enter`.

The capture file directory will be saved at the path specified with the `--download_dir` option or the current directory if this option not specified.

### GFXR Replay

First, push the GFXR capture to the device or find the path where it is located on the device.

If multiple Android Devices are connected, set the enviroment variable `ANDROID_SERIAL` to the device serial in preparation for the GFXR replay script.

Using the `gfxr-replay` command will install the `gfxr-replay.apk` found in the `install` dir, and then replay the specified capture.

Example:
```
./dive_client_cli --device 9A221FFAZ004TL --command gfxr_replay --gfxr_replay_file_path /storage/emulated/0/Download/gfxrFileName.gfxr
```

For a capture that is a single frame, it can be replayed in a loop.

Example:
```
./dive_client_cli --device 9A221FFAZ004TL  --command gfxr_replay --gfxr_replay_file_path /storage/emulated/0/Download/gfxrFileName.gfxr --gfxr_replay_flags "--loop-single-frame-count 300"
```

To trigger analysis during replay, specify `--gfxr_replay_run_type`. See `--help` for all options.

```
./dive_client_cli --device 9A221FFAZ004TL  --command gfxr_replay --gfxr_replay_file_path /storage/emulated/0/Download/gfxrFileName.gfxr --gfxr_replay_run_type pm4_dump
```

### Cleanup

The command line tool will clean up the device and application automatically at exit. If somehow it crashed and left the device in a uncleaned state, you can run following command to clean it up

```
./dive_client_cli --command cleanup --package de.saschawillems.vulkanBloom --device 9A221FFAZ004TL
```
This will remove all the libraries installed and the settings that had been setup by Dive for the package.