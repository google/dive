#!/bin/bash

# Copyright 2025 Google LLC
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

echo "Format in-place using gersemi tool"

SRC_DIRS=(
    "capture_service"
    "cli"
    "dive_core"
    "gfxr_dump_resources"
    "gfxr_ext"
    "gpu_time"
    "host_cli"
    "layer"
    "lrz_validator"
    "network"
    "plugins"
    "runtime_layer"
    "trace_stats"
    "ui"
    "utils"
)

echo "Formatting the top-level CMakeLists.txt"
gersemi -i --indent 4 CMakeLists.txt

echo "Formatting the CMakeLists.txt in Dive source code"
for dir in "${SRC_DIRS[@]}"
do
    echo "Formatting: ${dir}..."
    gersemi -i --definitions ${dir} --indent 4 ${dir}
done
