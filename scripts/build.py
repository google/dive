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
import shutil


# GLOBAL CONSTANTS
# Dive directory structure relative to root Dive dir
EXTERNAL_PLUGINS_DIR = "plugins/external"
# Build directory structure relative to --root-build-dir
PKG_DIR = "pkg"
APP_DIR = "dive_app"
# CMake generator names
NINJA_MULTI_CONFIG_NAME = "Ninja Multi-Config"
# For release
WINDOWS_UNNECESSARY_DIRS = ["translations"]
WINDOWS_QT_DEPENDENCIES_BINARY = "dive_gui_lib.dll"


class ActionType(enum.StrEnum):
    COPY_PLUGINS = enum.auto()      # copy external plugins into pkg dir
    CONFIGURE_HOST = enum.auto()
    BUILD_HOST = enum.auto()        # separated to make VS builds using the UI easy
    INSTALL_HOST = enum.auto()
    ALL_DEVICE = enum.auto()        # configure, build, and install device libraries
    DEPLOY_QT = enum.auto()
    PACKAGE = enum.auto()           # removes some extraneous files and packages the release


class BuildType(enum.StrEnum):
    DEBUG = "Debug"
    REL_WITH_DEB_INFO = "RelWithDebInfo"
    RELEASE = "Release"


def get_default_deployqt():
    if platform.system() == "Windows":
        return "windeployqt"
    # TODO: b/484082504 - Support other platforms
    return None


def rmtree_if_exists(path):
    if os.path.exists(path):
        shutil.rmtree(path)
        print(f"\nDeleted existing dir: {path}")


