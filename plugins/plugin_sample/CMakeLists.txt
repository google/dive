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

# --- DivePlugin Sample ---
# This is a plugin demonstrating the IDivePlugin interface.
# It still needs Qt Widgets because it modifies a Qt UI (MainWindow).
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

add_library(dive_plugin_sample SHARED plugin_sample.h plugin_sample.cpp)

target_link_libraries(
    dive_plugin_sample
    PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets dive_plugin_abi_includes
)

set_property(TARGET dive_plugin_sample PROPERTY AUTOMOC ON)

target_include_directories(
    dive_plugin_sample
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/dive_core
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

install(
    TARGETS dive_plugin_sample
    DESTINATION "${DIVE_PLUGINS_PARENT_DIR}/${DIVE_INSTALL_DEST_SAMPLE_PLUGIN}"
)
