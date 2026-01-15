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

add_library(dive_lib_os terminal.cpp terminal.h)
target_link_libraries(dive_lib_os PUBLIC dive_src_includes)

# === command_utils ==========================================================

add_library(
    command_utils
    command_utils.h
    command_utils.cpp
    $<$<PLATFORM_ID:Windows>:command_utils_win32.cpp>
    $<$<PLATFORM_ID:Linux,Darwin>:command_utils_unix.cpp>
)
target_compile_definitions(
    command_utils
    PRIVATE $<$<PLATFORM_ID:Windows>:UNICODE> $<$<PLATFORM_ID:Windows>:_UNICODE>
)
target_link_libraries(command_utils PRIVATE dive_src_includes absl::status)
