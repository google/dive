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

message(CHECK_START "Generate build files for layer (diveCaptureLayer)")
if(NOT ANDROID)
    message(CHECK_FAIL "not Android platform, skipping")
    return()
endif()
list(APPEND CMAKE_MESSAGE_INDENT "  ")

project(diveCaptureLayer)

include("${dive_SOURCE_DIR}/cmake/compiler_warnings.cmake")
enable_dive_compiler_warnings()

set(CMAKE_CXX_STANDARD 20)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/abseil-cpp/
)

add_library(
    VkLayer_dive
    SHARED
    layer_common.h
    layer_common.cc
    vk_dispatch.h
    vk_dispatch.cc
    vk_layer_impl.h
    vk_layer_android.cc
    vk_layer_base.cc
    vk_layer_impl.cc
    gles_layer.cc
)

add_definitions(-DLAYERNAME=Dive)

set(TARGET_LINK_LIBS PRIVATE dive_legacy_includes trace_mgr pm4_capture_service)

add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
list(
    APPEND TARGET_LINK_LIBS
    PRIVATE
    -lm
    PRIVATE
    -ldl
    PRIVATE
    -llog
    PRIVATE
    -landroid
    PRIVATE
    wrap
)

target_link_libraries(VkLayer_dive ${TARGET_LINK_LIBS} PRIVATE Vulkan::Headers)

# TODO b/515779388 - Check if generator expression is misused
if($<CONFIG:Release>)
    set_target_properties(VkLayer_dive PROPERTIES LINK_FLAGS_RELEASE -s)
endif()
set_target_properties(
    VkLayer_dive
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/"
)
install(TARGETS VkLayer_dive DESTINATION "${DIVE_DEST_DEVICE}")

set(XR_LAYER_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/openxr_layer.cc"
    "${CMAKE_SOURCE_DIR}/third_party/OpenXR-SDK/src/xr_generated_dispatch_table.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/layer_common.cc"
)

add_library(XrApiLayer_dive SHARED ${XR_LAYER_SRC_FILES})
set_target_properties(
    XrApiLayer_dive
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/"
)
target_include_directories(
    XrApiLayer_dive
    PUBLIC
        ${CMAKE_SOURCE_DIR}/third_party/OpenXR-SDK/include
        ${CMAKE_SOURCE_DIR}/third_party/OpenXR-SDK/src
        ${CMAKE_SOURCE_DIR}/third_party/OpenXR-SDK/src/common
)

# TODO b/515779388 - Check if generator expression is misused
if($<CONFIG:Release>)
    set_target_properties(XrApiLayer_dive PROPERTIES LINK_FLAGS_RELEASE -s)
endif()
target_link_libraries(XrApiLayer_dive ${TARGET_LINK_LIBS})

install(TARGETS XrApiLayer_dive DESTINATION "${DIVE_DEST_DEVICE}")

configure_file(manifest/XrApiLayer_dive.json.in manifest/XrApiLayer_dive.json)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/manifest/XrApiLayer_dive.json"
    DESTINATION "${DIVE_DEST_DEVICE}"
)

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "done")
