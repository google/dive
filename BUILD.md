# Prerequisites

- CMake
- Ninja
- The QT framework, can be installed from [QT online installer](https://download.qt.io/archive/online_installers/4.6/). We are currently using QT 5.15.2. Note that to install QT 5.15.2 from the online installer, you have to enable (turn on) the `archived` versions and then click on `filter`.
- Android NDK (currently we are using 25.2.9519653).
- Python is installed with `python` in your PATH. It is recommended to use a virtual environment such as virtualenv or pipenv. Alternatively, on Debian, you can `sudo apt install python-is-python3`.
- Mako Templates for Python: can be installed with following commandline
    ```sh
    pip install Mako
    ```
- gfxreconstruct [dependencies](https://github.com/LunarG/gfxreconstruct/blob/dev/BUILD.md#android-development-requirements), if targetting Android. Specifically:
    - Android Studio. Make sure to install an SDK and accept the licenses.
    - On Linux, set up environment variables for building GFXReconstruct as explained [here](https://github.com/LunarG/gfxreconstruct/blob/dev/BUILD.md#additional-linux-command-linux-prerequisites)
        - Note: Use Java 17, because this uses an older version of Gradle.

## Environment Variables

Add permanently or per-session as desired.

### Linux
```sh
# Example setup

# Android NDK prerequisite
export ANDROID_NDK_HOME=~/android_sdk/ndk/25.2.9519653

# gfxreconstruct prerequisite
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64

# QT prerequisite, gcc_64 directory
export QTDIR=~/Qt/5.11.2/gcc_64
export CMAKE_PREFIX_PATH=$QTDIR
export PATH=$QTDIR:$PATH

# Recommended but not necessary
export DIVE_ROOT_PATH=/path/to/dive
```

### Windows
```bat
REM Example setup

REM Android NDK prerequisite
set ANDROID_NDK_HOME=C:\Users\name\...\Android\Sdk\ndk\25.2.9519653

REM gfxreconstruct prerequisite
set JAVA_HOME=C:\Users\name\...\temurin-17.0.13

REM QT prerequisite, msvc directory
set QTDIR=C:\Users\name\...\Qt\5.11.2\msvc2017_64
set CMAKE_PREFIX_PATH=%QTDIR%
set PATH=%QTDIR%\bin;%PATH%

REM Recommended but not necessary
set DIVE_ROOT_PATH=C:\path\to\dive
```

# Building Dive

## Dive Host Tools

### Linux

1. Configure
    ```sh
    # Assumes python is on the path

    cd $DIVE_ROOT_PATH
    rm -rf build/host
    rm -rf build/pkg/host

    cmake . -G "Ninja Multi-Config" -Bbuild/host
    ```
1. Build
    ```sh
    cmake --build build/host --config=Debug
    ```
    You can specify the build type for release as well
    with `--config=Release` instead.
1.  Install in a predetermined location so that the host tools can locate the device libraries
    ```sh
    cmake --install build/host --prefix build/pkg/host --config Debug
    ```

### Windows

1. Configure
    ```bat
    REM Assumes python is on the path

    cd %DIVE_ROOT_PATH%
    rmdir /s build\host
    rmdir /s build\pkg\host

    cmake . -G "Visual Studio 17 2022" -Bbuild\host
    ```
1. Build with Visual Studio by opening `dive.sln` using IDE and selecting the appropriate build type, or build using cmake:
    ```sh
    cmake --build build\host --config=Debug
    ```
    You can specify other build types as well
    with `--config=<Debug/Release/RelWithDebInfo/MinSizeRel>` instead.
1.  Install in a predetermined location so that the host tools can locate the device libraries
    ```sh
    cmake --install build\host --prefix build\pkg\host --config Debug
    ```

## Dive Device Libraries

Warning: We only support "Debug" for the gradle build for GFXR portion, so it will be hardcoded and not depend on the `--config` below.

Provide the appropriate `ANDROID_ABI` depending on your device.

### Linux

1. Configure
    ```sh
    cd $DIVE_ROOT_PATH

    rm -rf build/device
    rm -rf build/pkg/device

    cmake . -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake \
        -G "Ninja Multi-Config" \
        -Bbuild/device \
        -DCMAKE_MAKE_PROGRAM="ninja" \
        -DCMAKE_SYSTEM_NAME=Android \
        -DANDROID_ABI=arm64-v8a \
        -DANDROID_PLATFORM=android-26 \
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER \
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER
    ```
1. Build
    ```sh
    cmake --build build/device --config=Debug
    ```
1. Install in a predetermined location so that the host tools can locate the device libraries
    ```sh
    cmake --install build/device --prefix build/pkg/device --config Debug
    ```

### Windows

Run the following in the Visual Studio Developer Command Prompt for VS 2022 (or 2019)

1. Configure
    ```bat
    cd %DIVE_ROOT_PATH%

    rmdir /s build\device
    rmdir /s build\pkg\device

    cmake . -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake ^
        -G "Ninja Multi-Config" ^
        -Bbuild\device ^
        -DCMAKE_MAKE_PROGRAM="ninja" ^
        -DCMAKE_SYSTEM_NAME=Android ^
        -DANDROID_ABI=arm64-v8a ^
        -DANDROID_PLATFORM=android-26 ^
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER ^
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER
    ```
1. Build
    ```bat
    cmake --build build\device --config=Debug
    ```
1. Install
    ```bat
    cmake --install build\device --prefix build\pkg\device --config Debug
    ```

### Troubleshooting Tips

- Open the gradle project at `third_party/gfxreconstruct/android` in Android Studio and try making recommended changes to the project and building from there.
- Delete GFXR build folders for a clean build
    - `third_party/gfxreconstruct/android/layer/build`
    - `third_party/gfxreconstruct/android/tools/replay/build`