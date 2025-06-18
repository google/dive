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

# --- DivePluginTest Sample ---
# This is a plugin demonstrating the IDivePlugin interface.
# It still needs Qt Widgets because it modifies a Qt UI (MainWindow).
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED) 

# Use qt5_wrap_cpp to process headers with Q_OBJECT and generate moc files.
# This macro will find all headers with Q_OBJECT and run MOC on them,
# adding the generated source files to a variable.
qt5_wrap_cpp(DIVE_PLUGIN_TEST_MOC_SRCS
    plugin_test.h # Relative to current directory
)

add_library(dive_plugin_test SHARED 
    plugin_test.h
    plugin_test.cpp
    ${DIVE_PLUGIN_TEST_MOC_SRCS} 
)

target_link_libraries(dive_plugin_test
    PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
)

# Set the output directory for the plugin.
# We want it to go into a 'plugins' subdirectory alongside the dive_ui executable.
set_target_properties(dive_plugin_test PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:dive_ui>/plugins"
    RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:dive_ui>/plugins" # For DLLs on Windows
    # On MSVC, ensure the `CreateDivePluginInstance` function is exported.
    # For GCC/Clang, DIVE_PLUGIN_EXPORT handles this.
    # Set a DEFINE_FILE for MSVC to automatically generate exports.
    CXX_EXTENSIONS ON
    GENERATOR_EXPRESSIONS $<$<BOOL:${MSVC}>:"$<TARGET_FILE_BASE_NAME:${PROJECT_NAME}>.def">
	FOLDER "plugins"
)

target_include_directories(dive_plugin_test
  PRIVATE
    ${CMAKE_SOURCE_DIR} 
    ${CMAKE_BINARY_DIR}/dive_core
    ${THIRDPARTY_DIRECTORY}/Vulkan-Headers/include
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Install the plugin to the 'plugins' subdirectory within the installation prefix's bin directory.
# This ensures that when Dive is installed, its plugins are correctly placed.
install(TARGETS dive_plugin_test
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/plugins
)