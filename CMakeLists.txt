#
# Copyright 2019 Google LLC
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

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(dive)

set(THIRDPARTY_DIRECTORY "${CMAKE_SOURCE_DIR}/third_party")
set(CMAKE_CXX_STANDARD 17)

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

add_subdirectory(layer)
add_subdirectory(dive_core)

if (EXISTS "${CMAKE_SOURCE_DIR}/cli/CMakeLists.txt")
    add_subdirectory(cli)
endif()


if (EXISTS "${CMAKE_SOURCE_DIR}/ui/CMakeLists.txt" AND NOT ANDROID AND NOT DIVE_NO_DISASSEMBLY)
    add_subdirectory(ui)
endif()

option(DIVE_UNIT_TESTS, "Build unit tests for dive")

if(DIVE_UNIT_TESTS)
    set(CATCH2_DIRECTORY "${CMAKE_SOURCE_DIR}/third_party/catch2")
    add_subdirectory(third_party/catch2)
    add_subdirectory(tests/unit_tests)
endif()

option(DIVE_PYTHON_BINDINGS "Build Python bindings for dive")

if(DIVE_PYTHON_BINDINGS)
    find_package(PythonLibs 3.7 REQUIRED)
    set(PYBIND11_DIRECTORY "${CMAKE_SOURCE_DIR}/third_party/pybind11")
    add_subdirectory(third_party/pybind11)
    add_subdirectory(pydive)
endif()

