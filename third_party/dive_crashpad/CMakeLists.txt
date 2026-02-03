#
# Copyright 2026 Google LLC
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

project(dive_crashpad)

message(CHECK_START "Generate build files for third_party/dive_crashpad")
list(APPEND CMAKE_MESSAGE_INDENT "  ")

if(WIN32)
    set(PLATFORM "win32")
    set(LAUNCHER cmd /c)
    set(SCRIPT_NAME "build_crashpad_win32.bat")
    set(HANDLER_NAME "crashpad_handler.exe")
    set(LIB_BASE "base.lib")
    set(LIB_COMMON "common.lib")
    set(LIB_CLIENT "client.lib")
    set(LIB_UTIL "util.lib")
    set(PLATFORM_UTILS "crashpad_utils_win32.cpp")
elseif(APPLE)
    set(PLATFORM "mac")
    set(LAUNCHER bash)
    set(SCRIPT_NAME "build_crashpad_mac.sh")
    set(HANDLER_NAME "crashpad_handler")
    set(LIB_BASE "libbase.a")
    set(LIB_COMMON "libcommon.a")
    set(LIB_CLIENT "libclient.a")
    set(LIB_UTIL "libutil.a")
    set(PLATFORM_UTILS "crashpad_utils_macos.cpp")
else()
    set(PLATFORM "linux")
    set(LAUNCHER bash)
    set(SCRIPT_NAME "build_crashpad_linux.sh")
    set(HANDLER_NAME "crashpad_handler")
    set(LIB_BASE "libbase.a")
    set(LIB_COMMON "libcommon.a")
    set(LIB_CLIENT "libclient.a")
    set(LIB_UTIL "libutil.a")
    set(PLATFORM_UTILS "crashpad_utils_linux.cpp")
endif()

set(PREBUILT_DIR "${CMAKE_CURRENT_LIST_DIR}/prebuilt/${PLATFORM}")

set(CACHED_HANDLER "${PREBUILT_DIR}/${HANDLER_NAME}")
set(CACHED_LIBS
    "${PREBUILT_DIR}/${LIB_CLIENT}"
    "${PREBUILT_DIR}/${LIB_COMMON}"
    "${PREBUILT_DIR}/${LIB_UTIL}"
    "${PREBUILT_DIR}/${LIB_BASE}"
)
set(METADATA_FILE "${PREBUILT_DIR}/build_metadata.txt")
set(COMPILER_STAMP
    "${CMAKE_CXX_COMPILER_ID}_${CMAKE_CXX_COMPILER_VERSION}_${CMAKE_C_COMPILER_ID}_${CMAKE_C_COMPILER_VERSION}"
)

string(REPLACE ";" " " CACHED_LIBS_STR "${CACHED_LIBS}")

add_custom_target(
    dive_crashpad_ensure_binaries
    COMMAND
        ${LAUNCHER} "${CMAKE_CURRENT_LIST_DIR}/${SCRIPT_NAME}"
        "${CMAKE_CURRENT_LIST_DIR}/crashpad_commit.txt" "${PREBUILT_DIR}"
        "${METADATA_FILE}" "$<CONFIG>" "${COMPILER_STAMP}" "${CACHED_LIBS_STR}"
        "${CACHED_HANDLER}"
    BYPRODUCTS ${CACHED_HANDLER} ${CACHED_LIBS} ${METADATA_FILE}
    DEPENDS
        "${CMAKE_CURRENT_LIST_DIR}/${SCRIPT_NAME}"
        "${CMAKE_CURRENT_LIST_DIR}/crashpad_commit.txt"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Verifying Crashpad cache (${PLATFORM}_$<CONFIG>)..."
    VERBATIM
)

add_library(
    dive_crashpad
    STATIC
    dive_crashpad/crashpad_client.cpp
    dive_crashpad/crashpad_client.h
    dive_crashpad/crashpad_utils.h
    dive_crashpad/${PLATFORM_UTILS}
)

add_dependencies(dive_crashpad dive_crashpad_ensure_binaries)

target_link_libraries(
    dive_crashpad
    PUBLIC absl::base absl::status absl::strings
    PRIVATE ${CACHED_LIBS}
)

if(APPLE)
    target_link_libraries(
        dive_crashpad
        PRIVATE
            "-framework AppKit"
            "-framework CoreFoundation"
            "-framework Security"
            "-framework IOKit"
            "bsm"
    )
endif()

target_include_directories(dive_crashpad PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_include_directories(
    dive_crashpad
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/crashpad"
        "${CMAKE_CURRENT_BINARY_DIR}/crashpad/third_party/mini_chromium/mini_chromium"
)
target_include_directories(dive_crashpad PUBLIC "${CMAKE_SOURCE_DIR}/src")

add_executable(crashpad_handler_exe IMPORTED GLOBAL)
set_property(
    TARGET crashpad_handler_exe
    PROPERTY IMPORTED_LOCATION "${CACHED_HANDLER}"
)
add_dependencies(crashpad_handler_exe dive_crashpad_ensure_binaries)

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "done")
