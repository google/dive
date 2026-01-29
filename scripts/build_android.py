#!/usr/bin/env python3
#
# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import os
import pathlib
import shutil
import subprocess
from timeit import default_timer as timer

def parse_args():
    parser = argparse.ArgumentParser(
        prog="build_android",
        description='This script automates the standard build process for Dive Device Libraries')
    parser.add_argument("--dive_release_type", default="dev", 
        help="Description string for Device Libraries version info (default: dev)")
    parser.add_argument("--build_type", default="Debug", choices=["Debug", "RelWithDebInfo", "Release"], 
        help="Build type for Dive libraries " 
        "(excluding GFXR gradle build which is always Debug) (default: Debug)")
    parser.add_argument("--clean_build", action="store_true",
        help="Clean device build folders before rebuilding"
    )

    return parser.parse_args()

def get_dive_root(my_env):
    """Predicts Dive root path from:
        1. Env var DIVE_ROOT_PATH, if it exists
        2. Relative position to this script
    """
    if "DIVE_ROOT_PATH" in my_env:
        print("Found DIVE_ROOT_PATH in environment variables")
        return my_env["DIVE_ROOT_PATH"]

    print("Predicting DIVE_ROOT_PATH")
    return pathlib.Path(__file__).parent.parent.resolve()

def echo_and_run(cmd):
    print("\n> {}".format(" ".join(cmd)))
    subprocess.run(cmd, check=True, text=True)
    return

def main(args):
    owd = os.getcwd()
    my_env = os.environ.copy()

    dive_root_path = get_dive_root(my_env)
    print("DIVE_ROOT_PATH={}".format(dive_root_path))
    os.chdir(dive_root_path)

    # Some initial checks
    if args.build_type != "Debug":
        print("WARNING: GFXR gradle build is hardcoded to Debug regardless of --build_type")
    print("\nChecking env vars...")
    if "ANDROID_NDK_HOME" not in my_env:
        raise Exception("ANDROID_NDK_HOME env var must be set")
    android_ndk_home = my_env["ANDROID_NDK_HOME"]
    print("\nChecking cmake...")
    cmd = ["cmake", "--version"]
    echo_and_run(cmd)
    print("\nChecking ninja...")
    cmd = ["ninja", "--version"]
    echo_and_run(cmd)

    if (args.clean_build):
        print("\nClearing device build folders...")
        if os.path.exists("build/device"):
            shutil.rmtree("build/device")
        if os.path.exists("build/pkg/device"):
            shutil.rmtree("build/pkg/device")


    print("\nGenerating build files with cmake...")
    cmd = ["cmake", ".", 
        "-DCMAKE_TOOLCHAIN_FILE={}/build/cmake/android.toolchain.cmake".format(android_ndk_home),
        "-GNinja Multi-Config",
        "-Bbuild/device",
        "-DCMAKE_MAKE_PROGRAM=ninja",
        "-DCMAKE_SYSTEM_NAME=Android",
        "-DANDROID_ABI=arm64-v8a",
        "-DANDROID_PLATFORM=android-26",
        "-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=NEVER",
        "-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=NEVER",
        "-DDIVE_RELEASE_TYPE={}".format(args.dive_release_type)
    ]
    echo_and_run(cmd)

    print("\nBuilding with ninja...")
    cmd = ["ninja",
        "-C", "build/device",
        "-f", "build-{}.ninja".format(args.build_type)
    ]
    echo_and_run(cmd)

    # This step is necessary because cmake may not overwrite file of same name even if the contents have changed
    if os.path.exists("build/pkg/device"):
        print("\nClearing install folder...")
        shutil.rmtree("build/pkg/device")

    print("\nInstalling with cmake...")
    cmd = ["cmake",
        "--install", "build/device",
        "--prefix", "build/pkg",
        "--config", "{}".format(args.build_type)
    ]
    echo_and_run(cmd)

    print("\nTIP: Remember to build host tools")
    print("TIP: Remember to place plugins in build/pkg/plugins")
    os.chdir(owd)
    return

if __name__ == "__main__":
    start = timer()
    main(parse_args())
    end = timer()
    print("\nTime Elapsed: {}s".format(end - start))
