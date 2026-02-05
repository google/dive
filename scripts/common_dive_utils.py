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

import os
import pathlib
import subprocess
import sys
import timeit


def check_python_version():
    assert sys.version_info >= (3, 9)


def get_dive_root() -> pathlib.Path:
    """Predicts Dive root path from:
        1. Env var DIVE_ROOT_PATH, if it exists
        2. Relative position to this script
    """
    if "DIVE_ROOT_PATH" in os.environ:
        print("Found DIVE_ROOT_PATH in environment variables")
        dive_root = os.environ["DIVE_ROOT_PATH"]
    else:
        print("Predicting DIVE_ROOT_PATH")
        dive_root = pathlib.Path(__file__).parent.parent.resolve()
    
    print(f"DIVE_ROOT_PATH={dive_root}")
    return pathlib.Path(dive_root)


def echo_and_run(cmd):
    print(f"\n> {" ".join(cmd)}")
    subprocess.run(cmd, check=True, text=True)


class Timer:
    def __enter__(self):
        self._start = timeit.default_timer()

    def __exit__(self, type, value, traceback):
        end = timeit.default_timer()
        elapsed_time = end - self._start
        print(f"\nTime Elapsed: {elapsed_time:.3f}s")
