#!/usr/bin/env python3
#
# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import common_dive_utils as dive
import os
import shutil

RELATIVE_INSTALL_DIR = "build/pkg"


def parse_args():
    parser = argparse.ArgumentParser(
        prog="deploy-mac-bundle",
        description='This script automates the bundling for Dive application on MacOS',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        "--sign", 
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Sign the app bundle")
    return parser.parse_args()


def main(args):
    old_working_directory = os.getcwd()

    dive_root_path = dive.get_dive_root()
    os.chdir(dive_root_path)
    os.chdir(RELATIVE_INSTALL_DIR)
    print(f"\nNavigated to folder: {os.getcwd()}")

    # Some initial checks
    print("\nChecking macdeployqt...")
    macdeployqt_exec = shutil.which("macdeployqt")
    assert(macdeployqt_exec is not None)

    if args.sign:
        print("\nChecking codesign...")
        codesign_exec = shutil.which("codesign")
        assert(codesign_exec is not None)

    print("\nSetting up dive.app...")
    # New dive.app was installed with host tools, move it and run macdeployqt
    if not os.path.exists("dive.app"):
        raise Exception("Not detecting any dive.app bundle, have you installed host tools?")

    # Find all plugins and add them via the -executable flag so that internal paths pointing to Homebrew are replaced with the bundle-relative paths.
    plugin_directory = "dive.app/Contents/PlugIns/plugins"
    cmd = [macdeployqt_exec, "dive.app"]
    for root, dirs, files in os.walk(plugin_directory):
        for file in files:
            if file.endswith(".dylib") or file.endswith(".so"):
                plugin_path = os.path.join(root, file)
                cmd.append(f"-executable={plugin_path}")

    dive.echo_and_run(cmd)

    if args.sign:
        cmd = [codesign_exec, "--force", "--deep", "--sign", "-", "dive.app"]
        dive.echo_and_run(cmd)
    else:
        print("\nSkipping signing step because --no-sign...")

    print(f"\nApplication now at {os.getcwd()}/dive.app")

    os.chdir(old_working_directory)


if __name__ == "__main__":
    dive.check_python_version()

    with dive.Timer():
        main(parse_args())
