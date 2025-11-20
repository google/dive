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

cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(dive VERSION 1.0.1)

# Global properties
set_property(GLOBAL PROPERTY USE_FOLDERS ON) # Defaults to ON with cmake 3.26+

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()
set(THIRDPARTY_DIRECTORY "${CMAKE_SOURCE_DIR}/third_party")
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install")
# TODO(b/460765024): Prefer $<CONFIG> to CMAKE_BUILD_TYPE to support multi-config generators
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_BUILD_TYPE_LOWERCASE "release")
    set(GFXR_REPLAY_APK_NAME "replay-release-unsigned.apk")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_BUILD_TYPE_LOWERCASE "debug")
    set(GFXR_REPLAY_APK_NAME "replay-debug.apk")
else()
    message(FATAL_ERROR "Must set -DCMAKE_BUILD_TYPE")
endif()

# Include gtest before Abseil so we can use ABSL_BUILD_TEST_HELPERS for status_matchers
if(NOT ANDROID)
    # Ask gtest to use /MD or /MDd since it matches the CMake defaults that we use.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory(third_party/googletest)
    # For highly parallelized builds, performing test dicovery (running a test with --gtest_list_tests) during the build may time out.
    # The high CPU load is likely starving program execution of resources.
    # Therefore, defer test discovery to when ctest is run with the expectation that CPU load is lower.
    set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)
endif()

# When building unit tests, make sure we have access to some TESTONLY libs like status_matcher
if(NOT ANDROID)
    set(ABSL_BUILD_TEST_HELPERS ON CACHE BOOL "")
    # Tell TESTONLY libs that we provide gtest via submodule
    set(ABSL_USE_EXTERNAL_GOOGLETEST ON CACHE BOOL "")
    set(ABSL_FIND_GOOGLETEST OFF CACHE BOOL "")
endif()
add_subdirectory(third_party/abseil-cpp)

if(ANDROID)
    add_subdirectory(third_party/freedreno)
else()
    add_subdirectory(utils)
endif()

# `dive_core/common.h` is used in more places other than dive_core now
add_definitions(-DDIVE_GUI_TOOL)

# Ignore Qt5.15 deprecations.
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050eff)

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE REPO_SHA1
    )
    string(STRIP "${REPO_SHA1}" DIVE_VERSION_SHA1)
endif()

set(DIVE_RELEASE_TYPE
    "dev"
    CACHE STRING
    "Release type can be set with cmake flag -DDIVE_RELEASE_TYPE=<dev|canary|release|internal>"
)
string(STRIP "${DIVE_RELEASE_TYPE}" DIVE_RELEASE_TYPE)

# For host tools version info
set(HOST_PLATFORM_STRING "Unknown")
if(NOT ANDROID)
    if(CMAKE_HOST_WIN32)
        set(HOST_PLATFORM_STRING "win")
    elseif(CMAKE_HOST_APPLE)
        set(HOST_PLATFORM_STRING "mac")
    else()
        set(HOST_PLATFORM_STRING "linux")
    endif()
endif()

# For device libraries version info
set(DEVICE_LIBRARIES_VERSION_FILENAME "DEVICE_LIBRARIES_VERSION.csv")
if(ANDROID)
    message(CHECK_START "Make version file for Dive device libraries")
    file(
        WRITE
        ${CMAKE_INSTALL_PREFIX}/${DEVICE_LIBRARIES_VERSION_FILENAME}
        "sha,"
        ${DIVE_VERSION_SHA1}
        "\nversion,"
        ${CMAKE_PROJECT_VERSION}
        "\nbuild_type,"
        ${CMAKE_BUILD_TYPE}
        "\nrelease_type,"
        ${DIVE_RELEASE_TYPE}
        "\nabi,"
        ${ANDROID_ABI}
    )
    message(CHECK_PASS "successful")
endif()

# Adds all the configure time information into dive_cmake_generated.h.in
configure_file(
    "${CMAKE_SOURCE_DIR}/common/dive_cmake_generated.h.in"
    "${CMAKE_BINARY_DIR}/generated_include/common/dive_cmake_generated.h"
)

include_directories("${CMAKE_BINARY_DIR}/generated_include")

add_subdirectory(network)
add_subdirectory(capture_service)
add_subdirectory(layer)
add_subdirectory(runtime_layer)
add_subdirectory(gpu_time)

