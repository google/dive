#
# Copyright 2023 Google LLC
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

cmake_minimum_required(VERSION 3.0)

project(diveCaptureLayer)

set(CMAKE_CXX_STANDARD 17)
set(target_name VkLayer_dive)

set(HDR_FILES "${CMAKE_CURRENT_SOURCE_DIR}/dispatch.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/layer_impl.h"
)

set(SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/dispatch.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/layer_base.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/layer_impl.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/layer_impl.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/vk_layer_android.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/gles_layer.cc"
)

include_directories(${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/Vulkan-Headers/include
    ${CMAKE_SOURCE_DIR}/third_party/grpc/third_party/abseil-cpp/
    ${CMAKE_SOURCE_DIR}/third_party/grpc/third_party/protobuf/src/)

add_library(${target_name} SHARED ${HDR_FILES}
                                ${SRC_FILES})

add_definitions(-DLAYERNAME=Dive)

set(TARGET_LINK_LIBS PRIVATE    
        dive_grpc_proto
        service
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})

if(ANDROID)
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
    list(APPEND TARGET_LINK_LIBS 
        PRIVATE -lm
        PRIVATE -ldl
        PRIVATE -llog
        PRIVATE -landroid
        PRIVATE wrap)    
endif()

target_link_libraries(${target_name}  ${TARGET_LINK_LIBS})
target_link_directories(${target_name}
PRIVATE ${CMAKE_SOURCE_DIR}/capture_service)

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


set(target_name XrApiLayer_dive)
set(XR_LAYER_SRC_FILES  "${CMAKE_CURRENT_SOURCE_DIR}/openxr_layer.cc"
    ${CMAKE_SOURCE_DIR}/third_party/OpenXR-SDK/src/xr_generated_dispatch_table.c)

add_library(${target_name} SHARED ${XR_LAYER_SRC_FILES})
set_target_properties(${target_name}
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/"
)
target_include_directories(${target_name} PUBLIC 
    ${CMAKE_SOURCE_DIR}/third_party/OpenXR-SDK/include
    ${CMAKE_SOURCE_DIR}/third_party/OpenXR-SDK/src
    ${CMAKE_SOURCE_DIR}/third_party/OpenXR-SDK/src/common
)
    
if(ANDROID)
    target_link_libraries(${target_name}  log)
endif()