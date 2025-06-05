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

project(host_cli)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /MP")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(${THIRDPARTY_DIRECTORY}/Vulkan-Headers/include 
  ${CMAKE_SOURCE_DIR} 
  ${CMAKE_BINARY_DIR}
  ${LibArchive_INCLUDE_DIRS})

set(HDR_FILES data_core_wrapper.h)
set(LIB_SRC_FILES data_core_wrapper.cpp)

add_library(${PROJECT_NAME}_lib ${HDR_FILES} ${LIB_SRC_FILES})
target_link_libraries(${PROJECT_NAME}_lib PUBLIC 
  dive_core
  absl::status
)
if(WIN32)
  target_link_libraries(${PROJECT_NAME}_lib PUBLIC psapi)
endif()
target_include_directories(${PROJECT_NAME}_lib PUBLIC ${THIRDPARTY_DIRECTORY}/gfxreconstruct/framework)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} PUBLIC 
  ${PROJECT_NAME}_lib 
  absl::flags
  absl::flags_parse
  absl::status
  absl::str_format
)

enable_testing()
include(GoogleTest)
add_executable(data_core_wrapper_test data_core_wrapper_test.cpp)
target_link_libraries(data_core_wrapper_test
  ${PROJECT_NAME}_lib
  gtest
  gtest_main
  )
gtest_discover_tests(data_core_wrapper_test)