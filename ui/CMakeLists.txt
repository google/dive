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

# Environment variables prerequisite:
#   set QTDIR=<qt msvc/gcc directory>
#       # Example QTDIR: C:\Qt\5.11.2\msvc2017_64 OR ~/Qt/5.11.2/gcc_64
#   set CMAKE_PREFIX_PATH=%QTDIR%;%CMAKE_PREFIX_PATH%
#   set PATH=%QTDIR%\\bin;%PATH%                        # (Windows) To allow DLLS to be found

project(dive_ui)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)

# Find the QtWidgets library
find_package(Qt5Widgets CONFIG REQUIRED)

if(UNIX AND NOT APPLE)
    add_library(crashpad_client STATIC IMPORTED)
    set_target_properties(
        crashpad_client
        PROPERTIES
            IMPORTED_LOCATION
                "${CMAKE_SOURCE_DIR}/third_party/crashpad/lib/libclient.a"
    )

    add_library(crashpad_util STATIC IMPORTED)
    set_target_properties(
        crashpad_util
        PROPERTIES
            IMPORTED_LOCATION
                "${CMAKE_SOURCE_DIR}/third_party/crashpad/lib/libutil.a"
    )

    add_library(crashpad_base STATIC IMPORTED)
    set_target_properties(
        crashpad_base
        PROPERTIES
            IMPORTED_LOCATION
                "${CMAKE_SOURCE_DIR}/third_party/crashpad/lib/libbase.a"
    )

    # Find system dependencies required by Crashpad on Linux
    find_package(Threads REQUIRED) # For pthread
    find_package(ZLIB REQUIRED) # For zlib
endif()

