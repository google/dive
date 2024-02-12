#
# Copyright 2024 Google LLC
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

cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

#TODO(b/323860315): Windows build is not supported for now
if (NOT WIN32)
    set(DIVE_ENABLE_PERFETTO ON)
endif()

# Build Perfetto SDK
if(DIVE_ENABLE_PERFETTO)
    message("DIVE_ENABLE_PERFETTO is enabled.")
    add_compile_definitions(DIVE_ENABLE_PERFETTO=1)
    set(PERFETTO_ROOT ${CMAKE_SOURCE_DIR}/third_party/perfetto)
    include_directories(${PERFETTO_ROOT}/sdk)
    add_library(perfetto STATIC ${PERFETTO_ROOT}/sdk/perfetto.cc)
if (WIN32)
#  TODO(b/323860315): support import trace_processor on Windows. Currently Ninja generated lib can not be linked
#        correctly on Windows.

# The perfetto library contains many symbols, so it needs the big object
# format.
#target_compile_options(perfetto PRIVATE "/bigobj")
# Disable legacy features in windows.h.
#add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)

#   add_library(trace_processor STATIC IMPORTED)
#   execute_process(
#     COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/prebuild/perfetto/windows/trace_processor.lib.tar.gz
#     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/prebuild/perfetto/windows
#     )
#   set_property(TARGET trace_processor PROPERTY
#                  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/prebuild/perfetto/windows/trace_processor.lib")
#   set(PERFETTO_BUILD_HEADER_DIR ${CMAKE_SOURCE_DIR}/prebuild/perfetto/windows/)
    set(PEFFETTO_TRACE_READER_LIB "")
else()
    add_library(trace_processor STATIC IMPORTED)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_SOURCE_DIR}/prebuild/perfetto/linux/libtrace_processor.a.tar.xz
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/prebuild/perfetto/linux
        )
    set_property(TARGET trace_processor PROPERTY
                     IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/prebuild/perfetto/linux/libtrace_processor.a")
    set(PERFETTO_BUILD_HEADER_DIR ${CMAKE_SOURCE_DIR}/prebuild/perfetto/linux/)
    add_subdirectory(perfetto_trace)
    set(PEFFETTO_TRACE_READER_LIB "perfetto_trace_reader")
endif (WIN32)
endif()

