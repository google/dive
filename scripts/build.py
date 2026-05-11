#!/usr/bin/env python3
#
# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import common_dive_utils as dive
import enum
import os
import platform
import re
import shutil
import typing

# GLOBAL CONSTANTS
PROFILING_PLUGIN_PATH = "plugins/external/dive_profiling_plugin"
PKG_DIR = "pkg"
# CMake generator names
NINJA_MULTI_CONFIG_NAME = "Ninja Multi-Config"
# For release
WINDOWS_UNNECESSARY_DIRS = ["translations"]
WINDOWS_QT_DEPENDENCIES_BINARY = "dive_gui_lib.dll"
MAC_HOST_PLUGINS_DIR = "Contents/PlugIns/plugins"
# For parsing version string
HOST_TOOLS_BUILD_NAME = "Host Tools Build"


class Action:
    def __init__(self, name: str, func: typing.Callable[[argparse.Namespace], None]):
        assert name == name.lower()
        self.name = name
        self.func = func

    def __str__(self):
        return self.name

    def __call__(self, args: argparse.Namespace):
        self.func(args)


available_actions: list[Action] = []


class BuildType(enum.StrEnum):
    DEBUG = "Debug"
    REL_WITH_DEB_INFO = "RelWithDebInfo"
    RELEASE = "Release"


def get_default_deployqt():
    match platform.system():
        case "Windows":
            return "windeployqt"
        case "Darwin":
            return "macdeployqt"
        case _:
            return None


def get_default_host_cmake_generator() -> str:
    match platform.system():
        case "Windows":
            return "Visual Studio 17 2022"
        case _:
            return NINJA_MULTI_CONFIG_NAME


def rmtree_if_exists(path):
    if os.path.exists(path):
        shutil.rmtree(path)
        print(f"\nDeleted existing dir: {path}")


def parse_args():
    parser = argparse.ArgumentParser(
        prog="build",
        description="This script automates the standard build process for Dive",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--action",
        action="append",
        default=[],
        choices=[str(action) for action in available_actions],
        help="List of actions that this script will perform. "
        "If unspecified, then all actions will be performed",
    )
    parser.add_argument("--actions", default="", help="Deprecated")
    parser.add_argument(
        "--archive-dir",
        help="Output directory of the final archive produced by this script. "
        f"If unspecified, the --root-build-dir will be used",
    )
    parser.add_argument(
        "--archive-name",
        help="Base name (no extension) of the final archive produced by this "
        f"script. If unspecified, the '{HOST_TOOLS_BUILD_NAME}' version string will be used",
    )
    parser.add_argument(
        "--build-type",
        type=BuildType,
        default=BuildType.DEBUG,
        choices=[x for x in BuildType],
        help="Build type for Dive (excluding GFXR gradle build which is always Debug)",
    )
    parser.add_argument(
        "--build-via-generator",
        action=argparse.BooleanOptionalAction,
        default=False,
        help="Perform the build step by invoking the generator rather than cmake, potentially faster on Windows platform",
    )
    parser.add_argument(
        "--ci",
        action=argparse.BooleanOptionalAction,
        default=False,
        help="Build is part of continuous integration",
    )
    parser.add_argument(
        "--dive-release-type",
        default="dev",
        help="Description string for Dive version info",
    )
    parser.add_argument(
        "--exec-cmake",
        default="cmake",
        help="The name of the CMake executable on the path",
    )
    parser.add_argument(
        "--exec-codesign",
        default="codesign",
        help="The name of the codesign executable on the path (macOS only)",
    )
    parser.add_argument(
        "--exec-deployqt",
        default=get_default_deployqt(),
        help="The path to the platform-specific deployqt executable",
    )
    parser.add_argument(
        "--exec-msbuild",
        default="msbuild",
        help="The name of the Microsoft Build Engine executable on the path",
    )
    parser.add_argument(
        "--exec-ninja",
        default="ninja",
        help="The name of the ninja executable on the path",
    )
    parser.add_argument(
        "--host-configure-flag",
        action="append",
        default=[],
        help="String of additional CMake flags to pass directly to cmake in the "
        "configure_host action",
    )
    parser.add_argument(
        "--host-configure-additional-flags", default="", help="Deprecated"
    )
    parser.add_argument(
        "--mac-sign",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Sign the macOS app bundle, no-op for other platforms",
    )
    parser.add_argument(
        "--prereq-checks",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Check if prerequisites are located as expected",
    )
    parser.add_argument(
        "--root-build-dir",
        default="build",
        help="The build directory, relative to Dive root directory",
    )
    parser.add_argument(
        "--host-cmake-generator",
        default=get_default_host_cmake_generator(),
        help="CMake generator to use for host builds",
    )
    return parser.parse_args()


