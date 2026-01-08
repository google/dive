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
    rm -rf build

    cmake . -G "Ninja Multi-Config" -Bbuild
    ```
1. Build with one of the following methods:
    * Build directly with ninja
        ```sh
        ninja -C build -f build-Debug.ninja
        ```
    * Build using cmake
        ```sh
        cmake --build build --config=Debug
        ```
    You can specify the build type for release as well by replacing "Debug" with "Release" or "RelWithDebInfo" instead.
1.  Install (the prefix must be coordinated with that of the [device resources](#dive-device-resources))
    ```sh
    cmake --install build --prefix pkg --config Debug
    ```

If you want to build a GFXR tool like `gfxrecon-convert`:

```
cmake --build build --target gfxrecon-convert
```

### Windows

1. Configure
    ```bat
    REM Assumes python is on the path

    cd %DIVE_ROOT_PATH%
    rmdir /s build

    cmake . -G "Visual Studio 17 2022" -Bbuild
    ```
1. Build with one of the following methods:
    * Build with Visual Studio by opening `dive.sln` using IDE and selecting the appropriate build type
    * Build using cmake:
        ```bat
        cmake --build build --config=Debug
        ```
    You can specify other build types as well
    with `--config=<Debug/Release/RelWithDebInfo/MinSizeRel>` instead.
1.  Install (the prefix must be coordinated with that of the [device resources](#dive-device-resources))
    ```bat
    cmake --install build --prefix pkg --config Debug
    ```

If you want to build a GFXR tool like `gfxrecon-convert`:

```
cmake --build build --target gfxrecon-convert
```

## Dive Device Resources

Warning: We only support "Debug" for the gradle build for GFXR portion, so it will be hardcoded and not depend on the build type specified in the script.

Modify the script if necessary to provide the appropriate `ANDROID_ABI` depending on your device.

### Linux

Running the script `scripts/build_android.sh` will build and install the device resources at `$DIVE_ROOT_PATH/pkg/device`.

### Windows

Run the following in the Visual Studio Developer Command Prompt for VS 2022 (or 2019):

Running the script `scripts\build_android.bat` will build and install the device resources at `$DIVE_ROOT_PATH\pkg\device`.

### Troubleshooting Tips
* Gradle build
    * Open the gradle project at `third_party/gfxreconstruct/android` in Android Studio and try making recommended changes to the project and building from there.
    * Delete GFXR build folders for a clean build
        * `third_party/gfxreconstruct/android/layer/build`
        * `third_party/gfxreconstruct/android/tools/replay/build`

## App bundle (macOS)

After building the host tools and the device resources as outlined above and installing them, additional steps are required to make a self-contained macOS application bundle from the contents of the `pkg/` directory.

All plugin folders must be placed under `$DIVE_ROOT_PATH/pkg` and named with substring "_plugin" at the end to be properly added to the bundle.

Running the script `scripts/deploy_mac_bundle.sh` will create the mac package and place it at `$DIVE_ROOT_PATH/pkg/dive.app`.
