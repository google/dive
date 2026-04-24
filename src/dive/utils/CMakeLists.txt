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

message(CHECK_START "Generate build files for src/dive/utils")
list(APPEND CMAKE_MESSAGE_INDENT "  ")

# === component_files ==========================================================

if(NOT ANDROID)
    add_library(component_files component_files.h component_files.cpp)
    target_link_libraries(
        component_files
        PUBLIC dive_src_includes absl::statusor
    )
endif()

# === dive_device_resources_constants ==========================================

# We don't need the two-stage configure but it's consistent with src/dive/build_defs.
configure_file(
    device_resources_constants.h.in
    device_resources_constants_tmp.h.in
)
file(
    GENERATE OUTPUT
        "${CMAKE_BINARY_DIR}/generated/$<CONFIG>/include/dive/utils/device_resources_constants.h"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/device_resources_constants_tmp.h.in"
)

add_library(dive_device_resources_constants INTERFACE)
target_include_directories(
    dive_device_resources_constants
    INTERFACE ${CMAKE_BINARY_DIR}/generated/$<CONFIG>/include
)
target_link_libraries(
    dive_device_resources_constants
    INTERFACE dive_src_includes
)

# === dive_device_resources ====================================================

if(NOT ANDROID)
    add_library(dive_device_resources device_resources.h device_resources.cpp)
    target_link_libraries(
        dive_device_resources
        PUBLIC absl::statusor dive_device_resources_constants
        PRIVATE
            absl::span
            absl::strings
            dive_src_includes
            dive_status
            command_utils
    )
endif()

# === version_info =============================================================

if(NOT ANDROID)
    add_library(version_info version_info.h version_info.cpp)
    target_link_libraries(
        version_info
        PUBLIC absl::statusor absl::strings
        PRIVATE dive_build_defs dive_src_includes
        PRIVATE dive_device_resources
    )
endif()

# === string_utils =============================================================

add_library(string_utils string_utils.h string_utils.cpp)
target_link_libraries(string_utils PRIVATE dive_src_includes)

# === dive_renderdoc_files =====================================================

add_library(dive_renderdoc_files renderdoc_files.h renderdoc_files.cpp)
target_link_libraries(
    dive_renderdoc_files
    PRIVATE dive_device_resources_constants
)

# === tests ====================================================================

if(NOT ANDROID)
    enable_testing()
    include(GoogleTest)

    add_executable(component_files_test component_files_test.cpp)
    target_link_libraries(
        component_files_test
        absl::status_matchers
        component_files
        gmock
        gtest
        gtest_main
    )
    gtest_discover_tests(component_files_test)

    add_executable(string_utils_test string_utils_test.cpp)
    target_link_libraries(
        string_utils_test
        string_utils
        gtest
        gtest_main
        dive_src_includes
    )
    gtest_discover_tests(string_utils_test)

    add_executable(version_info_test version_info_test.cpp)
    target_link_libraries(
        version_info_test
        version_info
        absl::status_matchers
        absl::log
        gmock
        gtest
        gtest_main
        dive_src_includes
    )
    gtest_discover_tests(version_info_test)
endif()

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "done")