# For generated headers
include_directories(${CMAKE_CURRENT_BINARY_DIR}/dive_core)

if(NOT ANDROID)
    set(D3D12_SUPPORT OFF CACHE BOOL "")
    set(GFXRECON_ENABLE_OPENXR OFF CACHE BOOL "" FORCE)
    include(libarchive.cmake)
    add_subdirectory(third_party/gfxreconstruct)
    add_subdirectory(gfxr_ext/decode)
    add_subdirectory(dive_core)
    add_subdirectory(cli)
    add_subdirectory(host_cli)
    add_subdirectory(lrz_validator)
    add_subdirectory(trace_stats)

    add_subdirectory(ui)

    add_subdirectory(gfxr_dump_resources)

    add_subdirectory(plugins)
else()
    add_subdirectory(third_party/Vulkan-ValidationLayers)

    if(NOT CMAKE_HOST_WIN32)
        set(DIVE_GFXR_GRADLE_CONSOLE
            "verbose"
            CACHE STRING
            "Value of --console to use when building GFXR with Gradle"
        )
        message(CHECK_START "Build gfxr for android")
        execute_process(
            COMMAND
                ./gradlew layer:assemble${CMAKE_BUILD_TYPE}
                replay:assemble${CMAKE_BUILD_TYPE}
                --console=${DIVE_GFXR_GRADLE_CONSOLE} -P${ANDROID_ABI}
                -PDisableOpenXR
            WORKING_DIRECTORY
                ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gfxreconstruct/android
            COMMAND_ERROR_IS_FATAL ANY
        )
        message(CHECK_PASS "successful")

        message(CHECK_START "Move gfxr files")
        file(
            ARCHIVE_EXTRACT
            INPUT
                "${CMAKE_CURRENT_SOURCE_DIR}/third_party/gfxreconstruct/android/layer/build/outputs/aar/layer-${CMAKE_BUILD_TYPE_LOWERCASE}.aar"
            DESTINATION "${CMAKE_INSTALL_PREFIX}/gfxr_layer"
        )
        install(
            FILES
                ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gfxreconstruct/android/tools/replay/build/outputs/apk/${CMAKE_BUILD_TYPE_LOWERCASE}/${GFXR_REPLAY_APK_NAME}
            DESTINATION ${CMAKE_INSTALL_PREFIX}
            RENAME "gfxr-replay.apk"
        )
        install(
            FILES
                ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gfxreconstruct/android/scripts/gfxrecon.py
            DESTINATION ${CMAKE_INSTALL_PREFIX}
        )
        message(CHECK_PASS "successful")
    endif()
endif()

option(DIVE_PYTHON_BINDINGS "Build Python bindings for dive")

if(DIVE_PYTHON_BINDINGS)
    find_package(PythonLibs 3.7 REQUIRED)
    set(PYBIND11_DIRECTORY "${CMAKE_SOURCE_DIR}/third_party/pybind11")
    add_subdirectory(third_party/pybind11)
endif()

enable_testing()
add_test(
    NAME TestLoadingUncompressedTrace
    COMMAND
        ${CMAKE_BINARY_DIR}/bin/divecli extract -o .
        ${CMAKE_SOURCE_DIR}/tests/traces/bloom-frame-0052.rd
)
add_test(
    NAME TestLoadingCompressedTrace
    COMMAND
        ${CMAKE_BINARY_DIR}/bin/divecli extract -o .
        ${CMAKE_SOURCE_DIR}/tests/traces/bloom-frame-0080-compressed.rd
)

# MacOS has limited thread stack size, allocating large object in stack makes its stack overflow
# This has been fixed in https://github.com/google/dive/pull/541
if(APPLE)
    set(DIVE_EXECUTABLE_PATH
        ${CMAKE_BINARY_DIR}/ui/dive.app/Contents/MacOS/dive
    )
else()
    set(DIVE_EXECUTABLE_PATH ${CMAKE_BINARY_DIR}/ui/dive)
endif()
add_test(
    NAME Regression_StackOverflow_MacOS
    COMMAND
        ${DIVE_EXECUTABLE_PATH} --exit-after-load
        ${CMAKE_SOURCE_DIR}/tests/gfxr_traces/com.google.bigwheels.project_graphics_pipeline.debug_trim_trigger_20251101T032619.gfxr
)