def parse_actions(args) -> list[str]:
    """Converts args.actions to an ordered list of actions"""

    allowed_actions = set(str(action) for action in available_actions)
    actions = set(args.action) | set(filter(None, args.actions.split(",")))
    for action in actions:
        if action not in allowed_actions:
            raise Exception(f"Invalid action specified: '{action}'")
    if not actions:
        actions = allowed_actions

    # Getting an ordered list of actions
    return [action for action in available_actions if str(action) in actions]


def check_environment(args):
    """Checks for env vars and executables that will be used by this script.
    Overwrites executables with their absolute path to satisfy the warning for
    subprocess.Popen(). This step can be skipped with --no-prereq-checks
    """

    if args.build_type != BuildType.DEBUG:
        print(
            "WARNING: GFXR gradle build is hardcoded to Debug regardless of --build_type"
        )

    if "ANDROID_NDK_HOME" not in os.environ:
        raise Exception("ANDROID_NDK_HOME env var must be set")
    print(f"\nANDROID_NDK_HOME found: {os.environ['ANDROID_NDK_HOME']}")

    cmake = shutil.which(args.exec_cmake)
    if not cmake:
        raise Exception(f"Cannot find cmake exec {args.exec_cmake}")
    cmd = [cmake, "--version"]
    dive.echo_and_run(cmd)
    args.exec_cmake = cmake

    ninja = shutil.which(args.exec_ninja)
    if not ninja:
        raise Exception(f"Cannot find ninja exec: {args.exec_ninja}")
    cmd = [ninja, "--version"]
    dive.echo_and_run(cmd)
    args.exec_ninja = ninja

    if (platform.system() == "Windows") and (args.build_via_generator):
        msbuild = shutil.which(args.exec_msbuild)
        if not msbuild:
            raise Exception(f"Cannot find msbuild exec: {args.exec_msbuild}")
        cmd = [msbuild, "--version"]
        dive.echo_and_run(cmd)
        args.exec_msbuild = msbuild

    # Cannot check the flag --version because for some reason the return code is nonzero
    if args.exec_deployqt:
        deployqt = shutil.which(args.exec_deployqt)
        if not deployqt:
            raise Exception(f"{args.exec_deployqt} not found on the Path")
        print(f"\n{args.exec_deployqt} found on the Path at {deployqt}")
        args.exec_deployqt = deployqt

    if (platform.system() == "Darwin") and (args.mac_sign):
        codesign = shutil.which(args.exec_codesign)
        if not codesign:
            raise Exception(f"Cannot find codesign exec: {args.exec_codesign}")
        args.exec_codesign = codesign


def copy_plugins(args):
    """Implements COPY_PLUGINS action"""

    if not os.path.exists(PROFILING_PLUGIN_PATH):
        print(f"\nDir {PROFILING_PLUGIN_PATH} does not exist, skipping")
        return

    print("\nCopying over known external device plugin: dive_profiling_plugin...")
    if platform.system() == "Darwin":
        profiling_dest = f"{args.root_build_dir}/{PKG_DIR}/dive.app/Contents/Resources/dive_profiling_plugin"
    else:
        profiling_dest = (
            f"{args.root_build_dir}/{PKG_DIR}/device/plugins/dive_profiling_plugin"
        )
    shutil.copytree(PROFILING_PLUGIN_PATH, profiling_dest, dirs_exist_ok=True)


def configure_host(args):
    """Implements CONFIGURE_HOST action, uses cmake to generate
    build files targeting the host
    """
    print("\nGenerating build files with cmake...")
    cmd = [
        args.exec_cmake,
        ".",
        f"-G{args.host_cmake_generator}",
        f"-B{args.root_build_dir}/host",
        f"-DDIVE_RELEASE_TYPE={args.dive_release_type}",
        f"-DDIVE_STR_BUILD={args.root_build_dir}",
    ]
    cmd.extend(args.host_configure_flag)
    cmd.extend(filter(None, args.host_configure_additional_flags.split(" ")))
    dive.echo_and_run(cmd)


