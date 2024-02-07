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

cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(dive)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(MSVC)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()
set(THIRDPARTY_DIRECTORY "${CMAKE_SOURCE_DIR}/third_party")
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install")

if(ANDROID)
    add_subdirectory(third_party/freedreno)
    find_program(_PROTOBUF_PROTOC protoc PATHS ${CMAKE_SOURCE_DIR}/bin)
    find_program(_GRPC_CPP_PLUGIN grpc_cpp_plugin PATHS ${CMAKE_SOURCE_DIR}/bin)
endif()
if(MSVC)
    option(BUILD_GRPC "Build gRPC from source" "OFF")
else()
    option(BUILD_GRPC "Build gRPC from source" "ON")
endif()
if(BUILD_GRPC)
    set(GRPC_AS_SUBMODULE "ON")
else()
    if(MSVC AND EXISTS "${CMAKE_SOURCE_DIR}/prebuild/grpc/lib/grpc.tar.gz")
        # Unzip certain large debug gRPC libraries
        execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/prebuild/grpc/lib/grpcd.tar.gz
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/prebuild/grpc/lib/grpc.tar.gz
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/prebuild/grpc/lib/grpc_unsecured.tar.gz
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/prebuild/grpc/lib/grpc_unsecure.tar.gz
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/prebuild/grpc/lib/grpc_authorization_providerd.tar.gz
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/prebuild/grpc/lib/libprotocd.tar.gz
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/prebuild/grpc/lib/libprotoc.tar.gz
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_CURRENT_SOURCE_DIR}/prebuild/grpc/lib/libprotobufd.tar.gz
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/prebuild/grpc/lib
        )
    endif()
    set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/prebuild/grpc/")
    link_directories(${CMAKE_SOURCE_DIR}/prebuild/grpc/lib)
endif()

include(grpc.cmake)

# `dive_core/common.h` is used in more places other than dive_core now
add_definitions(-DDIVE_GUI_TOOL)

# Ignore Qt5.15 deprecations.
add_definitions (-DQT_DISABLE_DEPRECATED_BEFORE=0x050eff)

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE REPO_SHA1)
    string(STRIP "${REPO_SHA1}" REPO_SHA1)
    add_definitions( -DREPO_SHA1="${REPO_SHA1}" )
endif()

option(DIVE_NO_DISASSEMBLY, "Include builtin shader disassembly.")
if(DIVE_NO_DISASSEMBLY)
    add_definitions(-DDIVE_NO_DISASSEMBLY=1)
endif()

option(DIVE_ENABLE_PERFETTO, "Enalbe functionality to get perfetto trace." OFF)
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

add_subdirectory(capture_service)
add_subdirectory(layer)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/dive_core)

if(NOT ANDROID)
    include(libarchive.cmake)
    add_subdirectory(dive_core)
    if (EXISTS "${CMAKE_SOURCE_DIR}/cli/CMakeLists.txt")
        add_subdirectory(cli)
    endif()

    if (EXISTS "${CMAKE_SOURCE_DIR}/ui/CMakeLists.txt" AND NOT DIVE_NO_DISASSEMBLY)
        add_subdirectory(ui)
    endif()
endif()

option(DIVE_UNIT_TESTS, "Build unit tests for dive")


option(DIVE_PYTHON_BINDINGS "Build Python bindings for dive")

if(DIVE_PYTHON_BINDINGS)
    find_package(PythonLibs 3.7 REQUIRED)
    set(PYBIND11_DIRECTORY "${CMAKE_SOURCE_DIR}/third_party/pybind11")
    add_subdirectory(third_party/pybind11)
    add_subdirectory(pydive)
endif()

enable_testing()
add_test(NAME TestLoadingUncompressedTrace
         COMMAND ${CMAKE_BINARY_DIR}/bin/divecli extract -o . ${CMAKE_SOURCE_DIR}/tests/traces/bloom-frame-0052.rd)
add_test(NAME TestLoadingCompressedTrace
         COMMAND ${CMAKE_BINARY_DIR}/bin/divecli extract -o . ${CMAKE_SOURCE_DIR}/tests/traces/bloom-frame-0080-compressed.rd)