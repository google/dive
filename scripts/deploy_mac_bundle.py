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
from timeit import default_timer as timer

RELATIVE_INSTALL_DIR = "build/pkg"

def parse_args():
    parser = argparse.ArgumentParser(
        prog="deploy_mac_bundle",
        description='This script automates the bundling for Dive application on MacOS')
    parser.add_argument("--no_sign", action="store_true",
        help="Do not sign the app bundle")
    parser.add_argument("--no_device_libraries", action="store_true",
        help="Do not copy over the device librares into the bundle")
    return parser.parse_args()

def main(args):
    owd = os.getcwd()
    my_env = os.environ.copy()

    dive_root_path = dive.get_dive_root(my_env)
    os.chdir(dive_root_path)
    os.chdir(RELATIVE_INSTALL_DIR)
    print("\nNavigated to folder: {}".format(os.getcwd()))

    # Some initial checks
    print("\nChecking macdeployqt...")
    cmd = ["which", "macdeployqt"]
    dive.echo_and_run(cmd)

    print("\nSetting up dive.app...")
    # New dive.app was installed with host tools, move it and run macdeployqt
    if os.path.exists("./host/dive.app"):
        if os.path.exists("./dive.app"):
            shutil.rmtree("./dive.app")
        shutil.move("./host/dive.app", "./dive.app")

        cmd = ["macdeployqt", "dive.app"]
        dive.echo_and_run(cmd)
    # There is an existing dive.app that has been deployed already, skip setup and proceed
    elif os.path.exists("./dive.app"):
        print("\nSkipping setup because it was already done, reinstall host tools to retrigger...")
    else:
        raise Exception("Not detecting any dive.app bundle, have you installed host tools?")

    print("\nCopying resources into app bundle...")

    shutil.copytree("./host", "./dive.app/Contents/MacOS", dirs_exist_ok=True)
    shutil.copytree("./plugins", "./dive.app/Contents/Resources/plugins", dirs_exist_ok=True)

    if args.no_device_libraries:
        print("\nSkipping copying device libraries because --no_device_libraries...")
    else:
        shutil.copytree("./device", "./dive.app/Contents/Resources", dirs_exist_ok=True)

    if args.no_sign:
        print("\nSkipping signing step becaouse --no_sign...")
    else:
        cmd = ["codesign", "--force", "--deep", "--sign", "-", "dive.app"]
        dive.echo_and_run(cmd)

    print("\nApplication now at {}/dive.app".format(os.getcwd()))

    os.chdir(owd)
    return

if __name__ == "__main__":
    start = timer()
    main(parse_args())
    end = timer()
    print("\nTime Elapsed: {}s".format(end - start))
