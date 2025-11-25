# Checkout the Code

```sh
git clone --recursive https://github.com/google/dive.git
```

If the code has been checked out without `--recursive` or you're pulling from the main branch later, please run following command to make sure the submodules are retrieved properly.
```sh
git submodule update --init --recursive
```

Follow the instructions in [BUILD.md](BUILD.md) to build

# UI

The recommended way of using Dive. Refer to [BUILD.md](BUILD.md) to first build the Dive host tools and the device libraries.

```sh
# On Linux
$DIVE_ROOT_PATH/build/ui/dive
```

```bat
REM On Windows
%DIVE_ROOT_PATH%\build\ui\%DIVE_HOST_TOOLS_BUILD_TYPE%\dive.exe
```

TODO(b/462767957): Figure out where these binaries are expected with the install flow

# CLI Tools

Refer to [BUILD.md](BUILD.md) to first build the Dive host tools and the device libraries.

```sh
# On Linux
$DIVE_ROOT_PATH/build/bin/dive_client_cli
$DIVE_ROOT_PATH/build/bin/divecli
$DIVE_ROOT_PATH/build/bin/host_cli
```

```bat
REM On Windows
%DIVE_ROOT_PATH%\build\bin\%DIVE_HOST_TOOLS_BUILD_TYPE%\dive_client_cli.exe
%DIVE_ROOT_PATH%\build\bin\%DIVE_HOST_TOOLS_BUILD_TYPE%\divecli.exe
%DIVE_ROOT_PATH%\build\bin\%DIVE_HOST_TOOLS_BUILD_TYPE%\host_cli.exe
```

TODO(b/462767957): Figure out where these binaries are expected with the install flow

## `divecli`
Supports manipulation of PM4-related files and raw strings

## `host_cli`
Supports manipulation of GFXR files

### Modifying GFXR File
Modifications to the GFXR file can be made using the Dive Host Tool `host_cli`

Example:
```sh
# On Linux
./host_cli --input_file_path ${path/to/original/file.gfxr} --output_gfxr_path ${path/to/new/file.gfxr}
```

## `dive_client_cli`
Supports capturing OpenXR and Vulkan applications on Android:
- PM4 capture
- GFXR capture, modification and replay

### Standalone PM4 Capture

Examples:
- Install the dependencies on device and start the package and do a capture after the applications runs 5 seconds.
    ```sh
    # On Linux
    ./dive_client_cli --command capture --package ${de.saschawillems.vulkanBloom} --type vulkan --trigger_capture_after ${5} --download_dir ${/path/to/save/captures}
    ```

- Install the dependencies on device and start the package
    ```sh
    # On Linux
    ./dive_client_cli --command run --package ${com.google.bigwheels.project_cube_xr.debug} --type openxr --download_dir ${"/path/to/save/captures"}
    ```

Then you can follow the hint output to trigger a capture by press key `t` and `enter` or exit by press key `enter` only.

The capture files will be saved at the path specified with the `--download_dir` option or the current directory if this option not specified.

### GFXR Capture
GFXR capturing can be triggered in the ui or within the cli.

To begin a GFXR capture in the ui, either press key `F6` or click `Capture` at the top left corner and select `GFXR Capture` from the dropdown menu.

To begin a GFXR capture with the cli, first ensure that you built the device libraries targetting the correct architecture for the device you are attempting to capture on.

Examples:
- Install the dependencies on device, start the package, and initiate a GFXR capture.
    ```sh
    # On Linux
    ./dive_client_cli --command gfxr_capture --package ${com.google.bigwheels.project_cube_xr.debug} --type vulkan --gfxr_capture_file_dir ${gfxr_bigwheels_capture} --download_dir ${/path/to/save/captures}
    ```

Then you can follow the hint output to trigger a capture by pressing key `g` and `enter`, stopping it with the same key combination, or exiting by pressing key `enter`.

The capture file directory will be saved at the path specified with the `--download_dir` option or the current directory if this option not specified.

### GFXR Replay

First, push the GFXR capture to the device or find the path where it is located on the device.

If multiple Android Devices are connected, set the enviroment variable `ANDROID_SERIAL` to the device serial in preparation for the GFXR replay script.

Using the `gfxr-replay` command will install the `gfxr-replay.apk` found in the `install` dir, and then replay the specified capture.

Example:
```sh
# On Linux
./dive_client_cli --command gfxr_replay --gfxr_replay_file_path ${/storage/emulated/0/Download/gfxrFileName.gfxr}
```

For a capture that is a single frame, it can be replayed in a loop.

Example:
```sh
# On Linux
./dive_client_cli  --command gfxr_replay --gfxr_replay_file_path ${/storage/emulated/0/Download/gfxrFileName.gfxr} --gfxr_replay_flags "--loop-single-frame-count 300"
```

To replay with the Vulkan Validation Layers, provide `--validation_layer`:

```sh
# On Linux
./dive_client_cli  --command gfxr_replay --gfxr_replay_file_path ${/storage/emulated/0/Download/gfxrFileName.gfxr} --validation_layer
```

To trigger analysis during replay, specify `--gfxr_replay_run_type`. See `--help` for all options.

```sh
# On Linux
./dive_client_cli  --command gfxr_replay --gfxr_replay_file_path ${/storage/emulated/0/Download/gfxrFileName.gfxr} --gfxr_replay_run_type pm4_dump
```

### Device Cleanup

The command line tool will clean up the device and application automatically at exit. If somehow it crashed and left the device in a uncleaned state, you can run following command to clean it up

```sh
# On Linux
./dive_client_cli --command cleanup --package ${de.saschawillems.vulkanBloom}
```
This will remove all the libraries installed and the settings that had been setup by Dive for the package.