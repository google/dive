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

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

add_library(
    dive_lib_ui
    # Header Only:
    forward.h
    types/context.h
    types/file_path.h
    types/impl_pointer.h
    utils/debug_utils.h
    # Sources:
    components/overlay/overlay.cpp
    components/overlay/overlay.h
    components/settings/settings.cpp
    components/settings/settings.h
)

target_link_libraries(
    dive_lib_ui
    PUBLIC
        # Dive dependencies
        dive_src_includes
        # External dependencies
        absl::core_headers
        Qt5::Widgets
)
