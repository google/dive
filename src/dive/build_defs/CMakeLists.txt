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

# Adds all the configure time information into dive_cmake_generated.h.in
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/dive_cmake_generated.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/dive_cmake_generated_tmp.h.in"
)
file(
    GENERATE OUTPUT
        "${CMAKE_BINARY_DIR}/generated/$<CONFIG>/include/dive/build_defs/dive_cmake_generated.h"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/dive_cmake_generated_tmp.h.in"
)

add_library(dive_build_defs INTERFACE)
target_link_libraries(dive_build_defs INTERFACE dive_src_includes)
target_include_directories(
    dive_build_defs
    INTERFACE ${CMAKE_BINARY_DIR}/generated/$<CONFIG>/include
)
