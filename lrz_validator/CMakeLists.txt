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

project(lrz_validator)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 17)


if(MSVC)
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /MP")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB HDR_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

include_directories(${THIRDPARTY_DIRECTORY}/Vulkan-Headers/include 
  ${CMAKE_SOURCE_DIR} 
  ${CMAKE_BINARY_DIR}
  ${LibArchive_INCLUDE_DIRS})

set(LIB_SRC_FILES ${SRC_FILES})
list(FILTER LIB_SRC_FILES EXCLUDE REGEX "main.cpp")

add_executable(${PROJECT_NAME} ${HDR_FILES} "main.cpp")
target_link_libraries(${PROJECT_NAME} PRIVATE dive_core)
target_include_directories(${PROJECT_NAME} PRIVATE ${THIRDPARTY_DIRECTORY}/gfxreconstruct/framework)

if (MSVC)
  # 4100: unreferenced formal parameter
  # 4201: prevent nameless struct/union
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /wd4100 /wd4201)
  target_link_libraries(${PROJECT_NAME} PRIVATE psapi)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-braces)
endif()

