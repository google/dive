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

set(DIVE_PLUGINS_BUILD_OUTPUT_DIR "${CMAKE_BINARY_DIR}/plugins")
file(MAKE_DIRECTORY "${DIVE_PLUGINS_BUILD_OUTPUT_DIR}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(DYNAMIC_LIBRARY_LOADER_HEADERS
    idynamic_library_loader.h
)

set(DYNAMIC_LIBRARY_LOADER_SOURCES "")
if (WIN32)
    list(APPEND DYNAMIC_LIBRARY_LOADER_SOURCES windows_library_loader.cpp)
    message(STATUS "Building for Windows: Including windows_library_loader.cpp")
elseif (LINUX)
    list(APPEND DYNAMIC_LIBRARY_LOADER_SOURCES linux_library_loader.cpp)
    message(STATUS "Building for Linux: Including linux_library_loader.cpp")
else()
    message(FATAL_ERROR "Unsupported platform for dynamic library loading. Only Windows and Linux are supported.")
endif()

add_library(dive_plugin_manager STATIC
    plugin_manager.h
    plugin_manager.cpp
    ${DYNAMIC_LIBRARY_LOADER_HEADERS}
    ${DYNAMIC_LIBRARY_LOADER_SOURCES}
)

set_target_properties(dive_plugin_manager PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

if (LINUX)
    find_library(DL_LIBRARY NAMES dl)
    if (DL_LIBRARY)
        target_link_libraries(dive_plugin_manager PRIVATE ${DL_LIBRARY}) 
    else()
        message(FATAL_ERROR "dl library not found on Linux. Required for dynamic loading.")
    endif()
endif()

# For each new plugin, you can add an add_subdirectory call here.
add_subdirectory(plugin_test)