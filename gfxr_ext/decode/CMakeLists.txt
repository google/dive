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
  dive_annotation_processor.h
  dive_annotation_processor.cpp
  dive_block_data.h
  dive_block_data.cpp
  dive_file_processor.h
  dive_file_processor.cpp
)
target_link_libraries(gfxr_decode_ext_lib PUBLIC
  gfxrecon_decode
  gfxrecon_util
  absl::status
  absl::strings
)
# gfxrecon_util has a psapi dependency on Windows. Ideally, gfxrecon_util should
# declare this dependency in its CMakeLists.txt
if (WIN32)
  target_link_libraries(gfxr_decode_ext_lib PUBLIC psapi)
endif()
target_include_directories(gfxr_decode_ext_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/gfxreconstruct/framework)

# ------------------------
# gfxr_decode_ext_lib_test
# TODO: Figure out a way to build the unit tests on Linux while avoiding X11/Xlib.h preprocessor macro collisions with gtest
if(WIN32)
  enable_testing()
  include(GoogleTest)
  add_executable(gfxr_decode_ext_lib_test 
    dive_annotation_processor_test.cpp
    dive_block_data_test.cpp
    dive_file_processor_test.cpp
  )
  target_link_libraries(gfxr_decode_ext_lib_test PRIVATE
    gfxr_decode_ext_lib
    gmock
    gtest
    gtest_main
  )
  gtest_discover_tests(gfxr_decode_ext_lib_test)
endif()