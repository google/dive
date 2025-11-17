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

enable_testing()
include(GoogleTest)

add_executable(available_metrics_test available_metrics_test.cpp)
target_link_libraries(available_metrics_test gtest gtest_main gmock dive_core)
target_compile_definitions(
    available_metrics_test
    PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
gtest_discover_tests(available_metrics_test)

add_executable(perf_metrics_data_test perf_metrics_data_test.cpp)
target_link_libraries(perf_metrics_data_test gtest gtest_main gmock dive_core)
target_compile_definitions(
    perf_metrics_data_test
    PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
gtest_discover_tests(perf_metrics_data_test)

add_executable(available_gpu_time_test available_gpu_time_test.cpp)
target_link_libraries(available_gpu_time_test gtest gtest_main dive_core)
target_compile_definitions(
    available_gpu_time_test
    PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
gtest_discover_tests(available_gpu_time_test)
