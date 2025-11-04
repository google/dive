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

add_library(
    gfxr_dump_resources_lib
    STATIC
    gfxr_dump_resources.cpp
    state_machine.cpp
    states.cpp
    dump_resources_builder_consumer.cpp
)
# For third_party includes, allow using the full path: #include "third_party/gfxreconstruct/framework/decode/file_processor.h"
target_include_directories(gfxr_dump_resources_lib PRIVATE ..)
# GFXR doesn't need to be PUBLIC since the tool's "public" headers don't expose GFXR constructs
target_link_libraries(gfxr_dump_resources_lib PRIVATE gfxr_decode_ext_lib)

add_executable(gfxr_dump_resources gfxr_dump_resources_main.cpp)
target_include_directories(gfxr_dump_resources PRIVATE ..)
target_link_libraries(
    gfxr_dump_resources
    PRIVATE
        gfxr_dump_resources_lib
        gfxr_decode_ext_lib
        absl::flags
        absl::flags_parse
        absl::flags_usage
)

enable_testing()

# Creates a test with the given NAME that runs gfxr_dump_resources given INPUT_GFXR file and compares the JSON output to GOLDEN_FILE.
# ADDITIONAL_ARGUMENTS are provided to gfxr_dump_resources when it is run.
# This is a wrapper for gfxr_dump_resources_test.cmake that makes tests simpler to define.
function(add_gfxr_dump_resources_test)
    set(oneValueArgs NAME INPUT_GFXR GOLDEN_FILE)
    set(multiValueArgs ADDITIONAL_ARGUMENTS)
    cmake_parse_arguments(
        PARSE_ARGV
        0
        arg
        ""
        "${oneValueArgs}"
        "${multiValueArgs}"
    )
    add_test(
        NAME ${arg_NAME}
        COMMAND
            ${CMAKE_COMMAND}
            -DTEST_EXECUTABLE=$<TARGET_FILE:gfxr_dump_resources>
            -DTEST_NAME=${arg_NAME} -DINPUT_GFXR=${arg_INPUT_GFXR}
            -DGOLDEN_FILE=${arg_GOLDEN_FILE}
            -DADDITIONAL_ARGUMENTS=${arg_ADDITIONAL_ARGUMENTS} -P
            ${CMAKE_CURRENT_SOURCE_DIR}/gfxr_dump_resources_test.cmake
    )
endfunction()

add_gfxr_dump_resources_test(
    NAME GfxrDumpResourcesBasic
    INPUT_GFXR
        ${PROJECT_SOURCE_DIR}/tests/gfxr_traces/vs_triangle_300_20221211T232110.gfxr
    GOLDEN_FILE
        ${PROJECT_SOURCE_DIR}/tests/gfxr_traces/golden/vs_triangle_300_20221211T232110_dump_resources_golden.json
)
add_gfxr_dump_resources_test(
    NAME GfxrDumpResourcesRenderPassKHR2
    INPUT_GFXR
        ${PROJECT_SOURCE_DIR}/tests/gfxr_traces/com.google.bigwheels.project_sample_01_triangle.debug_trim_trigger_20250625T180445.gfxr
    GOLDEN_FILE
        ${PROJECT_SOURCE_DIR}/tests/gfxr_traces/golden/com.google.bigwheels.project_sample_01_triangle.debug_trim_trigger_20250625T180445_dump_resources.json
)
add_gfxr_dump_resources_test(
    NAME GfxrDumpResourcesLastDrawOnly
    INPUT_GFXR
        ${PROJECT_SOURCE_DIR}/tests/gfxr_traces/com.google.bigwheels.project_sample_01_triangle.debug_trim_trigger_20250625T180445.gfxr
    GOLDEN_FILE
        ${PROJECT_SOURCE_DIR}/tests/gfxr_traces/golden/com.google.bigwheels.project_sample_01_triangle.debug_trim_trigger_20250625T180445_dump_resources_last_draw_only.json
    ADDITIONAL_ARGUMENTS --last_draw_only
)