def build_host(args):
    """Implementing BUILD_HOST action"""
    if not args.build_via_generator:
        print("\nBuilding host libraries by invoking cmake...")
        cmd = [
            args.exec_cmake,
            "--build",
            f"{args.root_build_dir}/host",
            "--config",
            f"{args.build_type}",
        ]
        # Force cmake to run parallel build on Windows.
        # Linux/macOS uses ninja, which do parallel builds by default.
        if platform.system() == "Windows":
            cmd.append(f"-j{os.cpu_count()}")
    elif platform.system() in ["Linux", "Darwin"]:
        print("\nBuilding host libraries by invoking Ninja...")
        cmd = [
            args.exec_ninja,
            "-f",
            f"build-{args.build_type}.ninja",
            "-C",
            f"{args.root_build_dir}/host",
        ]
    elif platform.system() == "Windows":
        print("\nBuilding host libraries by invoking Visual Studio...")
        cmd = [
            args.exec_msbuild,
            f"{args.root_build_dir}/host/ALL_BUILD.vcxproj",
            "-maxCpuCount",
            "-t:Rebuild",
            f"-p:Configuration={args.build_type}",
        ]
    else:
        raise Exception(f"Unrecognized platform: {platform.system()}")
    dive.echo_and_run(cmd)


def install_host(args):
    """Implementing INSTALL_HOST action"""
    # TODO: b/482108095 - This must be done regardless of clean build because
    # install folders are re-used between different build types and the
    # timestamps could affect whether files within are correctly replaced or not
    rmtree_if_exists(f"{args.root_build_dir}/{PKG_DIR}/host")

    print("\nInstalling with cmake...")
    cmd = [
        args.exec_cmake,
        "--install",
        f"{args.root_build_dir}/host",
        "--prefix",
        f"{args.root_build_dir}/{PKG_DIR}",
        "--config",
        f"{args.build_type}",
    ]
    dive.echo_and_run(cmd)


def all_device(args):
    """Implementing ALL_DEVICE action"""
    print("\nGenerating build files with cmake...")
    android_ndk_home = os.environ["ANDROID_NDK_HOME"]
    cmd = [
        args.exec_cmake,
        ".",
        f"-DCMAKE_TOOLCHAIN_FILE={android_ndk_home}/build/cmake/android.toolchain.cmake",
        f"-G{NINJA_MULTI_CONFIG_NAME}",
        f"-B{args.root_build_dir}/device",
        "-DCMAKE_SYSTEM_NAME=Android",
        "-DANDROID_ABI=arm64-v8a",
        "-DANDROID_PLATFORM=android-26",
        "-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER",
        "-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER",
        f"-DDIVE_RELEASE_TYPE={args.dive_release_type}",
        f"-DDIVE_STR_BUILD={args.root_build_dir}",
    ]
    if args.ci:
        cmd.append("-DDIVE_GFXR_GRADLE_CONSOLE=plain")
    dive.echo_and_run(cmd)

    print("\nBuilding with ninja...")
    cmd = [
        args.exec_ninja,
        "-C",
        f"{args.root_build_dir}/device",
        "-f",
        f"build-{args.build_type}.ninja",
    ]
    dive.echo_and_run(cmd)

    # TODO: b/482108095 - This must be done regardless of clean build because
    # install folders are re-used between different build types and the
    # timestamps could affect whether files within are correctly replaced or not
    rmtree_if_exists(f"{args.root_build_dir}/{PKG_DIR}/device")

    print("\nInstalling with cmake...")
    cmd = [
        args.exec_cmake,
        "--install",
        f"{args.root_build_dir}/device",
        "--prefix",
        f"{args.root_build_dir}/{PKG_DIR}",
        "--config",
        f"{args.build_type}",
    ]
    dive.echo_and_run(cmd)


