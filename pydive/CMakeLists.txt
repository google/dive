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

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(pydive)

set(CMAKE_CXX_STANDARD 17)


if(MSVC)
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /MP")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin)

file(GLOB_RECURSE HDR_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB_RECURSE SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})

add_definitions(-DLITTLEENDIAN_CPU)     # Required for some PAL files
add_definitions(-DDIVE_GUI_TOOL)
pybind11_add_module(${PROJECT_NAME} ${HDR_FILES} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE dive_core)

# Don't prefix with lib, python expects a bare .so.
# Also rename output to "_dive", to match the python module name
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "_dive")

if (MSVC)
  # 4100: unreferenced formal parameter
  # 4201: prevent nameless struct/union
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /wd4100 /wd4201)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-braces)
endif()