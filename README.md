### Checkout the code

```
git clone --recursive https://github.com/google/dive.git
```

If the code has been checked out without `--recursive` or you're pulling from the main branch later, please run following command to make sure the submodules are retrieved properly.
```
git submodule update --init --recursive
```

### Prerequisite

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
   - Java 17. Because it uses an older version of Gradle. Set the `JAVA_HOME` environment variable before building:
     ```
     export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
     ```

### Building Dive host tool on Linux
```
# Assumes QTDIR is set to gcc_64 directory (eg. ~/Qt/5.11.2/gcc_64)
export CMAKE_PREFIX_PATH=$QTDIR
export PATH=$QTDIR:$PATH
cd <dive_path>
git submodule update --init --recursive
mkdir build
cd build
cmake -GNinja ..
ninja
```

You can specify the build type for debug/release as well
with `-DCMAKE_BUILD_TYPE=Debug` or `-DCMAKE_BUILD_TYPE=Release` when running cmake.

Host tool:
```
<dive_path>/build/bin/dive_client_cli
```

### Building Dive host tool on Windows
```
REM Assumes QTDIR is set to msvc directory (eg. C:\Qt\5.11.2\msvc2017_64)
set CMAKE_PREFIX_PATH=%QTDIR%
set PATH=%QTDIR%\bin;%PATH%
cd <dive_path>
git submodule update --init --recursive
mkdir build
cd build
cmake -G "Visual Studio 17 2022" ..
```

Open `dive.sln` and build in Debug or Release 
Or run:
```
cmake --build . --config Debug
cmake --build . --config Release
```

Host tool:
```
<dive_path>/build/bin/<build_type>/dive_client_cli.exe
```



### Building Android Libraries


