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

file(GLOB_RECURSE HDR_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB_RECURSE SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(dive_lib_os ${HDR_FILES} ${SRC_FILES})
target_link_libraries(dive_lib_os PUBLIC dive_src_includes)

if(WIN32)
    set(COMMAND_UTILS_SRC "command_utils_win32.cc")
    add_definitions(-DUNICODE -D_UNICODE)
else()
    set(COMMAND_UTILS_SRC "command_utils.cc")
endif()

# === command_utils ==========================================================

add_library(command_utils command_utils.h ${COMMAND_UTILS_SRC})
target_link_libraries(command_utils PRIVATE dive_src_includes absl::status)
