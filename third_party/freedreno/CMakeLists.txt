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
project(libwrap)
set(target_name wrap)

if(NOT ANDROID)
    message( FATAL_ERROR "libwrap can only works on Android." )
endif()

message("Build libwrap for Android")
set(HDR_FILES  "${CMAKE_CURRENT_SOURCE_DIR}/wrap/list.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/wrap/wrap.h"
)

set(SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/wrap/wrap-util.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/wrap/wrap-syscall.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/wrap/load_log.cc"
)

add_library(wrap SHARED ${HDR_FILES} ${SRC_FILES})
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} -frtti -fexceptions")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -DBIONIC_IOCTL_NO_SIGNEDNESS_OVERLOAD")
add_definitions(-DANDROID_STL=c++_shared -DNDK_DEBUG=1)
set(CMAKE_BUILD_TYPE Debug)  # Release build didn't work
target_include_directories(${target_name} 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/includes
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/util)
target_link_libraries(${target_name} log)
set_target_properties(${target_name}
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/"
)
add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND "${ANDROID_TOOLCHAIN_ROOT}/bin/llvm-strip"  --strip-unneeded 
            "${CMAKE_BINARY_DIR}/bin/lib${target_name}.so"
            COMMENT "Strip debug symbols done on final binary.")
