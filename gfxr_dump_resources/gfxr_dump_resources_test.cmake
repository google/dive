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

# Launches gfxr_dump_resources compares the output to a golden file.
#
# Designed for use with add_test() like so:
#
#   enable_testing()
#   add_test(NAME MyTest
#     COMMAND ${CMAKE_COMMAND} -DTEST_EXECUTABLE=$<TARGET_FILE:gfxr_dump_resources> -DTEST_NAME=MyTest -DINPUT_GFXR=${PROJECT_SOURCE_DIR}/my_capture.gfxr -DGOLDEN_FILE=${PROJECT_SOURCE_DIR}/tests/gfxr_traces/golden/vs_triangle_300_20221211T232110_dump_resources_golden.json -P ${CMAKE_CURRENT_SOURCE_DIR}/gfxr_dump_resources_test.cmake
#   )
#
# TEST_EXECUTABLE will be run with INPUT_GFXR. The test will pass if TEST_EXECUTABLE returns 0 exit code and the contents of the output json match the contents of GOLDEN_FILE.

execute_process(
  COMMAND ${TEST_EXECUTABLE} ${INPUT_GFXR} ${TEST_NAME}.json
  RESULT_VARIABLE exit_code
)
# In CMake 3.19+, prefer COMMAND_ERROR_IS_FATAL (more succinct)
if (NOT exit_code EQUAL 0)
  message(FATAL_ERROR "${TEST_EXECUTABLE} failed: ${exit_code}")
endif()

# Since git lets users specify newline style, normalize both input and output to compare_files.
# This is required so that compare_files works regardless of platform and setting.
# In CMake 3.14+, prefer compare_files --ignore-eol (more succinct)
configure_file(${TEST_NAME}.json ${TEST_NAME}_output_unixeol.json NEWLINE_STYLE UNIX)
configure_file(${GOLDEN_FILE} ${TEST_NAME}_golden_unixeol.json NEWLINE_STYLE UNIX)

execute_process(
  COMMAND ${CMAKE_COMMAND} -E compare_files ${TEST_NAME}_output_unixeol.json ${TEST_NAME}_golden_unixeol.json
  RESULT_VARIABLE exit_code
)
# In CMake 3.19+, prefer COMMAND_ERROR_IS_FATAL (more succinct)
if (NOT exit_code EQUAL 0)
  message(FATAL_ERROR "Output differs from golden file")
endif()
