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

# Environment variables prerequisite:
#   set QTDIR=<qt msvc/gcc directory>
#       # Example QTDIR: C:\Qt\5.11.2\msvc2017_64 OR ~/Qt/5.11.2/gcc_64
#   set CMAKE_PREFIX_PATH=%QTDIR%;%CMAKE_PREFIX_PATH%
#   set PATH=%QTDIR%\\bin;%PATH%                        # (Windows) To allow DLLS to be found

cmake_minimum_required(VERSION 3.1.0)

project(dive_ui)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)

# Find the QtWidgets library
find_package(Qt5Widgets CONFIG REQUIRED)

file(GLOB_RECURSE HDR_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB_RECURSE SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
if(MSVC)
  # Add resource files for Windows build (for version information)
  file(GLOB_RECURSE RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.rc")
  set(SRC_FILES ${SRC_FILES} ${RC_FILES})
endif()

include_directories(${THIRDPARTY_DIRECTORY}/Vulkan-Headers/include )
include_directories(${CMAKE_SOURCE_DIR})
# Include CMAKE_BINARY_DIR for generated files.
include_directories(${CMAKE_BINARY_DIR})

#include gRPC headers
include_directories(
"${CMAKE_BINARY_DIR}/capture_service"
"${CMAKE_SOURCE_DIR}/third_party/grpc/include"
"${CMAKE_SOURCE_DIR}/third_party/grpc/third_party/protobuf/src/"
)
# Tell CMake to create the Dive executable
add_executable(${PROJECT_NAME} WIN32 ${HDR_FILES} ${SRC_FILES} ${COMMON_HDR_FILES} ${COMMON_SRC_FILES} resources.qrc)

# Use the Widgets module from Qt 5
target_link_libraries(${PROJECT_NAME} Qt5::Widgets)

target_link_libraries(${PROJECT_NAME} dive_core device_mgr client)
# std::filesystem needs to link with libstdc++fs for g++ before 9.0
if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
  target_link_libraries(${PROJECT_NAME}  stdc++fs)
endif()

# Custom executable name
set_property(TARGET ${PROJECT_NAME} PROPERTY OUTPUT_NAME "dive")

# Required for some PAL files
add_definitions(-DLITTLEENDIAN_CPU)
add_definitions(-DDIVE_GUI_TOOL)

# Set working directory for visual studio, so it can properly load resources
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

if (MSVC)
  # 4099: missing PDB file for lib
  # 4100: unreferenced formal parameter
  # 4201: prevent nameless struct/union
  # 4127: "conditional expression is constant" warnings
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /wd4099 /wd4100 /wd4201 /wd4127)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-braces -Wno-inconsistent-missing-override)
endif()

# Collect 3rd party licenses
file(COPY ${CMAKE_SOURCE_DIR}/third_party/Qt/LICENSE.LGPLv3 DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/licenses/Qt/)
file(COPY ${CMAKE_SOURCE_DIR}/third_party/catch2/LICENSE.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/licenses/catch2/)
file(COPY ${CMAKE_SOURCE_DIR}/third_party/pybind11/LICENSE DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/licenses/pybind11/)

# Generate license NOTICE file
include("${CMAKE_SOURCE_DIR}/scripts/collect_licenses.cmake")
GenerateThirdPartyLicenseFile("${CMAKE_CURRENT_BINARY_DIR}/NOTICE" "${CMAKE_CURRENT_BINARY_DIR}/licenses/")
if (MSVC)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_CURRENT_BINARY_DIR}/NOTICE" "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>")
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX})
