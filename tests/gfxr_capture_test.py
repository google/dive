import contextlib
import pathlib
import time
from mobly import asserts
from mobly import base_test
from mobly import test_runner
from mobly.controllers import android_device
from mobly.controllers.android_device_lib import apk_utils

DEVICE_BUILD_DIR = pathlib.Path(__file__).parent.parent / "build" / "pkg" / "device"
GFXR_REPLAY_APK = DEVICE_BUILD_DIR / "gfxr-replay.apk"
BIGWHEELS_SAMPLE_04_CUBE_PACKAGE = "com.google.bigwheels.project_sample_04_cube.debug"
BIGWHEELS_SAMPLE_04_CUBE_ACTIVITY = "com.google.bigwheels.MainActivity"
REPLAY_PACKAGE = "com.lunarg.gfxreconstruct.replay"
CAPTURE_LAYER_NAME = "VK_LAYER_LUNARG_gfxreconstruct"
REMOTE_CAPTURE_DIR = pathlib.PurePosixPath("/sdcard/Download/capture")


@contextlib.contextmanager
def gfxr_capture_settings(
    device: android_device.AndroidDevice,
    capture_file: pathlib.PurePosixPath,
    frame_count: int = 1,
    use_asset_file: bool = True,
):
    adb = device.adb
    device.log.info("Setting GFXR capture properties")
    adb.shell(["mkdir", "-p", str(capture_file.parent)])
    adb.shell(["setprop", "debug.gfxrecon.capture_file", str(capture_file)])
    adb.shell(["setprop", "debug.gfxrecon.capture_trigger_frames", str(frame_count)])
    # capture_android_trigger must be set in order for GFXR to listen for triggers.
    adb.shell(["setprop", "debug.gfxrecon.capture_android_trigger", "false"])
    adb.shell(
        [
            "setprop",
            "debug.gfxrecon.capture_use_asset_file",
            "true" if use_asset_file else "false",
        ]
    )
    try:
        yield
    finally:
        device.log.info("Cleaning up GFXR capture properties")
        adb.shell(["setprop", "debug.gfxrecon.capture_use_asset_file", "''"])
        adb.shell(["setprop", "debug.gfxrecon.capture_android_trigger", "''"])
        adb.shell(["setprop", "debug.gfxrecon.capture_trigger_frames", "''"])
        adb.shell(["setprop", "debug.gfxrecon.capture_file", "''"])


@contextlib.contextmanager
def enable_app_layer(
    device: android_device.AndroidDevice,
    debug_app: str,
    debug_layers: str,
    debug_layer_app: str,
):
    adb = device.adb
    device.log.info("Enabling GPU debug layer")
    adb.shell(["settings", "put", "global", "enable_gpu_debug_layers", "1"])
    adb.shell(["settings", "put", "global", "gpu_debug_app", debug_app])
    adb.shell(
        [
            "settings",
            "put",
            "global",
            "gpu_debug_layers",
            debug_layers,
        ]
    )
    # Using gpu_debug_layer_app supports both debuggable apps and non-debuggable apps on a rooted device
    adb.shell(
        [
            "settings",
            "put",
            "global",
            "gpu_debug_layer_app",
            debug_layer_app,
        ]
    )

    try:
        yield
    finally:
        device.log.info("Cleaning up GPU debug layer")
        adb.shell(["settings", "delete", "global", "enable_gpu_debug_layers"])
        adb.shell(["settings", "delete", "global", "gpu_debug_app"])
        adb.shell(["settings", "delete", "global", "gpu_debug_layers"])
        adb.shell(["settings", "delete", "global", "gpu_debug_layer_app"])


@contextlib.contextmanager
def no_verifier_on_adb_installs(device: android_device.AndroidDevice):
    device.adb.shell(["settings", "put", "global", "verifier_verify_adb_installs", "0"])
    try:
        yield
    finally:
        device.adb.shell(
            ["settings", "delete", "global", "verifier_verify_adb_installs"]
        )


