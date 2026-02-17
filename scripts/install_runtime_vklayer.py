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

import subprocess
import sys
import os

def get_package_name():
    """Prompts the user for the package name, ensuring it is not empty."""
    while True:
        package_name = input("Enter the package name: ").strip()
        if package_name:
            return package_name
        print("Package name cannot be empty. Please enter a valid package name.")

def run_adb_command(command):
    """Executes an ADB command and handles errors."""
    print(f"Executing: {' '.join(command)}")

    try:
        subprocess.run(command, check=True, shell=False)
    except subprocess.CalledProcessError:
        print("--- Error occurred during ADB execution ---")
        print("--- Please check the output above for details ---")
        sys.exit(1)
    except FileNotFoundError:
        print("Error: 'adb' command not found. Please ensure Android SDK Platform-Tools are in your PATH.")
        sys.exit(1)

def main():
    package_name = get_package_name()

    LIB_NAME = "libVkLayer_rt_dive.so"
    TARGET_PATH = "/data/local/tmp"
    LAYER_NAME = "VK_LAYER_Dive"

    source_path = os.path.join(".", "build", "pkg", "device", LIB_NAME)

    print("\n--- Running ADB commands ---")

    run_adb_command(["adb", "root"])

    run_adb_command(["adb", "wait-for-device"])

    run_adb_command(["adb", "shell", "setenforce", "0"])

    run_adb_command(["adb", "push", source_path, TARGET_PATH])

    cp_command = f"run-as {package_name} cp {TARGET_PATH}/{LIB_NAME} ."
    run_adb_command(["adb", "shell", cp_command])

    run_adb_command(["adb", "shell", "settings", "put", "global", "enable_gpu_debug_layers", "1"])

    run_adb_command(["adb", "shell", "settings", "put", "global", "gpu_debug_app", package_name])

    run_adb_command(["adb", "shell", "settings", "put", "global", "gpu_debug_layer_app", package_name])

    run_adb_command(["adb", "shell", "settings", "put", "global", "gpu_debug_layers", LAYER_NAME])

    print("\n\n--- ADB commands completed successfully ---")

if __name__ == "__main__":
    main()
