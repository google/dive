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

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(DYNAMIC_LIBRARY_LOADER_HEADERS
    idynamic_library_loader.h
)

add_library(dive_plugin_loader STATIC
    plugin_loader.h
    plugin_loader.cpp
    ${DYNAMIC_LIBRARY_LOADER_HEADERS}
)

if (WIN32)
    target_sources(dive_plugin_loader PRIVATE windows_library_loader.cpp)
    message(STATUS "Building for Windows: Including windows_library_loader.cpp")
elseif (LINUX)
    target_sources(dive_plugin_loader PRIVATE linux_library_loader.cpp)
    message(STATUS "Building for Linux: Including linux_library_loader.cpp")
else()
    message(FATAL_ERROR "Unsupported platform for dynamic library loading. Only Windows and Linux are supported.")
endif()

set_target_properties(dive_plugin_loader PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

if (LINUX)
    find_library(DL_LIBRARY NAMES dl)
    if (DL_LIBRARY)
        target_link_libraries(dive_plugin_loader PRIVATE ${DL_LIBRARY})
    else()
        message(FATAL_ERROR "dl library not found on Linux. Required for dynamic loading.")
    endif()
endif()

target_link_libraries(dive_plugin_loader PRIVATE
    absl::status
    absl::statusor
    absl::strings
)

# For each new plugin, you can add an add_subdirectory call here.
add_subdirectory(plugin_sample)

install(CODE "file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/plugins)")
