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
import mako # Not used in this script directly, but imported to check as it's a prerequisite
import os
import platform
import shutil


# GLOBAL VARS
# Build directory structure
ROOT_BUILD_DIR = "build"
PKG_DIR = "pkg"
# Executables on the path
CMAKE_EXEC = "cmake"
NINJA_EXEC = "ninja"
MSBUILD_EXEC = "msbuild"
# CMake generator names
NINJA_MULTI_CONFIG_NAME = "Ninja Multi-Config"


# TODO: b/484082504 - Add more stages for packaging and deploying (incorporate scripts/deploy_mac_bundle.py)
class ActionType(enum.StrEnum):
    CONFIGURE_HOST = "configure_host"
    BUILD_HOST = "build_host"           # separated to make VS builds using the UI easy
    INSTALL_HOST = "install_host"
    ALL_DEVICE = "all_device"           # configure, build, and install device libraries


class BuildType(enum.StrEnum):
    DEBUG = "Debug"
    REL_WITH_DEB_INFO = "RelWithDebInfo"
    RELEASE = "Release"


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
        "--build-with-cmake",
        action="store_true",
        help="Perform the build step by invoking cmake rather than the generator")
    parser.add_argument(
        "--bypass-prereq-checks",
        action="store_true",
        help="Bypass the check for if prerequisites are located as expected")
    parser.add_argument(
        "--clean-build",
        action="store_true",
        help="Attempt to clean relevant build folders before repopulating")
    parser.add_argument(
        "--ci",
        action="store_true",
        help="Build is part of continuous integration")
    parser.add_argument(
        "--dive-release-type",
        default="dev",
        help="Description string for Dive version info")
    parser.add_argument(
        "--host-build-additional-flags",
        help="String of additional CMake flags to pass directly to cmake in the "
        "configure_host stage.")
    parser.add_argument(
        "--list-actions",
        action="store_true",
        help="Build is part of continuous integration")
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


def parse_actions(lis):
    """Parses lis and returns a list of valid ActionType actions
    """
    actions = []
    if lis:
        comma_separated = lis.split(",")
        for action in comma_separated:
            if action not in ActionType:
                raise Exception(f"Invalid action specified: '{action}', refer "
                "to --list-actions for the valid list OR leave blank to run all")
            actions.append(action)
    else:
        actions = [x for x in ActionType]

    print(f"\nQueued actions, not listed in order: {", ".join(actions)}")

    return actions


def check_environment(args):
    """Checks for env vars and executables that will be used by this script. 
    Can be disabled with --bypass-prereq-checks 
    """
    if args.build_type != "Debug":
        print("WARNING: GFXR gradle build is hardcoded to Debug regardless of --build_type")
    print("\nChecking env vars...")
    if "ANDROID_NDK_HOME" not in os.environ:
        raise Exception("ANDROID_NDK_HOME env var must be set")

    print("\nChecking cmake...")
    cmake_exec = shutil.which(CMAKE_EXEC)
    assert(cmake_exec is not None)
    cmd = [cmake_exec, "--version"]
    dive.echo_and_run(cmd)

    print("\nChecking ninja...")
    ninja_exec = shutil.which(NINJA_EXEC)
    assert(ninja_exec is not None)
    cmd = [ninja_exec, "--version"]
    dive.echo_and_run(cmd)

    if (platform.system() == "Windows") and (not args.build_with_cmake):
        msbuild_exec = shutil.which(MSBUILD_EXEC)
        assert(msbuild_exec is not None)
        cmd = [msbuild_exec, "--version"]
        dive.echo_and_run(cmd)


def configure_host(args):
    """Implementing ActionType.CONFIGURE_HOST stage
    """
    if args.clean_build:
        if os.path.exists(f"{ROOT_BUILD_DIR}/host"):
            print("\nClearing host build folders...")
            shutil.rmtree(f"{ROOT_BUILD_DIR}/host")

    print("\nGenerating build files with cmake...")
    if platform.system() in ["Linux", "Darwin"]:
        cmd = [CMAKE_EXEC, ".",
        f'-G{NINJA_MULTI_CONFIG_NAME}',
        f"-B{ROOT_BUILD_DIR}/host",
        f"-DDIVE_RELEASE_TYPE={args.dive_release_type}"
        ]
    elif platform.system() == "Windows":
        cmd = [CMAKE_EXEC, ".",
        f"-G{args.visual_studio_name}",
        f"-B{ROOT_BUILD_DIR}/host",
        f"-DDIVE_RELEASE_TYPE={args.dive_release_type}"
        ]
    else:
        raise Exception(f"Unrecognized platform: {platform.system()}")
    if args.host_build_additional_flags:
        for arg in args.host_build_additional_flags.split(" "):
            if arg:
                cmd.append(arg)
    dive.echo_and_run(cmd)


