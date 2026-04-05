#
# Copyright 2026 Google LLC
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

find_package(Qt5 COMPONENTS Core Gui Widgets Test REQUIRED)

add_library(dive_automated_test_plugin SHARED automated_test.cpp)

target_link_libraries(
    dive_automated_test_plugin
    PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Test
        dive_plugin_abi_includes
        gtest
)

target_include_directories(
    dive_automated_test_plugin
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/dive_core
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# TODO: fix sample plugin
install(
    TARGETS dive_automated_test_plugin
    DESTINATION "${DIVE_PLUGINS_PARENT_DIR}"
)
