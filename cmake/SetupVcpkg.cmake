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

set(VCPKG_ROOT "${CMAKE_SOURCE_DIR}/third_party/vcpkg")

if(WIN32)
    set(VCPKG_EXEC "${VCPKG_ROOT}/vcpkg.exe")
    set(BOOTSTRAP_SCRIPT "${VCPKG_ROOT}/bootstrap-vcpkg.bat")
else()
    set(VCPKG_EXEC "${VCPKG_ROOT}/vcpkg")
    set(BOOTSTRAP_SCRIPT "${VCPKG_ROOT}/bootstrap-vcpkg.sh")
endif()

if(NOT EXISTS "${VCPKG_EXEC}")
    message(STATUS "Vcpkg binary not found. Bootstrapping ...")
    if(NOT EXISTS "${BOOTSTRAP_SCRIPT}")
        message(FATAL_ERROR "Vcpkg submodule missing. Run: git submodule update --init --recursive")
    endif()

    execute_process(
        COMMAND "${BOOTSTRAP_SCRIPT}" -disableMetrics
        WORKING_DIRECTORY "${VCPKG_ROOT}"
        RESULT_VARIABLE RESULT
    )
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to bootstrap vcpkg.")
    endif()
endif()

# Turns on Manifest mode automatically because vcpkg.json exists in root.
# Also, it lets cmake to look for libraries inside vcpkg.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()