- Download the Android NDK (e.g. 25.2.9519653)
- Set the environment variable `ANDROID_NDK_HOME` (e.g. export ANDROID_NDK_HOME=~/andriod_sdk/ndk/25.2.9519653)
- On Linux, set up environment variables for building GFXReconstruct as explained [here](https://github.com/LunarG/gfxreconstruct/blob/dev/BUILD.md#additional-linux-command-linux-prerequisites)

Run the script 

On Linux, run: 
```
./scripts/build_android.sh Debug
```
And on Windows, Open Developer Command Prompt for VS 2022(or 2019) and run 

```
scripts\build_android.bat Debug
```

This script will:
- Build the debug version of the libraries under `build_android` folder.
  - To build release version, replace parameter with `Release`. 
  - To build both versions, do not pass a parameter.
- Trigger gradle to rebuild gfxreconstruct binaries under `third_party/gfxreconstruct/android/...` and copy them to under `build_android`.
- Place the relevant files under `install` in preparation for deployment to the Android device.

Troubleshooting tips:
- Open the gradle project at `third_party/gfxreconstruct/android` in Android Studio and try making recommended changes to the project and building from there.
- Delete build folders for a clean build
  - `third_party/gfxreconstruct/android/layer/build`
  - `third_party/gfxreconstruct/android/tools/replay/build`
  - `build_android`
- If incremental builds are slow, try building only one version (Debug or Release) and not both

### CLI Tool for Android applications
The command-line tool `dive_client_cli` currently supports capturing OpenXR and Vulkan applications on Android. To capture, please follow the instructions in the sections above to check out the code and build the Android libraries.

To build the CLI tool on Linux, see [Building Dive host tool on Linux](#building-dive-host-tool-on-linux).

To build the CLI tool on Windows, see [Building Dive host tool on Windows](#building-dive-host-tool-on-windows).

To build the Android libraries, see [Building Android Libraries](#building-android-libraries).

You can find out the device serial by run `adb devices` or by `./dive_client_cli --command list_device`

Run `./dive_client_cli --help` for help.

#### Standalone PM4 Capture

Examples:
 - Install the dependencies on device and start the package and do a capture after the applications runs 5 seconds.
 ```
 ./dive_client_cli --device 9A221FFAZ004TL --command capture --package de.saschawillems.vulkanBloom --type vulkan --trigger_capture_after 5 --download_path "/path/to/save/captures"
 ```

 - Install the dependencies on device and start the package
 ```
 ./dive_client_cli --device 9A221FFAZ004TL --command run --package com.google.bigwheels.project_cube_xr.debug --type openxr --download_path "/path/to/save/captures"
 ```
Then you can follow the hint output to trigger a capture by press key `t` and `enter` or exit by press key `enter` only.

The capture files will be saved at the path specified with the `--download_path` option or the current directory if this option not specified. 

#### GFXR Capture
GFXR capturing can be triggered in the ui or within the cli.

To begin a GFXR capture in the ui, either press key `F6` or click `Capture` at the top left corner and select `GFXR Capture` from the dropdown menu.

To begin a GFXR capture with the cli, first ensure you know the correct architecture for the device you are attempting to capture on. This is required when intiating a GFXR capture.

Examples:
 - Install the dependencies on device, start the package, and initiate a GFXR capture.
 ```
 ./dive_client_cli --device 9A221FFAZ004TL --command gfxr_capture --package com.google.bigwheels.project_cube_xr.debug --type vulkan --device_architecture arm64-v8a --gfxr_capture_file_dir gfxr_bigwheels_capture --download_path "/path/to/save/captures"
 ```

Then you can follow the hint output to trigger a capture by pressing key `g` and `enter`, stopping it with the same key combination, or exiting by pressing key `enter`.

The capture file directory will be saved at the path specified with the `--download_path` option or the current directory if this option not specified. 

#### Modifying GFXR File
Modifications to the GFXR file can be made using the Dive Host Tool `host_cli`

Example:
 ```
 ./host_cli --input_file_path original/file.gfxr --output_gfxr_path new/file.gfxr
 ```

#### GFXR Replay

First, push the GFXR capture to the device or find the path where it is located on the device.

If multiple Android Devices are connected, set the enviroment variable `ANDROID_SERIAL` to the device serial in preparation for the GFXR replay script.

Using the `gfxr-replay` command will install the `gfxr-replay.apk` found in the `install` dir, and then replay the specified capture.

Example:
```
./dive_client_cli --device 9A221FFAZ004TL --command gfxr_replay --gfxr_replay_file_path /storage/emulated/0/Download/gfxrFileName.gfxr
```

For a capture that is a single frame, it can be replayed in a loop. Omit the `--loop-single-frame-count` flag for infinite looping.

Example:
```
./dive_client_cli --device 9A221FFAZ004TL  --command gfxr_replay --gfxr_replay_file_path /storage/emulated/0/Download/gfxrFileName.gfxr --gfxr_replay_flags "--loop-single-frame --loop-single-frame-count 300"
```

#### Cleanup

The command line tool will clean up the device and application automatically at exit. If somehow it crashed and left the device in a uncleaned state, you can run following command to clean it up

```
./dive_client_cli --command cleanup --package de.saschawillems.vulkanBloom --device 9A221FFAZ004TL
```
This will remove all the libraries installed and the settings that had been setup by Dive for the package.

### Updating Dive's gfxreconstruct subtree

1. Create a branch to contain the merge
2. Run the pull command: 
```
git subtree pull --prefix=third_party/gfxreconstruct https://github.com/LunarG/gfxreconstruct.git dev --squash
```
3. Resolve any conflicts that arise and ensure dive-specific changes are not removed. Files with dive-specific changes have comment lines: // GOOGLE: or # GOOGLE. If there are conflicts, don't forget to add them and commit:
```
git add third_party/gfxreconstruct
git commit -m "Merge third_party/gfxreconstruct updates"
```
4. Copy missing submodule entries from `//third_party/gfxreconstruct/.gitmodules` into `//.gitmodules`
5. Update submodules:
```
git submodule update --init --recursive
```
6. Regenerate GFXR Vulkan code:
```
cd third_party/gfxreconstruct/framework/generated
python generate_vulkan.py
```
7. Try to build. Fix any errors and commit.
```
cmake --build build
./scripts/build_android.sh Debug
```
8. Create a pull request for the updates.
9. Monitor PR builds; you might need to fix the GitHub workflows.
10. Ensure the commit is not squash merged so that git can find the subtree updates.
