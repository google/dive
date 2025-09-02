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
target_include_directories(data_core_wrapper_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../)

# --------
# host_cli
add_executable(host_cli "host_cli_main.cpp")
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

# Creates a test with the given NAME that runs host_cli given INPUT_GFXR file and compares the new modified gfxr output to GOLDEN_FILE.
# This is a wrapper for modify_gfxr_test.cmake that makes tests simpler to define.
function(add_modify_gfxr_test)
  set(oneValueArgs NAME INPUT_GFXR GOLDEN_FILE)
  set(multiValueArgs ADDITIONAL_ARGUMENTS)
  cmake_parse_arguments(PARSE_ARGV 0 arg
      "" "${oneValueArgs}" "${multiValueArgs}"
  )
  add_test(NAME ${arg_NAME}
    COMMAND ${CMAKE_COMMAND} 
    -DTEST_EXECUTABLE=$<TARGET_FILE:host_cli> 
    -DTEST_NAME=${arg_NAME} 
    -DINPUT_GFXR=${arg_INPUT_GFXR} 
    -DGOLDEN_FILE=${arg_GOLDEN_FILE} 
    -DADDITIONAL_ARGUMENTS=${arg_ADDITIONAL_ARGUMENTS} 
    -P ${CMAKE_CURRENT_SOURCE_DIR}/modify_gfxr_test.cmake
  )
endfunction()

add_modify_gfxr_test(NAME ModifyGfxrSeparateAssetsNoOp
  INPUT_GFXR ${PROJECT_SOURCE_DIR}/tests/gfxr_traces/com.google.bigwheels.project_sample_01_triangle.debug_trim_trigger_20250718T132545.gfxr
  GOLDEN_FILE ${PROJECT_SOURCE_DIR}/tests/gfxr_traces/com.google.bigwheels.project_sample_01_triangle.debug_trim_trigger_20250718T132545.gfxr
)

add_modify_gfxr_test(NAME ModifyGfxrSeparateAssetsDeleteBlocks
  INPUT_GFXR ${PROJECT_SOURCE_DIR}/tests/gfxr_traces/com.google.bigwheels.project_sample_01_triangle.debug_trim_trigger_20250718T132545.gfxr
  GOLDEN_FILE ${PROJECT_SOURCE_DIR}/tests/gfxr_traces/golden/com.google.bigwheels.project_sample_01_triangle.debug_trim_trigger_20250718T132545_delete_blocks_220_223.gfxr
  ADDITIONAL_ARGUMENTS --delete_gfxr_blocks=220,223
)