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

cmake_minimum_required(VERSION 3.5)

project(diveRuntimeLayer)

set(CMAKE_CXX_STANDARD 17)
set(target_name VkLayer_rt_dive)

set(HDR_FILES "${CMAKE_CURRENT_SOURCE_DIR}/vk_rt_dispatch.h"
                "${CMAKE_CURRENT_SOURCE_DIR}/vk_rt_layer_impl.h"
)

set(SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/vk_rt_dispatch.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/vk_rt_layer_base.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/vk_rt_layer_impl.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/vk_rt_layer_android.cc"
)

include_directories(${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/Vulkan-Headers/include)

add_library(${target_name} SHARED ${HDR_FILES}
                                ${SRC_FILES})

if(ANDROID)
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
    target_link_libraries(${target_name} 
        -lm
        -ldl
        -llog
        -landroid)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(${target_name} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

if (MSVC)
  # 4100: unreferenced formal parameter
  # 4201: prevent nameless struct/union
  target_compile_options(${target_name} PRIVATE /W4 /WX /wd4100 /wd4201)
else()
  target_compile_options(${target_name} PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-braces)
endif()

set_target_properties(${target_name}
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/"
)
if(ANDROID)
    install(TARGETS ${target_name} DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()