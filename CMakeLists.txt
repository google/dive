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

project(dive)

# Global properties
set_property(GLOBAL PROPERTY USE_FOLDERS ON) # Defaults to ON with cmake 3.26+

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(MSVC)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()
set(THIRDPARTY_DIRECTORY "${CMAKE_SOURCE_DIR}/third_party")
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install")
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_BUILD_TYPE_LOWERCASE "release")
    set(GFXR_REPLAY_APK_NAME "replay-release-unsigned.apk")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_BUILD_TYPE_LOWERCASE "debug")
    set(GFXR_REPLAY_APK_NAME "replay-debug.apk")
endif()

# Include gtest before Abseil so we can use ABSL_BUILD_TEST_HELPERS for status_matchers
if (NOT ANDROID)
    # Ask gtest to use /MD or /MDd since it matches the CMake defaults that we use.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory(third_party/googletest)
endif()

# When building unit tests, make sure we have access to some TESTONLY libs like status_matcher
if (NOT ANDROID)
    set(ABSL_BUILD_TEST_HELPERS ON CACHE BOOL "")
    # Tell TESTONLY libs that we provide gtest via submodule
    set(ABSL_USE_EXTERNAL_GOOGLETEST ON CACHE BOOL "")
    set(ABSL_FIND_GOOGLETEST OFF CACHE BOOL "")
endif()
add_subdirectory(third_party/abseil-cpp)

if(ANDROID)
    add_subdirectory(third_party/freedreno)
endif()

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

add_subdirectory(network)
add_subdirectory(capture_service)
add_subdirectory(layer)
add_subdirectory(runtime_layer)
add_subdirectory(gpu_time)

# For generated headers
include_directories(${CMAKE_CURRENT_BINARY_DIR}/dive_core)

if(NOT ANDROID)
    SET(D3D12_SUPPORT OFF CACHE BOOL "")
    add_subdirectory(third_party/gfxreconstruct)
    include(libarchive.cmake)
    add_subdirectory(gfxr_ext/decode)
    add_subdirectory(dive_core)
    add_subdirectory(cli)
    add_subdirectory(host_cli)
    add_subdirectory(lrz_validator)

    if (NOT DIVE_NO_DISASSEMBLY)
        add_subdirectory(ui)
    endif()

    add_subdirectory(gfxr_dump_resources)

    add_subdirectory(plugins)
else()
    if(NOT CMAKE_HOST_WIN32)
        message(CHECK_START "Build gfxr for android")
        execute_process(
            COMMAND ./gradlew assemble${CMAKE_BUILD_TYPE} --console=verbose -P${ANDROID_ABI}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gfxreconstruct/android
            COMMAND_ERROR_IS_FATAL ANY
        )
        message(CHECK_PASS "successful")

        message(CHECK_START "Move gfxr files")
        file(
            ARCHIVE_EXTRACT
            INPUT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/gfxreconstruct/android/layer/build/outputs/aar/layer-${CMAKE_BUILD_TYPE_LOWERCASE}.aar"
            DESTINATION "${CMAKE_INSTALL_PREFIX}/gfxr_layer"
        )
        install(
            FILES ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gfxreconstruct/android/tools/replay/build/outputs/apk/${CMAKE_BUILD_TYPE_LOWERCASE}/${GFXR_REPLAY_APK_NAME} 
            DESTINATION ${CMAKE_INSTALL_PREFIX}
            RENAME "gfxr-replay.apk")
        message(CHECK_PASS "successful")
    endif()
endif()

option(DIVE_UNIT_TESTS, "Build unit tests for dive")


option(DIVE_PYTHON_BINDINGS "Build Python bindings for dive")

if(DIVE_PYTHON_BINDINGS)
    find_package(PythonLibs 3.7 REQUIRED)
    set(PYBIND11_DIRECTORY "${CMAKE_SOURCE_DIR}/third_party/pybind11")
    add_subdirectory(third_party/pybind11)
endif()

enable_testing()
add_test(NAME TestLoadingUncompressedTrace
         COMMAND ${CMAKE_BINARY_DIR}/bin/divecli extract -o . ${CMAKE_SOURCE_DIR}/tests/traces/bloom-frame-0052.rd)
add_test(NAME TestLoadingCompressedTrace
         COMMAND ${CMAKE_BINARY_DIR}/bin/divecli extract -o . ${CMAKE_SOURCE_DIR}/tests/traces/bloom-frame-0080-compressed.rd)