def deploy_qt(args):
    """Implements DEPLOY_QT action, which uses a QT tool to add QT
    libraries to the same dir as the binary that depends on them in preparation
    for packaging and distribution
    """
    # TODO: b/504654590 - Investigate if there's a way to identify the binaries without hardcoding them
    system_name = platform.system()
    match system_name:
        case "Linux":
            print(
                "\ndeploy_qt() is a no-op for Linux, it's expected that the users install Qt..."
            )

        case "Darwin":
            print(f"\nDeploying with {args.exec_deployqt}...")
            app_dir = f"{args.root_build_dir}/{PKG_DIR}/dive.app"
            cmd = [args.exec_deployqt, app_dir]
            for root, dirs, files in os.walk(f"{app_dir}/{MAC_HOST_PLUGINS_DIR}"):
                for file in files:
                    if file.endswith(".dylib") or file.endswith(".so"):
                        plugin_path = os.path.join(root, file)
                        cmd.append(f"-executable={plugin_path}")
            dive.echo_and_run(cmd)

            if args.mac_sign:
                print(f"\nSigning with {args.exec_codesign}...")
                cmd = [args.exec_codesign, "--force", "--deep", "--sign", "-", app_dir]
                dive.echo_and_run(cmd)

        case "Windows":
            print(f"\nDeploying with {args.exec_deployqt}...")
            cmd = [
                args.exec_deployqt,
                f"{args.root_build_dir}/{PKG_DIR}/host/{WINDOWS_QT_DEPENDENCIES_BINARY}",
            ]
            dive.echo_and_run(cmd)

        case _:
            raise Exception(f"Unrecognized platform: {system_name}")


def get_archive_name(args, unparsed_string):
    """Given unparsed_string (output of host tool run with --version flag) and
    args, determine the appropriate archive name (without extension)
    """
    if args.archive_name:
        return args.archive_name

    pattern = r"(?<=" + re.escape(HOST_TOOLS_BUILD_NAME) + r"\:\s)\S+"
    match_obj = re.search(pattern, unparsed_string)

    if not match_obj:
        raise Exception(f"Unsupported format of version string:\n{unparsed_string}")

    print(f"Found host version: {match_obj.group()}")

    return "dive-" + match_obj.group()


def package(args):
    """Implements PACKAGE action, which will package this Dive build
    into a single file that is easy to transfer
    """
    # TODO: b/504654590 - Investigate if there's a way to identify extraneous files
    system_name = platform.system()
    match system_name:
        case "Linux":
            cmd = [f"{args.root_build_dir}/{PKG_DIR}/host/dive_client_cli", "--version"]

        case "Darwin":
            cmd = [
                f"{args.root_build_dir}/{PKG_DIR}/dive.app/Contents/MacOS/dive_client_cli",
                "--version",
            ]

        case "Windows":
            for dir_name in WINDOWS_UNNECESSARY_DIRS:
                rmtree_if_exists(f"{args.root_build_dir}/{PKG_DIR}/host/{dir_name}")
            cmd = [
                f"{args.root_build_dir}/{PKG_DIR}/host/dive_client_cli.exe",
                "--version",
            ]

        case _:
            raise Exception(f"Unrecognized platform: {system_name}")

    long_version_string = dive.run_and_return_output(cmd)
    archive_name = get_archive_name(args, long_version_string)

    print(f"\nZipping into '{archive_name}.zip'...")

    match platform.system():
        case "Darwin":
            dive.echo_and_run(
                [
                    "ditto",
                    "-c",
                    "-k",
                    "--sequesterRsrc",
                    "--keepParent",
                    f"{args.root_build_dir}/{PKG_DIR}/dive.app",
                    f"{archive_name}.zip",
                ]
            )
        case _:
            shutil.make_archive(archive_name, "zip", f"{args.root_build_dir}/{PKG_DIR}")

    archive_dir = args.archive_dir
    if not archive_dir:
        archive_dir = f"{os.getcwd()}/{args.root_build_dir}"
    final_location_zip = f"{archive_dir}/{archive_name}.zip"
    if os.path.exists(final_location_zip):
        print("\nClearing previous archive...")
        os.unlink(final_location_zip)

    print(f"\nMoving zip archive to {final_location_zip}...")
    shutil.move(f"{archive_name}.zip", final_location_zip)


available_actions.extend(
    [
        Action("configure_host", configure_host),
        Action("build_host", build_host),
        Action("install_host", install_host),
        Action("all_device", all_device),
        Action("copy_plugins", copy_plugins),
        Action("deploy_qt", deploy_qt),
        Action("package", package),
    ]
)


def main():
    dive.check_python_version()
    args = parse_args()

    actions = parse_actions(args)

    str_actions = ", ".join(map(str, actions))
    print(f"\nQueued actions: {str_actions}")

    if not args.ci:
        dive_root_path = dive.get_dive_root()
        os.chdir(dive_root_path)

    if args.prereq_checks:
        print(f"\nChecking build environment...")
        check_environment(args)

    with dive.Timer("total"):
        for action in actions:
            with dive.Timer(str(action)):
                action(args)


if __name__ == "__main__":
    main()
