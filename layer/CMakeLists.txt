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

set(CMAKE_CXX_STANDARD 20)
set(target_name VkLayer_dive)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/Vulkan-Headers/include
    ${CMAKE_SOURCE_DIR}/third_party/abseil-cpp/
)

add_library(
    ${target_name}
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

set(TARGET_LINK_LIBS PRIVATE dive_legacy_includes service)

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

target_link_libraries(${target_name} ${TARGET_LINK_LIBS})
target_link_directories(
    ${target_name}
    PRIVATE ${CMAKE_SOURCE_DIR}/capture_service
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(${target_name} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

if(MSVC)
    # 4100: unreferenced formal parameter
    # 4201: prevent nameless struct/union
    target_compile_options(${target_name} PRIVATE /W4 /WX /wd4100 /wd4201)
else()
    target_compile_options(
        ${target_name}
        PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-braces
    )
endif()

set_target_properties(
    ${target_name}
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/"
)
configure_file(
    "manifest/XrApiLayer_dive.json"
    "${CMAKE_BINARY_DIR}/bin/VkLayer_dive_capture.json"
    COPYONLY
)
install(TARGETS ${target_name} DESTINATION ${CMAKE_INSTALL_PREFIX})
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/manifest/XrApiLayer_dive.json"
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)

set(target_name XrApiLayer_dive)
set(XR_LAYER_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/openxr_layer.cc"
    "${CMAKE_SOURCE_DIR}/third_party/OpenXR-SDK/src/xr_generated_dispatch_table.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/layer_common.cc"
)

add_library(${target_name} SHARED ${XR_LAYER_SRC_FILES})
set_target_properties(
    ${target_name}
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/"
)
target_include_directories(
    ${target_name}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/third_party/OpenXR-SDK/include
        ${CMAKE_SOURCE_DIR}/third_party/OpenXR-SDK/src
        ${CMAKE_SOURCE_DIR}/third_party/OpenXR-SDK/src/common
)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(${target_name} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()
target_link_libraries(${target_name} ${TARGET_LINK_LIBS})

install(TARGETS ${target_name} DESTINATION ${CMAKE_INSTALL_PREFIX})
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/manifest/XrApiLayer_dive.json"
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "done")
