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

message(
    CHECK_START
    "Generate build files for runtime_layer (libVkLayer_rt_dive.so)"
)
if(NOT ANDROID)
    message(CHECK_FAIL "not Android platform, skipping")
    return()
endif()
list(APPEND CMAKE_MESSAGE_INDENT "  ")

include("${dive_SOURCE_DIR}/cmake/compiler_warnings.cmake")
enable_dive_compiler_warnings()

set(CMAKE_CXX_STANDARD 20)

set(HDR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/vk_rt_dispatch.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/vk_rt_layer_impl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/server_message_handler.h"
)

set(SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/vk_rt_dispatch.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/vk_rt_layer_base.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/vk_rt_layer_impl.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/vk_rt_layer_android.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/server_message_handler.cc"
)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/gpu_time)

add_library(VkLayer_rt_dive SHARED ${HDR_FILES} ${SRC_FILES})

target_link_libraries(
    VkLayer_rt_dive
    dive_legacy_includes
    gpu_time
    Vulkan::Headers
    network
    absl::log
    absl::flat_hash_map
    dive_device_resources_constants
)

add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
target_link_libraries(VkLayer_rt_dive -lm -ldl -llog -landroid)

# TODO b/515779388 - Check if generator expression is misused
if($<CONFIG:Release>)
    set_target_properties(VkLayer_rt_dive PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

set_target_properties(
    VkLayer_rt_dive
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/"
)

if(ANDROID)
    install(TARGETS VkLayer_rt_dive DESTINATION "${DIVE_DEST_DEVICE}")
else()
    install(TARGETS VkLayer_rt_dive DESTINATION "${DIVE_DEST_HOST}")
endif()

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "done")
