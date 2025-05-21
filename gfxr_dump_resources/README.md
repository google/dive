# gfxr_dump_resources

Process a GFXR file and produce a file suitable for use with GFXR `--dump-resources`. This can be used when replay does not work as expected for your application.

Usage:

```sh
./build/gfxr_dump_resources/gfxr_dump_resources in_capture.gfxr out_dump_resources.json
```

The capture and JSON can then be pushed to the device and replayed using `--dump-resources`:

```sh
adb push dump_resources.json capture.gfxr capture.gfxa /sdcard/Download

python third_party/gfxreconstruct/android/scripts/gfxrecon.py replay \
    --dump-resources /sdcard/Download/dump_resources.json \
    --dump-resources-dir /sdcard/Download/dump \
    /scard/Download/capture.gfxr

adb pull /sdcard/Download/dump
```

`replay-with-dump.sh` is a helper script which performs all the steps mentioned above:

```sh
./gfxr_dump_resources/replay-with-dump.sh in_capture.gfxr in_capture.gfxa
```

## Architecture

The GFXR file is parsed top to bottom for Vulkan instructions by FileProcessor with a VulkanDecoder. Vulkan instructions are forwarded to our custom DumpResourcesBuilderConsumer. DumpResourcesBuilderConsumer checks if there's any in-flight command buffers and sends the request through the state machine for that command buffer. The state machine validates that Vulkan calls appear in the expected order as well as accumulating that info into the DumpEntry struct. If all the required info is found then the complete DumpEntry is emitted. At the end, all complete DumpEntry's are written to disk as JSON.

This has only been tested on a handfull of BigWheels samples: cube_xr, fishtornado_xr, and sample_04_cube. Other captures will probably require implementing new Vulkan calls; to implement new calls:

1. Modify DumpResourcesBuilderConsumer to override `Process_vk*()`. The implementation for this function typically involves finding if there's an incomplete dump for the command buffer then forwarding the call to the state machine. Look at VulkanConsumer for the right signature.
2. Figure out which state to modify. Override `Process_vk*()` to translate the info into the DumpEntry and transition to an new state.
3. If you need to make a new state, instantiate it (probably in StateMachine) and set up the state transitions. Then, override `Process_vk*()` it needs to process.

## Limitations

This does not know how to handle dynamic rendering.