file(GLOB_RECURSE HDR_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB_RECURSE SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
if(MSVC)
    # Add resource files for Windows build (for version information)
    file(GLOB_RECURSE RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.rc")
    set(SRC_FILES ${SRC_FILES} ${RC_FILES})
endif()

include_directories(${THIRDPARTY_DIRECTORY}/Vulkan-Headers/include)
include_directories(${CMAKE_SOURCE_DIR})
# Include CMAKE_BINARY_DIR for generated files.
include_directories(${CMAKE_BINARY_DIR})
include_directories(${LibArchive_INCLUDE_DIRS})

if(UNIX AND NOT APPLE)
    include_directories(${CMAKE_SOURCE_DIR}/third_party/crashpad/include)
endif()

# Tell CMake to create the Dive executable
add_executable(
    ${PROJECT_NAME}
    ${HDR_FILES}
    ${SRC_FILES}
    ${COMMON_HDR_FILES}
    ${COMMON_SRC_FILES}
    resources.qrc
)

# For release builds, build Dive UI as WIN32 GUI application instead of a console application.
# This will supress logging to stdout.
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Use the Widgets module from Qt 5
target_link_libraries(${PROJECT_NAME} Qt5::Widgets)

target_link_libraries(
    ${PROJECT_NAME}
    network
    dive_core
    device_mgr
    dive_plugin_loader
    component_files
    version_info
)

if(UNIX AND NOT APPLE)
    target_link_libraries(
        ${PROJECT_NAME}
        crashpad_client
        crashpad_util
        crashpad_base
        Threads::Threads # Links pthread
        ZLIB::ZLIB # Links z
        ${CMAKE_DL_LIBS} # Links dl
    )
endif()

# std::filesystem needs to link with libstdc++fs for g++ before 9.0
if(CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(${PROJECT_NAME} stdc++fs)
endif()

# Custom executable name
set_property(TARGET ${PROJECT_NAME} PROPERTY OUTPUT_NAME "dive")

# Required for some PAL files
add_definitions(-DLITTLEENDIAN_CPU)
add_definitions(-DDIVE_GUI_TOOL)

# Set working directory for visual studio, so it can properly load resources
set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

if(MSVC)
    # 4099: missing PDB file for lib
    # 4100: unreferenced formal parameter
    # 4201: prevent nameless struct/union
    # 4127: "conditional expression is constant" warnings
    target_compile_options(
        ${PROJECT_NAME}
        PRIVATE /W4 /WX /wd4099 /wd4100 /wd4201 /wd4127
    )
else()
    target_compile_options(
        ${PROJECT_NAME}
        PRIVATE
            -Wall
            -Wextra
            -Werror
            -Wno-unused-parameter
            -Wno-missing-braces
            -Wno-inconsistent-missing-override
    )
endif()

if(MSVC)
    set_property(
        DIRECTORY ${CMAKE_SOURCE_DIR}
        PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME}
    )
endif()

# Collect 3rd party licenses
file(
    COPY ${CMAKE_SOURCE_DIR}/third_party/Qt/LICENSE.LGPLv3
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/licenses/Qt/
)
file(
    COPY ${CMAKE_SOURCE_DIR}/third_party/pybind11/LICENSE
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/licenses/pybind11/
)
file(
    COPY ${CMAKE_SOURCE_DIR}/third_party/renderdoc/LICENSE
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/licenses/renderdoc/
)
file(
    COPY ${CMAKE_SOURCE_DIR}/third_party/Vulkan-ValidationLayers/LICENSE.txt
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/licenses/Vulkan-ValidationLayers/
)

# Generate license NOTICE file
include("collect_licenses.cmake")
GenerateThirdPartyLicenseFile(
    "${CMAKE_CURRENT_BINARY_DIR}/NOTICE"
    "${CMAKE_CURRENT_BINARY_DIR}/licenses/"
)
add_custom_target(notice_file ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/NOTICE")

# For development
if(UNIX AND NOT APPLE)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND
            "${CMAKE_COMMAND}" -E copy
            "${CMAKE_SOURCE_DIR}/third_party/crashpad/bin/crashpad_handler"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copying crashpad_handler to build directory"
    )
endif()

if(MSVC)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND
            "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_BINARY_DIR}/NOTICE"
            "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>"
    )
endif()

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
endif()

if(APPLE)
    install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION .)
    install(
        CODE
            "
        message(STATUS \"Running macdeployqt...\")
        execute_process(COMMAND \"macdeployqt\" \"${CMAKE_INSTALL_PREFIX}/dive.app\")
        message(STATUS \"Creating install directory in app bundle\")
        execute_process(COMMAND \"${CMAKE_COMMAND}\" -E make_directory \"${CMAKE_INSTALL_PREFIX}/dive.app/Contents/Resources/\")
        message(STATUS \"Copying Android files to app bundle\")
        execute_process(COMMAND \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_INSTALL_PREFIX}/gfxr-replay.apk\" \"${CMAKE_INSTALL_PREFIX}/dive.app/Contents/Resources/\")
        execute_process(COMMAND \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_INSTALL_PREFIX}/gfxrecon.py\" \"${CMAKE_INSTALL_PREFIX}/dive.app/Contents/Resources/\")
        execute_process(COMMAND \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_INSTALL_PREFIX}/libVkLayer_dive.so\" \"${CMAKE_INSTALL_PREFIX}/dive.app/Contents/Resources/\")
        execute_process(COMMAND \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_INSTALL_PREFIX}/libVkLayer_khronos_validation.so\" \"${CMAKE_INSTALL_PREFIX}/dive.app/Contents/Resources/\")
        execute_process(COMMAND \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_INSTALL_PREFIX}/libXrApiLayer_dive.so\" \"${CMAKE_INSTALL_PREFIX}/dive.app/Contents/Resources/\")
        execute_process(COMMAND \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_INSTALL_PREFIX}/XrApiLayer_dive.json\" \"${CMAKE_INSTALL_PREFIX}/dive.app/Contents/Resources/\")
        execute_process(COMMAND \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_INSTALL_PREFIX}/libVkLayer_rt_dive.so\" \"${CMAKE_INSTALL_PREFIX}/dive.app/Contents/Resources/\")
        execute_process(COMMAND \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_INSTALL_PREFIX}/libwrap.so\" \"${CMAKE_INSTALL_PREFIX}/dive.app/Contents/Resources/\")
        execute_process(COMMAND \"${CMAKE_COMMAND}\" -E copy_directory \"${CMAKE_INSTALL_PREFIX}/dive_profiling_plugin/\" \"${CMAKE_INSTALL_PREFIX}/dive.app/Contents/Resources/dive_profiling_plugin\")
        execute_process(COMMAND \"${CMAKE_COMMAND}\" -E copy_directory \"${CMAKE_INSTALL_PREFIX}/gfxr_layer/\" \"${CMAKE_INSTALL_PREFIX}/dive.app/Contents/Resources/gfxr_layer\")
        message(STATUS \"Ad-hoc signing the application bundle\")
        execute_process(COMMAND \"codesign\" --force --deep --sign - \"${CMAKE_INSTALL_PREFIX}/dive.app\")
    "
    )
else()
    install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/NOTICE"
    DESTINATION "${CMAKE_INSTALL_PREFIX}"
)
add_dependencies(${PROJECT_NAME} notice_file)

# For deployment, only works with ninja install or make install
if(UNIX AND NOT APPLE)
    install(
        FILES "${CMAKE_SOURCE_DIR}/third_party/crashpad/bin/crashpad_handler"
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        PERMISSIONS
            OWNER_READ
            OWNER_WRITE
            OWNER_EXECUTE
            GROUP_READ
            GROUP_EXECUTE
            WORLD_READ
            WORLD_EXECUTE
    )
endif()

# You have to rename the `dive_ui_stripped` binary to `dive`
# The Problem: Filename Mismatch
# The stackwalker tool uses the executable's filename to find the correct symbol folder.
# When we run ./dive, the crash dump says: "The module dive crashed."
# The tool looks in: symbols/dive/... -> Found!
# When we run ./dive_ui_stripped, the crash dump says: "The module dive_ui_stripped crashed."
# The tool looks in: symbols/dive_ui_stripped/... -> Not Found.

if(UNIX AND NOT APPLE)
    # Define paths for the output symbol file and the stripped binary
    set(TARGET_BINARY "$<TARGET_FILE:${PROJECT_NAME}>")
    set(SYM_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.sym")
    set(STRIPPED_BINARY "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_stripped")

    # Add a post-build command to generate symbols and strip the binary automatically
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        # Generate the .sym file (requires dump_syms tool in PATH)
        COMMAND dump_syms ${TARGET_BINARY} > ${SYM_FILE}
        # Create a stripped version of the binary for release
        COMMAND strip --strip-unneeded ${TARGET_BINARY} -o ${STRIPPED_BINARY}
        # Log success
        COMMENT
            "Generated breakpad symbols: ${SYM_FILE}\nCreated stripped release binary: ${STRIPPED_BINARY}"
    )
endif()
