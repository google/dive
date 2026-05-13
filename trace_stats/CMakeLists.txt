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

message(CHECK_START "Generate build files for trace_stats")
if(ANDROID)
    message(CHECK_FAIL "detected Android platform, skipping")
    return()
endif()
list(APPEND CMAKE_MESSAGE_INDENT "  ")

project(trace_stats)

set(CMAKE_CXX_STANDARD 20)

if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(${dive_SOURCE_DIR}/cmake/compiler_warnings.cmake)
enable_dive_compiler_warnings()

add_library(dive_lib_trace_stats "trace_stats.cpp" "trace_stats.h")
target_link_libraries(
    dive_lib_trace_stats
    PUBLIC dive_core dive_src_includes Vulkan::Headers
)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} PRIVATE dive_lib_trace_stats)

if(MSVC)
    target_link_libraries(${PROJECT_NAME} PRIVATE psapi)
endif()

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "done")
