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

message(CHECK_START "Generate build files for dive_core")
if(ANDROID)
    message(CHECK_FAIL "detected Android platform, skipping")
    return()
endif()
list(APPEND CMAKE_MESSAGE_INDENT "  ")

project(dive_core)

set(CMAKE_CXX_STANDARD 20)
include(CheckFunctionExists)
if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif() # MSVC

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(dive_core_includes INTERFACE)

target_include_directories(
    dive_core_includes
    INTERFACE
        ${THIRDPARTY_DIRECTORY}/Vulkan-Headers/include
        ${THIRDPARTY_DIRECTORY}/mesa/src/freedreno/common
        ${THIRDPARTY_DIRECTORY}/mesa/src/
        ${THIRDPARTY_DIRECTORY}/mesa/include/
        ${THIRDPARTY_DIRECTORY}/libarchive/libarchive/
)

if(NOT MSVC AND NOT APPLE)
    add_definitions(-DHAVE_ENDIAN_H) # Required for freedreno_dev_info.c on Linux
endif() # NOT MSVC AND NOT APPLE

add_library(
    ${PROJECT_NAME}
    STATIC
    # Platform specific:
    $<$<PLATFORM_ID:Windows>:event_state.natvis>
    # Common
    common/common.h
    common/dive_capture_format.h
    common/emulate_pm4.cpp
    common/emulate_pm4.h
    common/gpudefs.h
    common/memory_manager_base.h
    common/pm4_packets/ce_pm4_packets.h
    common/pm4_packets/me_pm4_packets.h
    common/pm4_packets/pfp_pm4_packets.h
    # Sources:
    analysis.cpp
    analysis.h
    available_gpu_time.cpp
    available_gpu_time.h
    available_metrics.cpp
    available_metrics.h
    capture_data.h
    capture_event_info.cpp
    capture_event_info.h
    command_hierarchy.cpp
    command_hierarchy.h
    common.cpp
    common.h
    context.h
    conversions.h
    cross_ref.h
    data_core.cpp
    data_core.h
    dive_capture_data.cpp
    dive_capture_data.h
    dive_command_hierarchy.cpp
    dive_command_hierarchy.h
    dive_strings.cpp
    dive_strings.h
    error.cpp
    error.h
    event_state.cpp
    event_state.h
    gfxr_capture_data.cpp
    gfxr_capture_data.h
    gfxr_vulkan_command_hierarchy.cpp
    gfxr_vulkan_command_hierarchy.h
    info_id.h
    "log.cpp"
    "log.h"
    perf_metrics_data.cpp
    perf_metrics_data.h
    pm4_capture_data.cpp
    pm4_capture_data.h
    progress_tracker.h
    shader_disassembly.cpp
    shader_disassembly.h
    sqtt_ids.cpp
    sqtt_ids.h
    struct_of_arrays.h
)

add_dependencies(${PROJECT_NAME} pm4_info)
add_dependencies(${PROJECT_NAME} adreno_header)
add_dependencies(${PROJECT_NAME} a6xx_py)
add_dependencies(${PROJECT_NAME} freedreno_devices_header)

target_link_libraries(${PROJECT_NAME} PUBLIC absl::no_destructor absl::strings)

target_link_libraries(${PROJECT_NAME} PUBLIC dive_freedreno dive_lib_isa)

# This is linked privately because gfxr contains X11 headers that contain preprocessor macros which cause collisions
# gfxr_decode_ext_lib itself can't link gfxrecon_decode privately because it's a dependency of a target (gfxrecon-replay) that doesn't specify PRIVATE/PUBLIC
target_link_libraries(${PROJECT_NAME} PRIVATE gfxr_decode_ext_lib)
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        "${DIVE_REPO_DIR}"
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/gfxreconstruct/framework
)

target_link_libraries(${PROJECT_NAME} PRIVATE string_utils)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    target_link_libraries(${PROJECT_NAME} PRIVATE dl)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
    target_link_libraries(${PROJECT_NAME} PRIVATE z)
    target_link_libraries(${PROJECT_NAME} PRIVATE tinfo)
endif() # "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux"
if(WIN32)
    # Supress warning about deprecation of std::iterator when compile with c++17
    add_definitions(-D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
endif() # WIN32
target_link_libraries(${PROJECT_NAME} PRIVATE ${LibArchive_LIBRARIES})

# This is needed in u_debug.h in mesa, which is included by shader_disassembly.cpp
check_function_exists(secure_getenv HAVE_SECURE_GETENV)
if(HAVE_SECURE_GETENV)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DHAVE_SECURE_GETENV)
endif() # HAVE_SECURE_GETENV

include_directories(${LibArchive_INCLUDE_DIRS})
if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 11) # To support alignas
    set_source_files_properties(
        ${FREEDRENO_SRC_FILES}
        PROPERTIES COMPILE_FLAGS "/WX-"
    )
    # 4100: unreferenced formal parameter
    # 4201: prevent nameless struct/union
    # 4127: conditional expression is constant
    target_compile_options(
        ${PROJECT_NAME}
        PRIVATE /W4 /WX /wd4100 /wd4201 /wd4127
    )
    # Add NOMINMAX definition for Windows builds to prevent min/max macro conflicts
    target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX)
else()
    target_compile_options(
        ${PROJECT_NAME}
        PRIVATE
            -Wall
            -Wextra
            -Werror
            -Wno-unused-parameter
            -Wno-missing-braces
            -Wno-sign-compare
            -Wno-ignored-qualifiers
            -fPIC
            $<$<COMPILE_LANGUAGE:C>:-Wno-override-init>
    )
endif() # MSVC

enable_testing()
add_subdirectory(tests)

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "done")
