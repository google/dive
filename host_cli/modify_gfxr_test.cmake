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

# Launches host_cli and uses it to process a gfxr file to create a modified one, then compares the result to a golden file.
#
# Designed for use with add_test() like so:
#
#   enable_testing()
#   add_test(NAME MyTest
#     COMMAND ${CMAKE_COMMAND}
#     -DTEST_EXECUTABLE=$<TARGET_FILE:host_cli>
#     -DTEST_NAME=MyTest -DINPUT_GFXR=my_capture.gfxr
#     -DGOLDEN_FILE=my_capture_golden.gfxr
#     -DADDITIONAL_ARGUMENTS=""
#     -P ${CMAKE_CURRENT_SOURCE_DIR}/modify_gfxr_test.cmake
#   )
#
# TEST_EXECUTABLE will be run with INPUT_GFXR. The test will pass if TEST_EXECUTABLE returns 0 exit code and the contents of the output json match the contents of GOLDEN_FILE.
# TEST_NAME should match the NAME given to add_test(). This is mainly used to ensure that temp files are unique to the test (to support running tests in parallel).
# ADDITIONAL_ARGUMENTS will be passed to TEST_EXECUTABLE in addition to the normal command-line. This is where you can specify options and parameters.

execute_process(
    COMMAND
        ${TEST_EXECUTABLE} ${ADDITIONAL_ARGUMENTS} --input_file_path
        ${INPUT_GFXR} --output_gfxr_path ${TEST_NAME}.gfxr
    RESULT_VARIABLE exit_code
)
# In CMake 3.19+, prefer COMMAND_ERROR_IS_FATAL (more succinct)
if(NOT exit_code EQUAL 0)
    message(FATAL_ERROR "${TEST_EXECUTABLE} failed: ${exit_code}")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E compare_files ${TEST_NAME}.gfxr ${GOLDEN_FILE}
    RESULT_VARIABLE exit_code
)
# In CMake 3.19+, prefer COMMAND_ERROR_IS_FATAL (more succinct)
if(NOT exit_code EQUAL 0)
    message(FATAL_ERROR "Output differs from golden file")
endif()
