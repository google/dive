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

# === component_files ==========================================================

if(NOT ANDROID)
    add_library(component_files component_files.h component_files.cpp)
    target_link_libraries(
        component_files
        PUBLIC dive_src_includes absl::statusor
    )
endif()

# === version_info =============================================================

if(NOT ANDROID)
    add_library(version_info version_info.h version_info.cpp)
    target_link_libraries(
        version_info
        PUBLIC absl::statusor absl::strings
        PRIVATE dive_build_defs dive_src_includes
    )
endif()

# === string_utils =============================================================

add_library(string_utils string_utils.h string_utils.cpp)
target_link_libraries(string_utils PRIVATE dive_src_includes)

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
endif()
