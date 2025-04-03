#
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
#

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(PerfettoTrace)
set(target perfetto_trace_reader)

add_library(${target} trace_reader.h trace_reader.cpp gpu_slice_data.h gpu_slice_data.cpp )
target_link_libraries(${target}  trace_processor absl::str_format)
target_include_directories(${target} PRIVATE 
    "${THIRDPARTY_DIRECTORY}/perfetto/"
    "${THIRDPARTY_DIRECTORY}/perfetto/include"
    ${PERFETTO_BUILD_HEADER_DIR})