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

message(CHECK_START "Generate build files for gpu_time")
list(APPEND CMAKE_MESSAGE_INDENT "  ")

# Since this might be included from GFXR build, ensure Vulkan::Headers is defined
include(../cmake/vulkan.cmake)

add_library(gpu_time STATIC gpu_time.cpp gpu_time.h)
target_link_libraries(gpu_time PUBLIC Vulkan::Headers)

# This is to fix build on Linux
set_property(TARGET gpu_time PROPERTY POSITION_INDEPENDENT_CODE ON)

if(NOT ANDROID)
    enable_testing()
    include(GoogleTest)
    add_executable(gpu_time_test gpu_time_test.cpp)
    target_link_libraries(gpu_time_test PRIVATE gpu_time gtest gtest_main gmock)
    gtest_discover_tests(gpu_time_test)
endif()

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "done")
