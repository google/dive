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

project(host_cli)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /MP")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# TODO: Try to use include_directories sparingly or at the highest level appropriate
include_directories(
    ${THIRDPARTY_DIRECTORY}/Vulkan-Headers/include
)

# ---------------------
# data_core_wrapper_lib
add_library(data_core_wrapper_lib 
  data_core_wrapper.h
  data_core_wrapper.cpp
)
target_link_libraries(data_core_wrapper_lib PUBLIC 
  dive_core
  absl::status
)

# --------
# host_cli
add_executable(host_cli "main.cpp")
target_link_libraries(host_cli PUBLIC 
  data_core_wrapper_lib 
  absl::flags
  absl::flags_parse
  absl::status
  absl::str_format
)

# ---------------------
# data_core_wrapper_test
enable_testing()
include(GoogleTest)
add_executable(data_core_wrapper_test data_core_wrapper_test.cpp)
target_link_libraries(data_core_wrapper_test PRIVATE
  data_core_wrapper_lib
  gtest
  gtest_main
)
gtest_discover_tests(data_core_wrapper_test)