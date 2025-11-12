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

project(DiveService)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(WIN32)
    set(COMMAND_UTILS_SRC "command_utils_win32.cc")
    add_definitions(-DUNICODE -D_UNICODE)
else()
    set(COMMAND_UTILS_SRC "command_utils.cc")
endif()

# AndroidTraceManager can be compiled on the host. This allows us to unit test it.
# Group both cc files in the same target since the public header trace_mgr.h declares both.
add_library(trace_mgr android_trace_mgr.cc trace_mgr.cc)
target_link_libraries(trace_mgr PUBLIC absl::core_headers absl::synchronization)
# for #include "common/log.h"
target_include_directories(trace_mgr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(ANDROID)
    add_library(service service.cc ${COMMAND_UTILS_SRC})

    target_link_libraries(service PRIVATE log)
else()
    add_library(service service.cc trace_mgr.cc ${COMMAND_UTILS_SRC})
endif()
target_include_directories(service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(
    service
    PRIVATE network absl::flags absl::flags_parse trace_mgr
)
set_property(TARGET service PROPERTY POSITION_INDEPENDENT_CODE 1)

add_executable(server server.cc)

target_link_libraries(server PRIVATE service absl::flags absl::flags_parse)

if(ANDROID)
    target_link_libraries(service PRIVATE wrap)
else()
    add_library(
        device_mgr
        device_mgr.cc
        ${COMMAND_UTILS_SRC}
        android_application.cc
    )
    target_include_directories(
        device_mgr
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
    )
    target_link_libraries(
        device_mgr
        absl::cleanup
        absl::flags
        absl::flags_parse
        absl::algorithm
        absl::strings
        absl::statusor
        component_files
    )

    add_executable(dive_client_cli dive_client_cli.cc)
    target_link_libraries(
        dive_client_cli
        network
        device_mgr
        absl::flags
        absl::flags_parse
        absl::algorithm
        absl::strings
    )
    target_include_directories(
        dive_client_cli
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
    )

    install(TARGETS dive_client_cli DESTINATION ${CMAKE_INSTALL_PREFIX})

    enable_testing()
    include(GoogleTest)
    add_executable(device_mgr_test device_mgr_test.cc)
    target_link_libraries(
        device_mgr_test
        absl::status_matchers
        device_mgr
        gmock
        gtest
        gtest_main
    )
    gtest_discover_tests(device_mgr_test)

    add_executable(android_trace_mgr_test android_trace_mgr_test.cc)
    target_link_libraries(android_trace_mgr_test trace_mgr gtest gtest_main)
    gtest_discover_tests(android_trace_mgr_test)
endif()
