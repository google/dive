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

project(divecli)

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

add_definitions(-DDIVE_GUI_TOOL) # suppress DIVE_PAL_CAPTURE in common.h
add_definitions(-DLITTLEENDIAN_CPU)

set(LIB_SRC_FILES ${SRC_FILES})
list(FILTER LIB_SRC_FILES EXCLUDE REGEX "main.cpp")

add_library(${PROJECT_NAME}_lib ${HDR_FILES} ${LIB_SRC_FILES} ${PM4_GENERATED_SRC_FILE} ${PM4_GENERATED_HDR_FILE})
target_link_libraries(${PROJECT_NAME}_lib PRIVATE dive_core)

add_executable(${PROJECT_NAME} ${HDR_FILES} "main.cpp")
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_lib ${PEFFETTO_TRACE_READER_LIB})

if (MSVC)
  # 4100: unreferenced formal parameter
  # 4201: prevent nameless struct/union
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /wd4100 /wd4201)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-braces)
endif()

# Add NOMINMAX definition for Windows builds to prevent min/max macro conflicts
if (MSVC)
  target_compile_definitions(${PROJECT_NAME}_lib PRIVATE NOMINMAX)
endif()

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
target_link_libraries(${PROJECT_NAME} PRIVATE dl)
target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
target_link_libraries(${PROJECT_NAME} PRIVATE z)
if (NOT GGP)
target_link_libraries(${PROJECT_NAME} PRIVATE tinfo)
endif()
endif()
if(WIN32)
target_link_libraries(${PROJECT_NAME} PRIVATE ${ZLIB_LIBRARIES} -static)
endif()

# Fuzz only on Clang for now.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
add_executable(capture_fuzzer fuzz_main.cpp)
target_compile_definitions(capture_fuzzer PUBLIC -DDIVE_GUI_TOOL)
target_compile_options(capture_fuzzer
            PRIVATE $<$<C_COMPILER_ID:Clang>:-g -O1 -fno-omit-frame-pointer -fsanitize=fuzzer,address>
            )

target_link_libraries(capture_fuzzer
            PRIVATE $<$<C_COMPILER_ID:Clang>:-fsanitize=fuzzer,address>
            dive_core
            ${PEFFETTO_TRACE_READER_LIB}
            )

add_executable(capture_fuzzer_loader fuzz_main.cpp)
target_compile_definitions(capture_fuzzer_loader PUBLIC -DDIVE_GUI_TOOL)
target_compile_definitions(capture_fuzzer_loader PUBLIC -DDIVE_FUZZ_LOADER)

target_compile_options(capture_fuzzer_loader
            PRIVATE $<$<C_COMPILER_ID:Clang>:-g -O1 -fno-omit-frame-pointer -fsanitize=address>
            )

target_link_libraries(capture_fuzzer_loader
            PRIVATE $<$<C_COMPILER_ID:Clang>:-fsanitize=address>
            dive_core
            ${PEFFETTO_TRACE_READER_LIB}
            )
endif()