def parse_args():
    parser = argparse.ArgumentParser(
        prog="build_android",
        description='This script automates the standard build process for Dive',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        "--actions",
        help="Comma-separated list of actions that this script will perform. "
        "Use --list-actions to check which actions are available. If "
        "unspecified, then all actions will be performed")
    parser.add_argument(
        "--build-type",
        type=BuildType,
        default=BuildType.DEBUG,
        choices=[x for x in BuildType],
        help="Build type for Dive (excluding GFXR gradle build which is always Debug)")
    parser.add_argument(
        "--build-via-generator",
        action=argparse.BooleanOptionalAction,
        default=False,
        help="Perform the build step by invoking the generator rather than cmake, potentially faster on Windows platform")
    parser.add_argument(
        "--prereq-checks",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Check if prerequisites are located as expected")
    parser.add_argument(
        "--ci",
        action=argparse.BooleanOptionalAction,
        default=False,
        help="Build is part of continuous integration")
    parser.add_argument(
        "--dive-release-type",
        default="dev",
        help="Description string for Dive version info")
    parser.add_argument(
        "--exec-cmake",
        default="cmake",
        help="The name of the CMake executable on the path")
    parser.add_argument(
        "--exec-deployqt",
        default=get_default_deployqt(),
        help="The path to the platform-specific deployqt executable")
    parser.add_argument(
        "--exec-msbuild",
        default="msbuild",
        help="The name of the Microsoft Build Engine executable on the path")
    parser.add_argument(
        "--exec-ninja",
        default="ninja",
        help="The name of the ninja executable on the path")
    parser.add_argument(
        "--host-configure-additional-flags",
        help="String of additional CMake flags to pass directly to cmake in the "
        "configure_host stage")
    parser.add_argument(
        "--list-actions",
        action=argparse.BooleanOptionalAction,
        default=False,
        help="List all actions this script can perform in the order they will be performed")
    parser.add_argument(
        "--root-build-dir",
        default="build",
        help="The build directory, relative to Dive root directory")
    # TODO: b/504654590 - Make a platform-specific default so that users can
    # have more flexibility with choosing a generator
    parser.add_argument(
        "--visual-studio-name",
        default="Visual Studio 17 2022",
        help="Name of Visual Studio version to use on Windows platform")
    return parser.parse_args()


def list_actions():
    """Lists all of ActionType in the order they would be executed
    """
    print("\nIn order of execution...")
    for i, action in enumerate(ActionType):
        print(f"{i+1}. {action}")


def parse_actions(args):
    """Parses args.actions and changes it in-place into a list of valid ActionType actions
    """
    allowed_actions = [str(x) for x in ActionType]
    actions = []
    if args.actions:
        comma_separated = args.actions.split(",")
        for action in comma_separated:
            if action not in allowed_actions:
                raise Exception(f"Invalid action specified: '{action}', refer "
                                "to --list-actions for the valid list OR leave blank to run all")
            actions.append(action)
    else:
        actions = [x for x in ActionType]

    str_actions = ", ".join(actions)
    print(f"\nQueued actions, not necessarily listed in order: {str_actions}")

    args.actions = actions


def check_environment(args):
    """Checks for env vars and executables that will be used by this script.
    Overwrites executables with their absolute path to satisfy the warning for
    subprocess.Popen(). This step can be skipped with --no-prereq-checks
    """

    if args.build_type != BuildType.DEBUG:
        print("WARNING: GFXR gradle build is hardcoded to Debug regardless of --build_type")

    if "ANDROID_NDK_HOME" not in os.environ:
        raise Exception("ANDROID_NDK_HOME env var must be set")
    print(f"\nANDROID_NDK_HOME found: {os.environ["ANDROID_NDK_HOME"]}")

    cmake = shutil.which(args.exec_cmake)
    if not cmake:
        raise Exception(f"Cannot find cmake exec {args.exec_cmake}")
    cmd = [cmake, "--version"]
    dive.echo_and_run(cmd)
    args.exec_cmake = cmake

    ninja = shutil.which(args.exec_ninja)
    if not ninja:
        raise Exception(f"Cannot find ninja exec {args.exec_ninja}")
    cmd = [ninja, "--version"]
    dive.echo_and_run(cmd)
    args.exec_ninja = ninja

    if (platform.system() == "Windows") and (args.build_via_generator):
        msbuild = shutil.which(args.exec_msbuild)
        if not msbuild:
            raise Exception(f"Cannot find msbuild exec {args.exec_msbuild}")
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


def copy_plugins(args):
    """Implements ActionType.COPY_PLUGINS stage
    """
    if not os.path.exists(EXTERNAL_PLUGINS_DIR):
        print(f"\nDir {EXTERNAL_PLUGINS_DIR} does not exist, skipping")
        return

    print("\nCopying over external plugins...")
    print(os.listdir(EXTERNAL_PLUGINS_DIR))
    shutil.copytree(EXTERNAL_PLUGINS_DIR, f"{args.root_build_dir}/pkg/plugins/", dirs_exist_ok=True)


def configure_host(args):
    """Implements ActionType.CONFIGURE_HOST stage, uses cmake to generate
    build files targeting the host
    """
    print("\nGenerating build files with cmake...")
    system_name = platform.system()
    match system_name:
        case "Linux" | "Darwin":
            cmd = [args.exec_cmake, ".",
                   f'-G{NINJA_MULTI_CONFIG_NAME}',
                   f"-B{args.root_build_dir}/host",
                   f"-DDIVE_RELEASE_TYPE={args.dive_release_type}"
                   ]
        case "Windows":
            cmd = [args.exec_cmake, ".",
                   f"-G{args.visual_studio_name}",
                   f"-B{args.root_build_dir}/host",
                   f"-DDIVE_RELEASE_TYPE={args.dive_release_type}"
                   ]
        case _:
            raise Exception(f"Unrecognized platform: {system_name}")
    if args.host_configure_additional_flags:
        for arg in args.host_configure_additional_flags.split(" "):
            if arg:
                cmd.append(arg)
    dive.echo_and_run(cmd)


def build_host(args):
    """Implementing ActionType.BUILD_HOST stage
    """
    if not args.build_via_generator:
        print("\nBuilding host libraries by invoking cmake...")
        cmd = [args.exec_cmake,
               "--build", f"{args.root_build_dir}/host",
               "--config", f"{args.build_type}"
               ]
    elif platform.system() in ["Linux", "Darwin"]:
        print("\nBuilding host libraries by invoking Ninja...")
        cmd = [args.exec_ninja,
               "-f", f"build-{args.build_type}.ninja",
               "-C", f"{args.root_build_dir}/host"
               ]
    elif platform.system() == "Windows":
        print("\nBuilding host libraries by invoking Visual Studio...")
        cmd = [args.exec_msbuild, f"{args.root_build_dir}/host/ALL_BUILD.vcxproj",
               "-maxCpuCount",
               "-t:Rebuild",
               f"-p:Configuration={args.build_type}"
               ]
    else:
        raise Exception(f"Unrecognized platform: {platform.system()}")
    dive.echo_and_run(cmd)


def install_host(args):
    """Implementing ActionType.INSTALL_HOST stage
    """
    # TODO: b/482108095 - This must be done regardless of clean build because
    # install folders are re-used between different build types and the
    # timestamps could affect whether files within are correctly replaced or not
    rmtree_if_exists(f"{args.root_build_dir}/{PKG_DIR}/host")

    print("\nInstalling with cmake...")
    cmd = [args.exec_cmake,
           "--install", f"{args.root_build_dir}/host",
           "--prefix", f"{args.root_build_dir}/{PKG_DIR}",
           "--config", f"{args.build_type}"
           ]
    dive.echo_and_run(cmd)


def all_device(args):
    """Implementing ActionType.ALL_DEVICE stage
    """
    print("\nGenerating build files with cmake...")
    android_ndk_home = os.environ["ANDROID_NDK_HOME"]
    cmd = [args.exec_cmake, ".",
           f"-DCMAKE_TOOLCHAIN_FILE={android_ndk_home}/build/cmake/android.toolchain.cmake",
           f"-G{NINJA_MULTI_CONFIG_NAME}",
           f"-B{args.root_build_dir}/device",
           "-DCMAKE_SYSTEM_NAME=Android",
           "-DANDROID_ABI=arm64-v8a",
           "-DANDROID_PLATFORM=android-26",
           "-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER",
           "-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER",
           f"-DDIVE_RELEASE_TYPE={args.dive_release_type}"
           ]
    if args.ci:
        cmd.append("-DDIVE_GFXR_GRADLE_CONSOLE=plain")
    dive.echo_and_run(cmd)

    print("\nBuilding with ninja...")
    cmd = [args.exec_ninja,
           "-C", f"{args.root_build_dir}/device",
           "-f", f"build-{args.build_type}.ninja"
           ]
    dive.echo_and_run(cmd)

    # TODO: b/482108095 - This must be done regardless of clean build because
    # install folders are re-used between different build types and the
    # timestamps could affect whether files within are correctly replaced or not
    rmtree_if_exists(f"{args.root_build_dir}/{PKG_DIR}/device")

    print("\nInstalling with cmake...")
    cmd = [args.exec_cmake,
           "--install", f"{args.root_build_dir}/device",
           "--prefix", f"{args.root_build_dir}/{PKG_DIR}",
           "--config", f"{args.build_type}"
           ]
    dive.echo_and_run(cmd)


def deploy_qt(args):
    """Implements ActionType.DEPLOY_QT stage, which uses a QT tool to add QT
    libraries to the same dir as the binary that depends on them in preparation
    for packaging and distribution
    """
    # TODO: b/484082504 - Support other platforms
    # TODO: b/504654590 - Investigate if there's a way to identify the binaries without hardcoding them
    system_name = platform.system()
    match system_name:
        case "Linux":
            print("\ndeploy_qt() is UNIMPLEMENTED for Linux...")
            return

        case "Darwin":
            print("\ndeploy_qt() is UNIMPLEMENTED for macOS...")
            return

        case "Windows":
            print(f"\nDeploying with {args.exec_deployqt}...")
            cmd = [args.exec_deployqt, 
                f"{args.root_build_dir}/{PKG_DIR}/host/{WINDOWS_QT_DEPENDENCIES_BINARY}"
                ]
            dive.echo_and_run(cmd)

        case _:
            raise Exception(f"Unrecognized platform: {system_name}")


def package(args):
    """Implements ActionType.PACKAGE stage, which will package this Dive build
    into a single file that is easy to transfer
    """
    # TODO: b/484082504 - Support other platforms
    # TODO: b/504654590 - Investigate if there's a way to identify extraneous files
    system_name = platform.system()
    match system_name:
        case "Linux":
            print("\npackage() is UNIMPLEMENTED for Linux...")
            return

        case "Darwin":
            print("\npackage() is UNIMPLEMENTED for macOS...")
            return

        case "Windows":
            for dir_name in WINDOWS_UNNECESSARY_DIRS:
                rmtree_if_exists(f"{args.root_build_dir}/{PKG_DIR}/host/{dir_name}")
        
            print(f"\nZipping into '{APP_DIR}.zip'...")
            shutil.make_archive(f"{APP_DIR}", "zip", f"{args.root_build_dir}/{PKG_DIR}")

            final_location_zip = f"{args.root_build_dir}/{APP_DIR}.zip"
            if os.path.exists(final_location_zip):
                print("\nClearing previous archive...")
                os.unlink(final_location_zip)

            print(f"\nMoving zip archive to {os.getcwd()}/{final_location_zip}...")
            shutil.move(f"{APP_DIR}.zip", final_location_zip)

        case _:
            raise Exception(f"Unrecognized platform: {system_name}")


def main():
    dive.check_python_version()
    args = parse_args()

    if args.list_actions:
        list_actions()
        return

    # TODO: b/504654590 - Use parser.add_argument() functionality to avoid
    # having to call parse_actions() separately
    parse_actions(args)

    if not args.ci:
        dive_root_path = dive.get_dive_root()
        os.chdir(dive_root_path)

    if args.prereq_checks:
        print(f"\nChecking build environment...")
        check_environment(args)

    with dive.Timer("total"):
        if ActionType.COPY_PLUGINS in args.actions:
            with dive.Timer(ActionType.COPY_PLUGINS):
                copy_plugins(args)
        if ActionType.CONFIGURE_HOST in args.actions:
            with dive.Timer(ActionType.CONFIGURE_HOST):
                configure_host(args)
        if ActionType.BUILD_HOST in args.actions:
            with dive.Timer(ActionType.BUILD_HOST):
                build_host(args)
        if ActionType.INSTALL_HOST in args.actions:
            with dive.Timer(ActionType.INSTALL_HOST):
                install_host(args)
        if ActionType.ALL_DEVICE in args.actions:
            with dive.Timer(ActionType.ALL_DEVICE):
                all_device(args)
        if ActionType.DEPLOY_QT in args.actions:
            with dive.Timer(ActionType.DEPLOY_QT):
                deploy_qt(args)
        if ActionType.PACKAGE in args.actions:
            with dive.Timer(ActionType.PACKAGE):
                package(args)


if __name__ == "__main__":
    main()