def build_host(args):
    """Implementing ActionType.BUILD_HOST stage
    """
    if args.build_with_cmake:
        print("\nBuilding host libraries by invoking cmake...")
        cmd = [CMAKE_EXEC,
            "--build", f"{ROOT_BUILD_DIR}/host",
            "--config", f"{args.build_type}"
        ]
    elif platform.system() in ["Linux", "Darwin"]:
        print("\nBuilding host libraries by invoking Ninja...")
        cmd = [NINJA_EXEC, 
        "-f", f"build-{args.build_type}.ninja",
        "-C", f"{ROOT_BUILD_DIR}/host"
        ]
    elif platform.system() == "Windows":
        print("\nBuilding host libraries by invoking Visual Studio...")
        cmd = [MSBUILD_EXEC, f"{ROOT_BUILD_DIR}/host/ALL_BUILD.vcxproj",
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
    if os.path.exists(f"{ROOT_BUILD_DIR}/{PKG_DIR}/host"):
        print("\nClearing host pkg folders...")
        shutil.rmtree(f"{ROOT_BUILD_DIR}/{PKG_DIR}/host")
    
    print("\nInstalling with cmake...")
    cmd = [CMAKE_EXEC,
        "--install", f"{ROOT_BUILD_DIR}/host",
        "--prefix", f"{ROOT_BUILD_DIR}/{PKG_DIR}",
        "--config", f"{args.build_type}"
    ]
    dive.echo_and_run(cmd)


def all_device(args):
    """Implementing ActionType.ALL_DEVICE stage
    """
    if args.clean_build:
        if os.path.exists(f"{ROOT_BUILD_DIR}/device"):
            print("\nClearing device build folders...")
            shutil.rmtree(f"{ROOT_BUILD_DIR}/device")

    print("\nGenerating build files with cmake...")
    android_ndk_home = os.environ["ANDROID_NDK_HOME"]
    cmd = [CMAKE_EXEC, ".",
        f"-DCMAKE_TOOLCHAIN_FILE={android_ndk_home}/build/cmake/android.toolchain.cmake",
        f"-G{NINJA_MULTI_CONFIG_NAME}",
        f"-B{ROOT_BUILD_DIR}/device",
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
    cmd = [NINJA_EXEC,
        "-C", f"{ROOT_BUILD_DIR}/device",
        "-f", f"build-{args.build_type}.ninja"
    ]
    dive.echo_and_run(cmd)

    # TODO: b/482108095 - This must be done regardless of clean build because 
    # install folders are re-used between different build types and the 
    # timestamps could affect whether files within are correctly replaced or not
    if os.path.exists(f"{ROOT_BUILD_DIR}/{PKG_DIR}/device"):
        print("\nClearing install folder...")
        shutil.rmtree(f"{ROOT_BUILD_DIR}/{PKG_DIR}/device")

    print("\nInstalling with cmake...")
    cmd = [CMAKE_EXEC,
        "--install", f"{ROOT_BUILD_DIR}/device",
        "--prefix", f"{ROOT_BUILD_DIR}/{PKG_DIR}",
        "--config", f"{args.build_type}"
    ]
    dive.echo_and_run(cmd)


if __name__ == "__main__":
    dive.check_python_version()
    args = parse_args()

    if args.list_actions:
        list_actions()
        quit()

    actions = parse_actions(args.actions)

    if not args.ci:
        dive_root_path = dive.get_dive_root()
        os.chdir(dive_root_path)

    with dive.Timer("total"):
        if not args.bypass_prereq_checks:
            print(f"\nChecking build environment...")
            check_environment(args)

        if ActionType.CONFIGURE_HOST in actions:
            with dive.Timer(ActionType.CONFIGURE_HOST):
                configure_host(args)
        if ActionType.BUILD_HOST in actions:
            with dive.Timer(ActionType.BUILD_HOST):
                build_host(args)
        if ActionType.INSTALL_HOST in actions:
            with dive.Timer(ActionType.INSTALL_HOST):
                install_host(args)
        if ActionType.ALL_DEVICE in actions:
            with dive.Timer(ActionType.ALL_DEVICE):
                all_device(args)

    print(f"\nTIP: Remember to place plugins in build/pkg/plugins")
