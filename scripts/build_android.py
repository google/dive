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
import shutil


class BuildType(enum.StrEnum):
    DEBUG = "Debug"
    REL_WITH_DEB_INFO = "RelWithDebInfo"
    RELEASE = "Release"


def parse_args():
    parser = argparse.ArgumentParser(
        prog="build_android",
        description='This script automates the standard build process for Dive Device Libraries',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        "--dive-release-type", 
        default="dev", 
        help="Description string for Device Libraries version info")
    parser.add_argument(
        "--build-type", 
        type=BuildType, 
        default=BuildType.DEBUG, 
        choices=[str(build_type) for build_type in BuildType],
        help="Build type for Dive libraries (excluding GFXR gradle build which is always Debug)")
    parser.add_argument(
        "--clean-build", 
        action="store_true",
        help="Clean device build folders before rebuilding")
    return parser.parse_args()


def main(args):
    old_working_directory = os.getcwd()

    dive_root_path = dive.get_dive_root()
    os.chdir(dive_root_path)

    # Some initial checks
    if args.build_type != "Debug":
        print("WARNING: GFXR gradle build is hardcoded to Debug regardless of --build_type")
    print("\nChecking env vars...")
    if "ANDROID_NDK_HOME" not in os.environ:
        raise Exception("ANDROID_NDK_HOME env var must be set")
    android_ndk_home = os.environ["ANDROID_NDK_HOME"]
    
    print("\nChecking cmake...")
    cmake_exec = shutil.which("cmake")
    assert(cmake_exec is not None)
    cmd = [cmake_exec, "--version"]
    dive.echo_and_run(cmd)

    print("\nChecking ninja...")
    ninja_exec = shutil.which("ninja")
    assert(ninja_exec is not None)
    cmd = [ninja_exec, "--version"]
    dive.echo_and_run(cmd)

    if args.clean_build:
        print("\nClearing device build folders...")
        if os.path.exists("build/device"):
            shutil.rmtree("build/device")
        if os.path.exists("build/pkg/device"):
            shutil.rmtree("build/pkg/device")


    print("\nGenerating build files with cmake...")
    cmd = [cmake_exec, ".", 
        f"-DCMAKE_TOOLCHAIN_FILE={android_ndk_home}/build/cmake/android.toolchain.cmake",
        "-GNinja Multi-Config",
        "-Bbuild/device",
        "-DCMAKE_SYSTEM_NAME=Android",
        "-DANDROID_ABI=arm64-v8a",
        "-DANDROID_PLATFORM=android-26",
        "-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER",
        "-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER",
        f"-DDIVE_RELEASE_TYPE={args.dive_release_type}"
    ]
    dive.echo_and_run(cmd)

    print("\nBuilding with ninja...")
    cmd = [ninja_exec,
        "-C", "build/device",
        "-f", f"build-{args.build_type}.ninja"
    ]
    dive.echo_and_run(cmd)

    # TODO: b/482108095 - This step is necessary because cmake may not overwrite file of same name even if the contents have changed
    if os.path.exists("build/pkg/device"):
        print("\nClearing install folder...")
        shutil.rmtree("build/pkg/device")

    print("\nInstalling with cmake...")
    cmd = [cmake_exec,
        "--install", "build/device",
        "--prefix", "build/pkg",
        "--config", f"{args.build_type}"
    ]
    dive.echo_and_run(cmd)

    print("\nTIP: Remember to build host tools")
    print("TIP: Remember to place plugins in build/pkg/plugins")
    os.chdir(old_working_directory)


if __name__ == "__main__":
    dive.check_python_version()

    with dive.Timer():
        main(parse_args())
