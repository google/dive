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

import pathlib
import subprocess
from timeit import default_timer

def get_dive_root(my_env):
    """Predicts Dive root path from:
        1. Env var DIVE_ROOT_PATH, if it exists
        2. Relative position to this script
    """
    if "DIVE_ROOT_PATH" in my_env:
        print("Found DIVE_ROOT_PATH in environment variables")
        dive_root = my_env["DIVE_ROOT_PATH"]
    else:
        print("Predicting DIVE_ROOT_PATH")
        dive_root = pathlib.Path(__file__).parent.parent.resolve()
    
    print(f"DIVE_ROOT_PATH={dive_root}")
    return dive_root

def echo_and_run(cmd):
    print(f"\n> {" ".join(cmd)}")
    subprocess.run(cmd, check=True, text=True)
    return

class timer:
    def __enter__(self):
        self.start = default_timer()
        return

    def __exit__(self, type, value, traceback):
        self.end = default_timer()
        self.elapsed_time = self.end - self.start
        print(f"\nTime Elapsed: {self.elapsed_time:.3f}s")
