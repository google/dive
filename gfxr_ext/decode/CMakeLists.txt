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

project(gfxr_decode_ext_lib)
set_property(GLOBAL PROPERTY USE_FOLDERS ON) # TODEL UNICORN

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /MP")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# -------------------
# gfxr_decode_ext_lib
add_library(gfxr_decode_ext_lib 
    dive_block_data.h
    dive_block_data.cpp
    dive_file_processor.h
    dive_file_processor.cpp
)
# TODO: Consider making this path included for all dive sources and make sure all includes are using full path
target_include_directories(gfxr_decode_ext_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(gfxr_decode_ext_lib
    gfxrecon_decode
    gfxrecon_format
    gfxrecon_util
)
# gfxr_util has a psapi dependency on Windows. Ideally, gfxr_util should declare this dependency in their CMakeLists.txt
if (WIN32)
    target_link_libraries(gfxr_decode_ext_lib psapi)
endif()

# ------------------------
# gfxr_decode_ext_lib_test
if(NOT ANDROID)
    enable_testing()
    include(GoogleTest)
    add_executable(gfxr_decode_ext_lib_test 
        dive_block_data_test.cpp
        dive_file_processor_test.cpp
    )
    target_link_libraries(gfxr_decode_ext_lib_test
        gfxr_decode_ext_lib
        gtest
        gtest_main
    )
    gtest_discover_tests(gfxr_decode_ext_lib_test)
endif()