@contextlib.contextmanager
def screen_on(device: android_device.AndroidDevice):
    device.adb.shell(["input", "keyevent", "KEYCODE_WAKEUP"])
    try:
        yield
    finally:
        device.adb.shell(["input", "keyevent", "KEYCODE_SLEEP"])


@contextlib.contextmanager
def launch_app(
    device: android_device.AndroidDevice,
    package: str,
    activity: str,
    options: str | None = None,
):
    device.log.info("Starting %s/%s", package, activity)

    # Apps need permission to write captures to /sdcard
    device.adb.shell(
        ["appops", "set", "--uid", package, "MANAGE_EXTERNAL_STORAGE", "allow"]
    )

    command = ["am", "start", "-S", "-W", "-n", f"{package}/{activity}"]
    if options:
        command.append(options)
    device.adb.shell(command)
    try:
        yield
    finally:
        device.log.info("Stopping %s", package)
        device.adb.shell(["am", "force-stop", package])


def trigger_gfxr_capture(device: android_device.AndroidDevice):
    device.adb.shell(["setprop", "debug.gfxrecon.capture_android_trigger", "true"])
    device.log.info("Waiting for capture trigger to register")
    time.sleep(1)
    device.adb.shell(["setprop", "debug.gfxrecon.capture_android_trigger", "false"])


class GfxrCaptureTest(base_test.BaseTestClass):

    def setup_class(self):
        devices: list[android_device.AndroidDevice] = self.register_controller(
            android_device
        )
        # Arbitrarily choose the first device
        # TODO: Make this logic smarter or allow the user to choose the serial
        self._dut = devices[0]

        with no_verifier_on_adb_installs(self._dut):
            # Replay needs --force-queryable so we can use gpu_debug_layer_app
            apk_utils.install(
                self._dut, str(GFXR_REPLAY_APK), params=["--force-queryable"]
            )

    def test_capture(self):

        log = self._dut.log
        adb = self._dut.adb
        output_path = self.current_test_info.output_path

        with gfxr_capture_settings(
            self._dut, REMOTE_CAPTURE_DIR / f"{BIGWHEELS_SAMPLE_04_CUBE_PACKAGE}.gfxr"
        ), enable_app_layer(
            self._dut,
            BIGWHEELS_SAMPLE_04_CUBE_PACKAGE,
            CAPTURE_LAYER_NAME,
            REPLAY_PACKAGE,
        ), screen_on(
            self._dut
        ), launch_app(
            self._dut,
            BIGWHEELS_SAMPLE_04_CUBE_PACKAGE,
            BIGWHEELS_SAMPLE_04_CUBE_ACTIVITY,
        ):
            log.info("Waiting 2s for app to initialize")
            time.sleep(2)

            log.info("Taking capture")
            trigger_gfxr_capture(self._dut)

            log.info("Waiting 2s for capture to complete")
            time.sleep(2)

            log.info("Pulling results")
            log.info(adb.pull([str(REMOTE_CAPTURE_DIR), output_path]))
            adb.shell(["rm", "-rf", "--", str(REMOTE_CAPTURE_DIR)])

            gfxa_files = list(
                (pathlib.Path(output_path) / REMOTE_CAPTURE_DIR.name).glob("*.gfxa")
            )
            log.info(gfxa_files)
            asserts.assert_true(len(gfxa_files) > 0, "Missing GFXA file!")

            gfxr_files = list(
                (pathlib.Path(output_path) / REMOTE_CAPTURE_DIR.name).glob("*.gfxr")
            )
            log.info(gfxr_files)
            asserts.assert_true(len(gfxr_files) > 0, "Missing GFXR file!")

    def teardown_class(self):
        apk_utils.uninstall(self._dut, "com.lunarg.gfxreconstruct.replay")
        return super().teardown_class()


if __name__ == "__main__":
    test_runner.main()
