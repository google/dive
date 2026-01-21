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

message(CHECK_START "Generate build files for ui")
if(ANDROID)
    message(CHECK_FAIL "detected Android platform, skipping")
    return()
endif()
list(APPEND CMAKE_MESSAGE_INDENT "  ")

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

if(APPLE)
    set_source_files_properties(
        dive.icns
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
    )
endif()

add_executable(
    ${PROJECT_NAME}
    # Qt
    resources.qrc
    # Add resource files for Windows build (for version information)
    $<$<PLATFORM_ID:Windows>:version.rc>
    # MacOS icon
    $<$<PLATFORM_ID:Darwin>:dive.icns>
    # Entry point:
    main.cpp
    # Dive Gui Source:
    about_window.cpp
    about_window.h
    analyze_window.cpp
    analyze_window.h
    application_controller.cpp
    application_controller.h
    capture_file_manager.cpp
    capture_file_manager.h
    capture_worker.cpp
    capture_worker.h
    color_utils.cpp
    color_utils.h
    command_buffer_model.cpp
    command_buffer_model.h
    command_buffer_view.cpp
    command_buffer_view.h
    command_model.cpp
    command_model.h
    command_tab_view.cpp
    command_tab_view.h
    custom_metatypes.cpp
    custom_metatypes.h
    dive_application.cpp
    dive_application.h
    dive_tree_view.cpp
    dive_tree_view.h
    draw_dispatch_stats_model.cpp
    draw_dispatch_stats_model.h
    draw_dispatch_stats_tab_view.cpp
    draw_dispatch_stats_tab_view.h
    error_dialog.cpp
    error_dialog.h
    event_selection_model.cpp
    event_selection_model.h
    event_state_view.cpp
    event_state_view.h
    frame_tab_view.cpp
    frame_tab_view.h
    gfxr_capture_worker.cpp
    gfxr_capture_worker.h
    gfxr_vulkan_command_arguments_filter_proxy_model.cpp
    gfxr_vulkan_command_arguments_filter_proxy_model.h
    gfxr_vulkan_command_arguments_tab_view.cpp
    gfxr_vulkan_command_arguments_tab_view.h
    gfxr_vulkan_command_filter_proxy_model.cpp
    gfxr_vulkan_command_filter_proxy_model.h
    gfxr_vulkan_command_filter.cpp
    gfxr_vulkan_command_filter.h
    gfxr_vulkan_command_model.cpp
    gfxr_vulkan_command_model.h
    gpu_timing_model.cpp
    gpu_timing_model.h
    gpu_timing_tab_view.cpp
    gpu_timing_tab_view.h
    gui_constants.h
    hover_help_model.cpp
    hover_help_model.h
    main_window.cpp
    main_window.h
    misc_stats_model.cpp
    misc_stats_model.h
    misc_stats_tab_view.cpp
    misc_stats_tab_view.h
    most_expensive_events_view.cpp
    most_expensive_events_view.h
    object_names.h
    overview_tab_view.cpp
    overview_tab_view.h
    package_filter.cpp
    package_filter.h
    perf_counter_model.cpp
    perf_counter_model.h
    perf_counter_tab_view.cpp
    perf_counter_tab_view.h
    problems_view.cpp
    problems_view.h
    progress_tracker_callback.cpp
    progress_tracker_callback.h
    property_panel.cpp
    property_panel.h
    search_bar.cpp
    search_bar.h
    search_dialog.cpp
    search_dialog.h
    shader_text_view.cpp
    shader_text_view.h
    shader_view.cpp
    shader_view.h
    shortcuts_window.cpp
    shortcuts_window.h
    shortcuts.h
    text_file_view.cpp
    text_file_view.h
    tile_stats_tab_view.cpp
    tile_stats_tab_view.h
    tool_tip_summaries.cpp
    tool_tip_summaries.h
    trace_window.cpp
    trace_window.h
    tree_view_combo_box.cpp
    tree_view_combo_box.h
    viewport_stats_model.cpp
    viewport_stats_model.h
    window_scissors_stats_model.cpp
    window_scissors_stats_model.h
)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        "${THIRDPARTY_DIRECTORY}/Vulkan-Headers/include"
        "${CMAKE_SOURCE_DIR}"
        # Include CMAKE_BINARY_DIR for generated files.
        "${CMAKE_BINARY_DIR}"
)

# Enable asan and ubsan
if(DIVE_BUILD_WITH_SANITIZER AND NOT MSVC)
    target_compile_options(
        ${PROJECT_NAME}
        PRIVATE $<$<CONFIG:Debug>:-fsanitize=address,undefined>
    )
    target_link_options(
        ${PROJECT_NAME}
        PRIVATE $<$<CONFIG:Debug>:-fsanitize=address,undefined>
    )
endif()

# For release builds, build Dive UI as WIN32 GUI application instead of a console application.
if(MSVC)
    set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
            WIN32_EXECUTABLE $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>
    )
endif()

target_link_libraries(
    ${PROJECT_NAME}
    Qt5::Widgets
    absl::flags
    absl::flags_parse
    absl::failure_signal_handler
    absl::span
    absl::symbolize
    network
    dive_archive
    dive_core
    dive_lib_os
    dive_lib_ui
    device_mgr
    dive_plugin_loader
    component_files
    dive_device_resources
    version_info
    dive_build_defs
    dive_lib_trace_stats
    dive_src_includes
)

# std::filesystem needs to link with libstdc++fs for g++ before 9.0
if(CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(${PROJECT_NAME} stdc++fs)
endif()

# Custom executable name
set_property(TARGET ${PROJECT_NAME} PROPERTY OUTPUT_NAME "dive")

# Required for some PAL files
add_definitions(-DLITTLEENDIAN_CPU)

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
    # 4324: structure was padded due to alignment specifier
    # 4996: _CRT_SECURE_NO_WARNINGS
    target_compile_options(
        ${PROJECT_NAME}
        PRIVATE /W4 /WX /wd4099 /wd4100 /wd4201 /wd4127 /wd4324 /wd4996
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
    "${CMAKE_CURRENT_BINARY_DIR}/${DIVE_LICENSE_FILE_NAME}"
    "${CMAKE_CURRENT_BINARY_DIR}/licenses/"
)
add_custom_target(
    notice_file
    ALL
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${DIVE_LICENSE_FILE_NAME}"
)

if(APPLE)
    set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    )
endif()

if(APPLE)
    install(
        TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION "${DIVE_INSTALL_DEST_HOST}"
    )
else()
    install(TARGETS ${PROJECT_NAME} DESTINATION "${DIVE_INSTALL_DEST_HOST}")
endif()

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${DIVE_LICENSE_FILE_NAME}"
    DESTINATION "${DIVE_INSTALL_DEST_HOST}"
)
add_dependencies(${PROJECT_NAME} notice_file)

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "done")
