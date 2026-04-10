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
- dump_syms, which generates debug symbols for release builds, can be installed via the [Rust installer](https://rustup.rs/). Alternatively, on Linux, you can use `sudo apt install cargo`. Also on macOS, you can use `brew install rust`. Once Cargo is set up, run the command `cargo install dump_syms`.
- On Linux, the curl library is required to upload debug symbols to a Crashpad server. You can install it using `sudo apt install libcurl4-openssl-dev`.

For MacOS, they can be installed via the brew command

```sh
brew install cmake ninja qt@5 python openjdk@17
```

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

# Crashpad Symbol Upload (Required for Official Releases)
export CRASHPAD_API_KEY=your_api_key

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

REM Crashpad Symbol Upload (Required for Official Releases)
set CRASHPAD_API_KEY=your_api_key

REM Recommended but not necessary
set DIVE_ROOT_PATH=C:\path\to\dive
```

### MacOS
```sh
# Example setup

# Android NDK prerequisite
export ANDROID_NDK_HOME=~/Library/Android/sdk/ndk/25.2.9519653

# gfxreconstruct prerequisite
export JAVA_HOME=$(brew --prefix openjdk@17)/libexec/openjdk.jdk/Contents/Home

# QT prerequisite
export QTDIR="$(brew --prefix qt@5)"
export CMAKE_PREFIX_PATH=$QTDIR
export PATH=$QTDIR/bin:$PATH

# Recommended but not necessary
export DIVE_ROOT_PATH=~/src/dive
```

# Building Dive

## Host and Device libraries

```
python scripts/build.py
```

## Troubleshooting Tips
* To use Visual Studio UI for the host build, split the build process like so:
    1. `python scripts/build.py --actions configure_host`
    1. Open Visual Studio UI and build target ALL_BUILD
    1. `python scripts/build.py --actions ...` Specify all the following actions after the action "build_host"
* To build with Crashpad, specify `--build-type RelWithDebInfo`. In case the default is Crashpad off, specify additionally `--host-configure-additional-flags "-DDIVE_BUILD_WITH_CRASHPAD=ON"`
* Gradle build
    * Open the gradle project at `third_party/gfxreconstruct/android` in Android Studio and try making recommended changes to the project and building from there.
    * Delete GFXR build folders for a clean build
        * `third_party/gfxreconstruct/android/layer/.cxx`
        * `third_party/gfxreconstruct/android/layer/build`
        * `third_party/gfxreconstruct/android/tools/replay/.cxx`
        * `third_party/gfxreconstruct/android/tools/replay/build`

## App bundle (macOS)

After building the host tools and the device resources as outlined above and installing them, additional steps are required to make a self-contained macOS application bundle from the contents of the `build/pkg/` directory.

All external plugin folders must be placed under `$DIVE_ROOT_PATH/build/pkg/plugins` to be properly added to the bundle.

```sh
python scripts/deploy_mac_bundle.py
```
This will deploy the mac package and place it at `$DIVE_ROOT_PATH/build/pkg/dive.app`.
