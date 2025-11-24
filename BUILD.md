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

# Building Dive

## Dive Host Tools

### Linux

```
# Assumes QTDIR is set to gcc_64 directory (eg. ~/Qt/5.11.2/gcc_64)
# Assumes Python is on the PATH

export CMAKE_PREFIX_PATH=$QTDIR
export PATH=$QTDIR:$PATH

cd <dive_path>
rm -rf build

cmake . -GNinja -DCMAKE_BUILD_TYPE=Debug -Bbuild 

ninja -C build

cmake --install build
```

You can specify the build type for release as well
with `-DCMAKE_BUILD_TYPE=Release` instead.

### Windows

```
REM Assumes QTDIR is set to msvc directory (eg. C:\Qt\5.11.2\msvc2017_64)
REM Assumes Python is on the PATH

set CMAKE_PREFIX_PATH=%QTDIR%
set PATH=%QTDIR%\bin;%PATH%

cd <dive_path>
rmdir /s build

cmake . -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -Bbuild
```

You can specify other build types as well with `-DCMAKE_BUILD_TYPE=<Debug/Release/RelWithDebInfo/MinSizeRel>`. If you are opening the VS IDE to build, make sure the selected build type in the dropdown matches the type specified earlier, to ensure proper version info is reflected by the built host tools.

TODO(b/460765024): Support multi-configuration generator (VS) for Windows build and disallow `-DCMAKE_BUILD_TYPE=` flag

Open `dive.sln` in Visual Studio and build.

Or run:
```
cmake --build build
```

## Dive Device Libraries

### Linux

Warning: Release build is not supported

Provide the appropriate `ANDROID_ABI` depending on your device.

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

### Windows

Warning: Release build is not supported

Provide the appropriate `ANDROID_ABI` depending on your device.

Note: On Windows, run the following in the Visual Studio Developer Command Prompt for VS 2022 (or 2019)

```
cd <dive_path>

rmdir /s build_android

cmake . -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake ^
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

### Troubleshooting Tips

- Open the gradle project at `third_party/gfxreconstruct/android` in Android Studio and try making recommended changes to the project and building from there.
- Delete GFXR build folders for a clean build
  - `third_party/gfxreconstruct/android/layer/build`
  - `third_party/gfxreconstruct/android/tools/replay/build`