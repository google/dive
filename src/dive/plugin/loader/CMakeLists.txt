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

message(CHECK_START "Generate build files for plugin loader")
if(ANDROID)
    message(CHECK_FAIL "detected Android platform, skipping")
    return()
endif()
list(APPEND CMAKE_MESSAGE_INDENT "  ")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    set(DIVE_LIBRARY_LOADER_SOURCE library_loader_windows.cpp)
elseif(LINUX)
    set(DIVE_LIBRARY_LOADER_SOURCE library_loader_linux.cpp)
elseif(APPLE)
    set(DIVE_LIBRARY_LOADER_SOURCE library_loader_macos.cpp)
else()
    message(
        FATAL_ERROR
        "Unsupported platform for dynamic library loading. Only Windows, Linux, and Mac are supported."
    )
endif()

add_library(
    dive_plugin_loader
    STATIC
    plugin_loader.h
    plugin_loader.cpp
    library_loader.h
    ${DIVE_LIBRARY_LOADER_SOURCE}
)

set_target_properties(
    dive_plugin_loader
    PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

if(UNIX)
    find_library(DL_LIBRARY NAMES dl)
    if(DL_LIBRARY)
        target_link_libraries(dive_plugin_loader PRIVATE ${DL_LIBRARY})
    else()
        message(
            FATAL_ERROR
            "dl library not found on Linux or macOS. Required for dynamic loading."
        )
    endif()
endif()

target_link_libraries(
    dive_plugin_loader
    PRIVATE absl::status absl::statusor absl::strings dive_src_includes
)

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "done")
